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

#include "MidiOutput.h"

#include "../MidiEvent/MidiEvent.h"

#include <QByteArray>
#include <QFile>
#include <QTextStream>

#include <vector>

#include "rtmidi/RtMidi.h"
#include "MidiOutputBackend.h"
#include "RtMidiBackend.h"
#include "FluidSynthBackend.h"

#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "SenderThread.h"
#include <QSettings>
#include <QFile>

RtMidiOut* MidiOutput::_midiOut = 0;
QString MidiOutput::_outPort = "";
SenderThread* MidiOutput::_sender = new SenderThread();
QMap<int, QList<int> > MidiOutput::playedNotes = QMap<int, QList<int> >();
bool MidiOutput::isAlternativePlayer = false;

int MidiOutput::_stdChannel = 0;

// Backend abstraction
MidiOutputBackend* MidiOutput::_backend = nullptr;
RtMidiBackend* MidiOutput::_rtMidiBackend = nullptr;
FluidSynthBackend* MidiOutput::_fluidSynthBackend = nullptr;

void MidiOutput::init()
{

    // Initialize RtMidi backend
    try {
        _midiOut = new RtMidiOut(RtMidi::UNSPECIFIED, QString("MidiEditor output").toStdString());
        _rtMidiBackend = new RtMidiBackend();
    } catch (RtMidiError& error) {
        error.printMessage();
    }

    // Initialize FluidSynth backend
    _fluidSynthBackend = new FluidSynthBackend();

    // Default to RtMidi backend
    // _backend = _rtMidiBackend;

    _sender->start(QThread::TimeCriticalPriority);
}

void MidiOutput::sendCommand(QByteArray array)
{

    sendEnqueuedCommand(array);
}

void MidiOutput::sendCommand(MidiEvent* e)
{

    if (e->channel() >= 0 && e->channel() < 16 || e->line() == MidiEvent::SYSEX_LINE) {
        _sender->enqueue(e);

        if (isAlternativePlayer) {
            NoteOnEvent* n = dynamic_cast<NoteOnEvent*>(e);
            if (n && n->velocity() > 0) {
                playedNotes[n->channel()].append(n->note());
            } else if (n && n->velocity() == 0) {
                playedNotes[n->channel()].removeOne(n->note());
            } else {
                OffEvent* o = dynamic_cast<OffEvent*>(e);
                if (o) {
                    n = dynamic_cast<NoteOnEvent*>(o->onEvent());
                    if (n) {
                        playedNotes[n->channel()].removeOne(n->note());
                    }
                }
            }
        }
    }
}

QStringList MidiOutput::outputPorts()
{

    QStringList ports;

    // Add RtMidi hardware ports
    if (_rtMidiBackend) {
        ports.append(_rtMidiBackend->availablePorts());
    }

    // Add FluidSynth virtual port
    if (_fluidSynthBackend) {
        ports.append(_fluidSynthBackend->availablePorts());
    }

    return ports;
}

bool MidiOutput::setOutputPort(QString name)
{

    std::cout << "Output set " << name.toStdString() << "\n";
    // Check if it's FluidSynth port
    if (name == "FluidSynth (embedded)") {
        if (_fluidSynthBackend && _fluidSynthBackend->selectPort(name)) {
            _backend = _fluidSynthBackend;
            _outPort = name;

          // Load saved FluidSynth settings
          QSettings settings(QString("MidiEditor"), QString("NONE"));

          // Load and set volume (convert 0-200 integer to 0.0-2.0 double)
          int savedVolume = settings.value("fluidsynth_volume", 50).toInt();
          _fluidSynthBackend->setGain(savedVolume / 100.0);

          // Load soundfont (will be applied when FluidSynth is initialized)
          QString savedPath = settings.value("fluidsynth_soundfont", "").toString();
          if (!QFile::exists(savedPath)) {
              savedPath = _fluidSynthBackend->findDefaultSoundfont();
          }
          _fluidSynthBackend->loadSoundfont(savedPath);
            return true;
        }
        return false;
    }

    // Otherwise, use RtMidi backend
    if (_rtMidiBackend && _rtMidiBackend->selectPort(name)) {
        _backend = _rtMidiBackend;
        _outPort = name;
        return true;
    }

    // Port not found
    return false;
}

QString MidiOutput::outputPort()
{
    return _outPort;
}

void MidiOutput::sendEnqueuedCommand(QByteArray array)
{

    if (_outPort != "" && _backend) {
        _backend->sendMessage(array);
    }
}

void MidiOutput::setStandardChannel(int channel)
{
    _stdChannel = channel;
}

int MidiOutput::standardChannel()
{
    return _stdChannel;
}

void MidiOutput::sendProgram(int channel, int prog)
{
    QByteArray array = QByteArray();
    array.append(0xC0 | channel);
    array.append(prog);
    sendCommand(array);
}

bool MidiOutput::isConnected()
{
    return _outPort != "";
}

FluidSynthBackend* MidiOutput::fluidSynthBackend()
{
    return _fluidSynthBackend;
}
