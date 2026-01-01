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

#ifndef FLUIDSYNTHBACKEND_H_
#define FLUIDSYNTHBACKEND_H_

#include "MidiOutputBackend.h"
#include <QMutex>
#include <fluidsynth.h>

/**
 * MIDI output backend using FluidSynth for software synthesis.
 */
class FluidSynthBackend : public MidiOutputBackend {
public:
    FluidSynthBackend();
    ~FluidSynthBackend() override;

    QStringList availablePorts() override;
    bool selectPort(QString portName) override;
    QString selectedPort() override;
    void sendMessage(const QByteArray& data) override;
    void closePort() override;
    bool isConnected() override;

    // FluidSynth-specific methods
    bool loadSoundfont(const QString& path);
    QString currentSoundfont() const;
    bool isInitialized() const;
    QString lastError() const;

    // Volume control (0.0 to 1.0, default 0.5)
    void setGain(double gain);
    double gain() const;

    // Default soundfont detection
    QString findDefaultSoundfont();
    bool loadDefaultSoundfont();

private:
    void initializeFluidSynth();
    void shutdownFluidSynth();

    fluid_settings_t* _settings;
    fluid_synth_t* _synth;
    fluid_audio_driver_t* _audioDriver;
    int _soundfontId;
    QString _soundfontPath;
    QString _selectedPort;
    QString _lastError;
    bool _isInitialized;
    double _gain;
    QMutex _mutex;
};

#endif
