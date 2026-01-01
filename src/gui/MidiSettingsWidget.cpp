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

#include "MidiSettingsWidget.h"

#include "../Terminal.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiInput.h"
#include "../midi/MidiOutput.h"
#include "../midi/Metronome.h"
#include "../midi/FluidSynthBackend.h"
#include <QCheckBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextEdit>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSlider>

AdditionalMidiSettingsWidget::AdditionalMidiSettingsWidget(QSettings* settings, QWidget* parent)
    : SettingsWidget("Additional Midi Settings", parent)
{

    _settings = settings;

    QGridLayout* layout = new QGridLayout(this);
    setLayout(layout);

    layout->addWidget(new QLabel("Default ticks per quarter note:", this), 0, 0, 1, 2);
    _tpqBox = new QSpinBox(this);
    _tpqBox->setMinimum(1);
    _tpqBox->setMaximum(1024);
    _tpqBox->setValue(MidiFile::defaultTimePerQuarter);
    connect(_tpqBox, SIGNAL(valueChanged(int)), this, SLOT(setDefaultTimePerQuarter(int)));
    layout->addWidget(_tpqBox, 0, 2, 1, 4);

    QWidget* tpqInfo = createInfoBox("Note: There aren't many reasons to change this. MIDI files have a resolution for how many ticks can fit in a quarter note. Higher values = more detail. Lower values may be required for compatibility. Only affects new files.");
    layout->addWidget(tpqInfo, 1, 0, 1, 6);

    layout->addWidget(separator(), 2, 0, 1, 6);

    _alternativePlayerModeBox = new QCheckBox("Manually stop notes", this);
    _alternativePlayerModeBox->setChecked(MidiOutput::isAlternativePlayer);

    connect(_alternativePlayerModeBox, SIGNAL(toggled(bool)), this, SLOT(manualModeToggled(bool)));
    layout->addWidget(_alternativePlayerModeBox, 3, 0, 1, 6);

    QWidget* playerModeInfo = createInfoBox("Note: the above option should not be enabled in general. It is only required if the stop button does not stop playback as expected (e.g. when some notes are not stopped correctly).");
    layout->addWidget(playerModeInfo, 4, 0, 1, 6);

    layout->addWidget(separator(), 5, 0, 1, 6);

    layout->addWidget(new QLabel("Metronome loudness:", this), 6, 0, 1, 2);
    _metronomeLoudnessBox = new QSpinBox(this);
    _metronomeLoudnessBox->setMinimum(10);
    _metronomeLoudnessBox->setMaximum(100);
    _metronomeLoudnessBox->setValue(Metronome::loudness());
    connect(_metronomeLoudnessBox, SIGNAL(valueChanged(int)), this, SLOT(setMetronomeLoudness(int)));
    layout->addWidget(_metronomeLoudnessBox, 6, 2, 1, 4);

    layout->addWidget(separator(), 7, 0, 1, 6);

    layout->addWidget(new QLabel("Start command:", this), 8, 0, 1, 2);
    startCmd = new QLineEdit(this);
    layout->addWidget(startCmd, 8, 2, 1, 4);

    QWidget* startCmdInfo = createInfoBox("The start command can be used to start additional software components (e.g. Midi synthesizers) each time, MidiEditor is started. You can see the output of the started software / script in the field below.");
    layout->addWidget(startCmdInfo, 9, 0, 1, 6);

    layout->addWidget(Terminal::terminal()->console(), 10, 0, 1, 6);

    startCmd->setText(_settings->value("start_cmd", "").toString());
    layout->setRowStretch(3, 1);
}

void AdditionalMidiSettingsWidget::manualModeToggled(bool enable)
{
    MidiOutput::isAlternativePlayer = enable;
}

void AdditionalMidiSettingsWidget::setDefaultTimePerQuarter(int value)
{
    MidiFile::defaultTimePerQuarter = value;
}

void AdditionalMidiSettingsWidget::setMetronomeLoudness(int value){
    Metronome::setLoudness(value);
}

bool AdditionalMidiSettingsWidget::accept()
{
    QString text = startCmd->text();
    if (!text.isEmpty()) {
        _settings->setValue("start_cmd", text);
    }
    return true;
}

MidiSettingsWidget::MidiSettingsWidget(QWidget* parent)
    : SettingsWidget("Midi I/O", parent)
{

    QGridLayout* layout = new QGridLayout(this);
    setLayout(layout);

    QWidget* playerModeInfo = createInfoBox("Choose the Midi ports on your machine to which MidiEditor connects in order to play and record Midi data.");
    layout->addWidget(playerModeInfo, 0, 0, 1, 6);

    // output
    layout->addWidget(new QLabel("Midi output: ", this), 1, 0, 1, 2);
    _outList = new QListWidget(this);
    connect(_outList, SIGNAL(itemChanged(QListWidgetItem*)), this,
        SLOT(outputChanged(QListWidgetItem*)));

    layout->addWidget(_outList, 2, 0, 1, 3);
    QPushButton* reloadOutputList = new QPushButton();
    reloadOutputList->setToolTip("Refresh port list");
    reloadOutputList->setFlat(true);
    reloadOutputList->setIcon(QIcon(":/run_environment/graphics/tool/refresh.png"));
    reloadOutputList->setFixedSize(30, 30);
    layout->addWidget(reloadOutputList, 1, 2, 1, 1);
    connect(reloadOutputList, SIGNAL(clicked()), this,
        SLOT(reloadOutputPorts()));
    reloadOutputPorts();

    // input
    layout->addWidget(new QLabel("Midi input: ", this), 1, 3, 1, 2);
    _inList = new QListWidget(this);
    connect(_inList, SIGNAL(itemChanged(QListWidgetItem*)), this,
        SLOT(inputChanged(QListWidgetItem*)));

    layout->addWidget(_inList, 2, 3, 1, 3);
    QPushButton* reloadInputList = new QPushButton();
    reloadInputList->setFlat(true);
    layout->addWidget(reloadInputList, 1, 5, 1, 1);
    reloadInputList->setToolTip("Refresh port list");
    reloadInputList->setIcon(QIcon(":/run_environment/graphics/tool/refresh.png"));
    reloadInputList->setFixedSize(30, 30);
    connect(reloadInputList, SIGNAL(clicked()), this,
        SLOT(reloadInputPorts()));
    reloadInputPorts();

    // Add FluidSynth settings section
    layout->addWidget(separator(), 3, 0, 1, 6);
    createFluidSynthSettings();
    layout->addWidget(_fluidSynthSettingsWidget, 4, 0, 1, 6);
}

void MidiSettingsWidget::reloadInputPorts()
{

    disconnect(_inList, SIGNAL(itemChanged(QListWidgetItem*)), this,
        SLOT(inputChanged(QListWidgetItem*)));

    // clear the list
    _inList->clear();

    foreach (QString name, MidiInput::inputPorts()) {

        QListWidgetItem* item = new QListWidgetItem(name, _inList,
            QListWidgetItem::UserType);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);

        if (name == MidiInput::inputPort()) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
        _inList->addItem(item);
    }
    connect(_inList, SIGNAL(itemChanged(QListWidgetItem*)), this,
        SLOT(inputChanged(QListWidgetItem*)));
}

void MidiSettingsWidget::reloadOutputPorts()
{

    disconnect(_outList, SIGNAL(itemChanged(QListWidgetItem*)), this,
        SLOT(outputChanged(QListWidgetItem*)));

    // clear the list
    _outList->clear();

    foreach (QString name, MidiOutput::outputPorts()) {

        QListWidgetItem* item = new QListWidgetItem(name, _outList,
            QListWidgetItem::UserType);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);

        if (name == MidiOutput::outputPort()) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
        _outList->addItem(item);
    }
    connect(_outList, SIGNAL(itemChanged(QListWidgetItem*)), this,
        SLOT(outputChanged(QListWidgetItem*)));
}

void MidiSettingsWidget::inputChanged(QListWidgetItem* item)
{

    if (item->checkState() == Qt::Checked) {

        MidiInput::setInputPort(item->text());

        reloadInputPorts();
    }
}

void MidiSettingsWidget::outputChanged(QListWidgetItem* item)
{

    if (item->checkState() == Qt::Checked) {

        MidiOutput::setOutputPort(item->text());

        // If FluidSynth selected, ensure soundfont is loaded
        if (item->text() == "FluidSynth (embedded)") {
            QString currentPath = _soundfontPathEdit->text();

            // If no soundfont configured, try to load default
            if (currentPath.isEmpty()) {
                FluidSynthBackend* fs = MidiOutput::fluidSynthBackend();
                if (fs && fs->loadDefaultSoundfont()) {
                    QString defaultPath = fs->currentSoundfont();
                    _soundfontPathEdit->setText(defaultPath);
                    QSettings settings(QString("MidiEditor"), QString("NONE"));
                    settings.setValue("fluidsynth_soundfont", defaultPath);
                }
            }

            soundfontPathChanged();
        }

        reloadOutputPorts();
    }
}

void MidiSettingsWidget::createFluidSynthSettings()
{
    _fluidSynthSettingsWidget = new QWidget(this);
    QGridLayout* fsLayout = new QGridLayout(_fluidSynthSettingsWidget);

    // Title
    QLabel* titleLabel = new QLabel("FluidSynth Settings:", _fluidSynthSettingsWidget);
    QFont boldFont = titleLabel->font();
    boldFont.setBold(true);
    titleLabel->setFont(boldFont);
    fsLayout->addWidget(titleLabel, 0, 0, 1, 3);

    // Info box
    QWidget* infoBox = createInfoBox(
        "FluidSynth is an embedded software synthesizer. "
        "Select a soundfont file (.sf2) to use FluidSynth for MIDI playback. "
        "Then select 'FluidSynth (embedded)' from the output ports above."
    );
    fsLayout->addWidget(infoBox, 1, 0, 1, 3);

    // Soundfont path
    fsLayout->addWidget(new QLabel("Soundfont (.sf2):", _fluidSynthSettingsWidget),
                        2, 0, 1, 1);

    _soundfontPathEdit = new QLineEdit(_fluidSynthSettingsWidget);
    _soundfontPathEdit->setReadOnly(true);
    fsLayout->addWidget(_soundfontPathEdit, 2, 1, 1, 1);

    _soundfontBrowseButton = new QPushButton("Browse...", _fluidSynthSettingsWidget);
    connect(_soundfontBrowseButton, SIGNAL(clicked()), this, SLOT(browseSoundfont()));
    fsLayout->addWidget(_soundfontBrowseButton, 2, 2, 1, 1);

    // Status label
    _soundfontStatusLabel = new QLabel(_fluidSynthSettingsWidget);
    fsLayout->addWidget(_soundfontStatusLabel, 3, 0, 1, 3);

    // Volume control
    fsLayout->addWidget(new QLabel("Volume:", _fluidSynthSettingsWidget), 4, 0, 1, 1);

    _volumeSlider = new QSlider(Qt::Horizontal, _fluidSynthSettingsWidget);
    _volumeSlider->setMinimum(0);
    _volumeSlider->setMaximum(200);  // 0-200 represents 0.0-2.0 gain
    _volumeSlider->setValue(50);     // Default 0.5 gain (50%)
    _volumeSlider->setTickPosition(QSlider::TicksBelow);
    _volumeSlider->setTickInterval(25);
    connect(_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(volumeChanged(int)));
    fsLayout->addWidget(_volumeSlider, 4, 1, 1, 1);

    _volumeLabel = new QLabel("50%", _fluidSynthSettingsWidget);
    _volumeLabel->setMinimumWidth(50);
    fsLayout->addWidget(_volumeLabel, 4, 2, 1, 1);

    _fluidSynthSettingsWidget->setLayout(fsLayout);

    // Load saved settings
    QSettings settings(QString("MidiEditor"), QString("NONE"));
    QString savedPath = settings.value("fluidsynth_soundfont", "").toString();
    if (!savedPath.isEmpty()) {
        _soundfontPathEdit->setText(savedPath);
    } else {
        // Try to load default soundfont
        FluidSynthBackend* fs = MidiOutput::fluidSynthBackend();
        if (fs) {
            QString defaultPath = fs->findDefaultSoundfont();
            if (!defaultPath.isEmpty()) {
                _soundfontPathEdit->setText(defaultPath);
                settings.setValue("fluidsynth_soundfont", defaultPath);
            }
        }
    }

    // Load saved volume
    int savedVolume = settings.value("fluidsynth_volume", 50).toInt();
    _volumeSlider->setValue(savedVolume);

    updateFluidSynthUI();
}

void MidiSettingsWidget::browseSoundfont()
{
    QSettings settings(QString("MidiEditor"), QString("NONE"));
    QString lastDir = settings.value("fluidsynth_soundfont_dir",
                                     QDir::homePath()).toString();

    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Soundfont File",
        lastDir,
        "SoundFont Files (*.sf2 *.SF2);;All Files (*.*)"
    );

    if (!fileName.isEmpty()) {
        _soundfontPathEdit->setText(fileName);

        // Save the directory for next time
        QFileInfo fileInfo(fileName);
        settings.setValue("fluidsynth_soundfont_dir", fileInfo.absolutePath());
        settings.setValue("fluidsynth_soundfont", fileName);

        // Load the soundfont
        soundfontPathChanged();
    }
}

void MidiSettingsWidget::soundfontPathChanged()
{
    QString path = _soundfontPathEdit->text();

    if (path.isEmpty()) {
        _soundfontStatusLabel->setText("");
        return;
    }

    // Check if file exists
    if (!QFile::exists(path)) {
        _soundfontStatusLabel->setText(
            "<font color='red'>Error: File not found</font>"
        );
        return;
    }

    // Try to load the soundfont
    FluidSynthBackend* fs = MidiOutput::fluidSynthBackend();
    if (fs) {
        if (fs->loadSoundfont(path)) {
            _soundfontStatusLabel->setText(
                "<font color='green'>Soundfont loaded successfully</font>"
            );
        } else {
            _soundfontStatusLabel->setText(
                "<font color='red'>Error: " + fs->lastError() + "</font>"
            );
        }
    }
}

void MidiSettingsWidget::updateFluidSynthUI()
{
    soundfontPathChanged();

    // Update volume in backend
    FluidSynthBackend* fs = MidiOutput::fluidSynthBackend();
    if (fs) {
        double gain = _volumeSlider->value() / 100.0;
        fs->setGain(gain);
    }
}

void MidiSettingsWidget::volumeChanged(int value)
{
    // Update label
    _volumeLabel->setText(QString::number(value) + "%");

    // Update FluidSynth gain
    FluidSynthBackend* fs = MidiOutput::fluidSynthBackend();
    if (fs) {
        double gain = value / 100.0;  // Convert 0-200 to 0.0-2.0
        fs->setGain(gain);
    }

    // Save to settings
    QSettings settings(QString("MidiEditor"), QString("NONE"));
    settings.setValue("fluidsynth_volume", value);
}
