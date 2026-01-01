#include "RtMidiBackend.h"
#include "rtmidi/RtMidi.h"
#include <vector>

RtMidiBackend::RtMidiBackend()
    : _midiOut(nullptr)
    , _selectedPort("")
{
    try {
        _midiOut = new RtMidiOut(RtMidi::UNSPECIFIED, QString("MidiEditor output").toStdString());
    } catch (RtMidiError& error) {
        error.printMessage();
    }
}

RtMidiBackend::~RtMidiBackend()
{
    if (_midiOut) {
        delete _midiOut;
        _midiOut = nullptr;
    }
}

QStringList RtMidiBackend::availablePorts()
{
    QStringList ports;

    if (!_midiOut) {
        return ports;
    }

    unsigned int nPorts = _midiOut->getPortCount();

    for (unsigned int i = 0; i < nPorts; i++) {
        try {
            ports.append(QString::fromStdString(_midiOut->getPortName(i)));
        } catch (RtMidiError&) {
        }
    }

    return ports;
}

bool RtMidiBackend::selectPort(QString portName)
{
    if (!_midiOut) {
        return false;
    }

    // Try to find the port
    unsigned int nPorts = _midiOut->getPortCount();

    for (unsigned int i = 0; i < nPorts; i++) {
        try {
            // If the current port has the given name, select it and close current port
            if (_midiOut->getPortName(i) == portName.toStdString()) {
                _midiOut->closePort();
                _midiOut->openPort(i);
                _selectedPort = portName;
                return true;
            }
        } catch (RtMidiError&) {
        }
    }

    // Port not found
    return false;
}

QString RtMidiBackend::selectedPort()
{
    return _selectedPort;
}

void RtMidiBackend::sendMessage(const QByteArray& data)
{
    if (!_midiOut || _selectedPort.isEmpty()) {
        return;
    }

    // Convert data to std::vector
    std::vector<unsigned char> message;
    foreach (char byte, data) {
        message.push_back(byte);
    }

    try {
        _midiOut->sendMessage(&message);
    } catch (RtMidiError& error) {
        error.printMessage();
    }
}

void RtMidiBackend::closePort()
{
    if (_midiOut) {
        _midiOut->closePort();
        _selectedPort = "";
    }
}

bool RtMidiBackend::isConnected()
{
    return !_selectedPort.isEmpty();
}
