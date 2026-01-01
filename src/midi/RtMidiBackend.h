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

#ifndef RTMIDIBACKEND_H_
#define RTMIDIBACKEND_H_

#include "MidiOutputBackend.h"

class RtMidiOut;

/**
 * MIDI output backend using RtMidi library for hardware MIDI devices.
 */
class RtMidiBackend : public MidiOutputBackend {
public:
    RtMidiBackend();
    ~RtMidiBackend() override;

    QStringList availablePorts() override;
    bool selectPort(QString portName) override;
    QString selectedPort() override;
    void sendMessage(const QByteArray& data) override;
    void closePort() override;
    bool isConnected() override;

private:
    RtMidiOut* _midiOut;
    QString _selectedPort;
};

#endif
