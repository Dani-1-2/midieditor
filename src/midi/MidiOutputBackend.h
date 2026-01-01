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

#ifndef MIDIOUTPUTBACKEND_H_
#define MIDIOUTPUTBACKEND_H_

#include <QByteArray>
#include <QString>
#include <QStringList>

/**
 * Abstract interface for MIDI output backends.
 * Implementations include RtMidiBackend (hardware MIDI) and FluidSynthBackend (software synthesis).
 */
class MidiOutputBackend {
public:
    virtual ~MidiOutputBackend() = default;

    /**
     * Get list of available output ports for this backend.
     * @return List of port names
     */
    virtual QStringList availablePorts() = 0;

    /**
     * Select an output port by name.
     * @param portName Name of the port to select
     * @return true if successful, false otherwise
     */
    virtual bool selectPort(QString portName) = 0;

    /**
     * Get the currently selected port name.
     * @return Port name, or empty string if not connected
     */
    virtual QString selectedPort() = 0;

    /**
     * Send raw MIDI message bytes.
     * @param data Raw MIDI message data
     */
    virtual void sendMessage(const QByteArray& data) = 0;

    /**
     * Close the current port/connection.
     */
    virtual void closePort() = 0;

    /**
     * Check if currently connected to a port.
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() = 0;
};

#endif
