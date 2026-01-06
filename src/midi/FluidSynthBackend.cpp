/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FluidSynthBackend.h"
#include <QMutexLocker>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>

FluidSynthBackend::FluidSynthBackend()
    : _settings(nullptr)
    , _synth(nullptr)
    , _audioDriver(nullptr)
    , _soundfontId(-1)
    , _soundfontPath("")
    , _selectedPort("")
    , _lastError("")
    , _isInitialized(false)
    , _gain(0.5)
{
}

FluidSynthBackend::~FluidSynthBackend()
{
    shutdownFluidSynth();
}

QStringList FluidSynthBackend::availablePorts()
{
    return QStringList() << "FluidSynth (embedded)";
}

bool FluidSynthBackend::selectPort(QString portName)
{
    if (portName != "FluidSynth (embedded)") {
        return false;
    }

    QMutexLocker locker(&_mutex);

    if (!_isInitialized) {
        initializeFluidSynth();
    }

    if (_isInitialized) {
        _selectedPort = portName;
        return true;
    }

    return false;
}

QString FluidSynthBackend::selectedPort()
{
    return _selectedPort;
}

void FluidSynthBackend::sendMessage(const QByteArray& data)
{
    if (!_isInitialized || data.isEmpty()) {
        return;
    }

    QMutexLocker locker(&_mutex);

    unsigned char status = static_cast<unsigned char>(data[0]);
    unsigned char type = status & 0xF0;
    int channel = status & 0x0F;

    switch (type) {
    case 0x90: // Note On
        if (data.size() >= 3) {
            int key = static_cast<unsigned char>(data[1]);
            int velocity = static_cast<unsigned char>(data[2]);
            if (velocity == 0) {
                fluid_synth_noteoff(_synth, channel, key);
            } else {
                fluid_synth_noteon(_synth, channel, key, velocity);
            }
        }
        break;

    case 0x80: // Note Off
        if (data.size() >= 3) {
            int key = static_cast<unsigned char>(data[1]);
            fluid_synth_noteoff(_synth, channel, key);
        }
        break;

    case 0xB0: // Control Change
        if (data.size() >= 3) {
            int controller = static_cast<unsigned char>(data[1]);
            int value = static_cast<unsigned char>(data[2]);
            fluid_synth_cc(_synth, channel, controller, value);
        }
        break;

    case 0xC0: // Program Change
        if (data.size() >= 2) {
            int program = static_cast<unsigned char>(data[1]);
            fluid_synth_program_change(_synth, channel, program);
        }
        break;

    case 0xE0: // Pitch Bend
        if (data.size() >= 3) {
            int lsb = static_cast<unsigned char>(data[1]);
            int msb = static_cast<unsigned char>(data[2]);
            int value = (msb << 7) | lsb;
            fluid_synth_pitch_bend(_synth, channel, value);
        }
        break;

    case 0xD0: // Channel Pressure
        if (data.size() >= 2) {
            int value = static_cast<unsigned char>(data[1]);
            fluid_synth_channel_pressure(_synth, channel, value);
        }
        break;

    case 0xA0: // Key Pressure (Polyphonic Aftertouch)
        if (data.size() >= 3) {
            int key = static_cast<unsigned char>(data[1]);
            int value = static_cast<unsigned char>(data[2]);
            fluid_synth_key_pressure(_synth, channel, key, value);
        }
        break;
    }
}

void FluidSynthBackend::closePort()
{
    QMutexLocker locker(&_mutex);
    _selectedPort = "";
}

bool FluidSynthBackend::isConnected()
{
    return !_selectedPort.isEmpty();
}

bool FluidSynthBackend::loadSoundfont(const QString& path)
{
    QMutexLocker locker(&_mutex);

    if (!_isInitialized) {
        _lastError = "FluidSynth not initialized";
        return false;
    }

    // Unload previous soundfont if exists
    if (_soundfontId >= 0) {
        fluid_synth_sfunload(_synth, _soundfontId, 1);
        _soundfontId = -1;
    }

    // Load new soundfont
    _soundfontId = fluid_synth_sfload(_synth, path.toUtf8().constData(), 1);
    if (_soundfontId == FLUID_FAILED) {
        _lastError = "Failed to load soundfont: " + path;
        return false;
    }

    _soundfontPath = path;
    _lastError = "";
    return true;
}

QString FluidSynthBackend::currentSoundfont() const
{
    return _soundfontPath;
}

bool FluidSynthBackend::isInitialized() const
{
    return _isInitialized;
}

QString FluidSynthBackend::lastError() const
{
    return _lastError;
}

void FluidSynthBackend::initializeFluidSynth()
{
    // Create settings
    _settings = new_fluid_settings();
    if (!_settings) {
        _lastError = "Failed to create FluidSynth settings";
        qWarning() << _lastError;
        return;
    }

    // Configure audio driver based on platform
#if defined(__LINUX_ALSASEQ__) || defined(__LINUX_ALSA__)
    fluid_settings_setstr(_settings, "audio.driver", "alsa");
#elif defined(__MACOSX_CORE__)
    fluid_settings_setstr(_settings, "audio.driver", "coreaudio");
#elif defined(__WINDOWS_MM__)
    fluid_settings_setstr(_settings, "audio.driver", "dsound");
#else
    fluid_settings_setstr(_settings, "audio.driver", "pulseaudio");
#endif

    // Set audio parameters
    fluid_settings_setnum(_settings, "synth.sample-rate", 44100.0);
    fluid_settings_setint(_settings, "synth.polyphony", 256);
    fluid_settings_setint(_settings, "audio.periods", 8);
    fluid_settings_setint(_settings, "audio.period-size", 512);

    // Create synthesizer
    _synth = new_fluid_synth(_settings);
    if (!_synth) {
        _lastError = "Failed to create FluidSynth synthesizer";
        qWarning() << _lastError;
        delete_fluid_settings(_settings);
        _settings = nullptr;
        return;
    }

    // Create audio driver
    _audioDriver = new_fluid_audio_driver(_settings, _synth);
    if (!_audioDriver) {
        _lastError = "Failed to create FluidSynth audio driver";
        qWarning() << _lastError;
        delete_fluid_synth(_synth);
        delete_fluid_settings(_settings);
        _synth = nullptr;
        _settings = nullptr;
        return;
    }

    _isInitialized = true;
    _lastError = "";

    // Set initial gain
    fluid_synth_set_gain(_synth, _gain);

    qDebug() << "FluidSynth initialized successfully";
}

void FluidSynthBackend::shutdownFluidSynth()
{
    QMutexLocker locker(&_mutex);

    if (_audioDriver) {
        delete_fluid_audio_driver(_audioDriver);
        _audioDriver = nullptr;
    }

    if (_synth) {
        delete_fluid_synth(_synth);
        _synth = nullptr;
    }

    if (_settings) {
        delete_fluid_settings(_settings);
        _settings = nullptr;
    }

    _isInitialized = false;
    _soundfontId = -1;
}

void FluidSynthBackend::setGain(double gain)
{
    QMutexLocker locker(&_mutex);

    // Clamp gain to valid range (0.0 to 10.0, but typically use 0.0 to 2.0)
    _gain = qBound(0.0, gain, 2.0);

    if (_synth) {
        fluid_synth_set_gain(_synth, _gain);
    }
}

double FluidSynthBackend::gain() const
{
    return _gain;
}

QString FluidSynthBackend::findDefaultSoundfont()
{
    // Common soundfont locations on Linux
    QStringList searchPaths;
    searchPaths << "/usr/share/soundfonts"
                << "/usr/share/sounds/sf2"
                << "/usr/local/share/soundfonts"
                << QDir::homePath() + "/.soundfonts"
                << QDir::homePath() + "/soundfonts";

    // Common soundfont filenames
    QStringList soundfontNames;
    soundfontNames << "FluidR3_GM.sf2"
                   << "FluidR3_GS.sf2"
                   << "default.sf2"
                   << "GeneralUser_GS.sf2"
                   << "TimGM6mb.sf2";

    // Search for soundfonts
    foreach (const QString& path, searchPaths) {
        QDir dir(path);
        if (!dir.exists()) {
            continue;
        }

        // Try specific filenames first
        foreach (const QString& name, soundfontNames) {
            QString fullPath = dir.filePath(name);
            if (QFile::exists(fullPath)) {
                qDebug() << "Found default soundfont:" << fullPath;
                return fullPath;
            }
        }

        // If no specific name found, try any .sf2 file
        QStringList sf2Files = dir.entryList(QStringList() << "*.sf2" << "*.SF2", QDir::Files);
        if (!sf2Files.isEmpty()) {
            QString fullPath = dir.filePath(sf2Files.first());
            qDebug() << "Found default soundfont:" << fullPath;
            return fullPath;
        }
    }

    qDebug() << "No default soundfont found in common locations";
    return QString();
}

bool FluidSynthBackend::loadDefaultSoundfont()
{
    QString defaultPath = findDefaultSoundfont();
    if (defaultPath.isEmpty()) {
        _lastError = "No default soundfont found. Please select a .sf2 file manually.";
        return false;
    }

    return loadSoundfont(defaultPath);
}
