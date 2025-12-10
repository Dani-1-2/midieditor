#include "Metronome.h"

#include "MidiFile.h"

#include <QtCore/qmath.h>
#include <QFile>
#include <QFileInfo>

#include <QMediaPlayer>
#include <QAudioOutput>

Metronome *Metronome::_instance = nullptr;
bool Metronome::_enable = false;

Metronome::Metronome(QObject *parent) :	QObject(parent) {
    _file = 0;
    num = 4;
    denom = 2;
    _player = new QMediaPlayer(this);
    _audioOutput = new QAudioOutput(this);
    _player->setAudioOutput(_audioOutput);
    _audioOutput->setVolume(1.0);

    // Try to load the metronome sound file if it exists
    QString metronomeFile = QFileInfo("metronome/metronome-01.wav").absoluteFilePath();
    if (QFile::exists(metronomeFile)) {
        _player->setSource(QUrl::fromLocalFile(metronomeFile));
    }
}

void Metronome::setFile(MidiFile *file){
    _file = file;
}

void Metronome::measureUpdate(int measure, int tickInMeasure){

    // compute pos
    if(!_file){
        return;
    }

    int ticksPerClick = (_file->ticksPerQuarter()*4)/qPow(2, denom);
    int pos = tickInMeasure / ticksPerClick;

    if(lastMeasure < measure){
        click();
        lastMeasure = measure;
        lastPos = 0;
        return;
    } else {
        if(pos > lastPos){
            click();
            lastPos = pos;
            return;
        }
    }
}

void Metronome::meterChanged(int n, int d){
    num = n;
    denom = d;
}

void Metronome::playbackStarted(){
    reset();
}

void Metronome::playbackStopped(){

}

Metronome *Metronome::instance(){
    if (!_instance) {
        _instance = new Metronome();
    }
    return _instance;
}

void Metronome::reset(){
    lastPos = 0;
    lastMeasure = -1;
}

void Metronome::click(){

    if(!enabled()){
        return;
    }
    _player->play();
}

bool Metronome::enabled(){
    return _enable;
}

void Metronome::setEnabled(bool b){
    _enable = b;
}

void Metronome::setLoudness(int value){
    if (_instance && _instance->_audioOutput) {
        _instance->_audioOutput->setVolume(value / 100.0);
    }
}

int Metronome::loudness(){
    if (_instance && _instance->_audioOutput) {
        return (int)(_instance->_audioOutput->volume() * 100);
    }
    return 100;
}
