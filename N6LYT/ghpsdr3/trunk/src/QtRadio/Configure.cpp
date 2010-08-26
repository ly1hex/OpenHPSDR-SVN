/*
 * File:   Configure.cpp
 * Author: john
 *
 * Created on 16 August 2010, 20:03
 */

#include <QSettings>
#include <QComboBox>

#include "Configure.h"

Configure::Configure() {
    widget.setupUi(this);

    widget.sampleRateComboBox->addItem("8000");
    widget.sampleRateComboBox->addItem("48000");
    widget.audioChannelsSpinBox->setValue(1);
    widget.hostComboBox->addItem("127.0.0.1");
    widget.hostComboBox->addItem("192.168.1.82");
    widget.spectrumHighSpinBox->setValue(-40);
    widget.spectrumLowSpinBox->setValue(-160);
    widget.waterfallHighSpinBox->setValue(-60);
    widget.waterfallLowSpinBox->setValue(-120);
    widget.fpsSpinBox->setValue(15);
    widget.encodingComboBox->addItem("aLaw");
    widget.encodingComboBox->addItem("16 bit pcm");
    widget.byteOrderComboBox->addItem("LittleEndian");
    widget.byteOrderComboBox->addItem("BigEndian");
    widget.byteOrderComboBox->setCurrentIndex(0);

    connect(widget.spectrumHighSpinBox,SIGNAL(valueChanged(int)),this,SLOT(slotSpectrumHighChanged(int)));
    connect(widget.spectrumLowSpinBox,SIGNAL(valueChanged(int)),this,SLOT(slotSpectrumLowChanged(int)));
    connect(widget.fpsSpinBox,SIGNAL(valueChanged(int)),this,SLOT(slotFpsChanged(int)));
    connect(widget.waterfallHighSpinBox,SIGNAL(valueChanged(int)),this,SLOT(slotWaterfallHighChanged(int)));
    connect(widget.waterfallLowSpinBox,SIGNAL(valueChanged(int)),this,SLOT(slotWaterfallLowChanged(int)));

    connect(widget.audioDeviceComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(slotAudioDeviceChanged(int)));
    connect(widget.audioChannelsSpinBox,SIGNAL(valueChanged(int)),this,SLOT(slotChannelsChanged(int)));
    connect(widget.byteOrderComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(slotByteOrderChanged(int)));
    connect(widget.sampleRateComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(slotSampleRateChanged(int)));

    connect(widget.hostComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(slotHostChanged(int)));
    connect(widget.rxSpinBox,SIGNAL(valueChanged(int)),this,SLOT(slotReceiverChanged(int)));

}

Configure::~Configure() {
}

void Configure::initAudioDevices(Audio* audio) {
    audio->get_audio_devices(widget.audioDeviceComboBox);
}

void Configure::connected(bool state) {
    // set configuration options enabled/disabled based on connection state
    widget.audioDeviceComboBox->setDisabled(state);
    widget.sampleRateComboBox->setDisabled(state);
    widget.audioChannelsSpinBox->setDisabled(state);
    widget.encodingComboBox->setDisabled(state);
    widget.byteOrderComboBox->setDisabled(state);

    widget.hostComboBox->setDisabled(state);
    widget.rxSpinBox->setDisabled(state);
}

void Configure::loadSettings(QSettings* settings) {
    int i;

    settings->beginGroup("Servers");
    if(settings->contains("entries")) {
        widget.hostComboBox->clear();
        int entries=settings->value("entries").toInt();
        for(i=0;i<entries;i++) {
            widget.hostComboBox->addItem(settings->value(QString::number(i)).toString());
        }
        widget.hostComboBox->setCurrentIndex(settings->value("selected").toInt());
    }
    qDebug() << "server count=" << widget.hostComboBox->count();
    qDebug() << "server selected: " << widget.hostComboBox->currentIndex();

    if(settings->contains("rx")) widget.rxSpinBox->setValue(settings->value("rx").toInt());
    settings->endGroup();
    settings->beginGroup("Display");
    if(settings->contains("spectrumHigh"))widget.spectrumHighSpinBox->setValue(settings->value("spectrumHigh").toInt());
    if(settings->contains("spectrumLow"))widget.spectrumLowSpinBox->setValue(settings->value("spectrumLow").toInt());
    if(settings->contains("fps"))widget.fpsSpinBox->setValue(settings->value("fps").toInt());
    if(settings->contains("waterfallHigh"))widget.waterfallHighSpinBox->setValue(settings->value("waterfallHigh").toInt());
    if(settings->contains("waterfallLow"))widget.waterfallLowSpinBox->setValue(settings->value("waterfallLow").toInt());
    settings->endGroup();
    settings->beginGroup("Audio");
    if(settings->contains("device")) widget.audioDeviceComboBox->setCurrentIndex(settings->value("device").toInt());
    if(settings->contains("channels"))widget.audioChannelsSpinBox->setValue(settings->value("channels").toInt());
    if(settings->contains("samplerate")) widget.sampleRateComboBox->setCurrentIndex(settings->value("samplerate").toInt());
    if(settings->contains("byteorder")) widget.byteOrderComboBox->setCurrentIndex(settings->value("byteorder").toInt());
    settings->endGroup();
}

void Configure::saveSettings(QSettings* settings) {
    int i;
    settings->beginGroup("Servers");
    qDebug() << "server count=" << widget.hostComboBox->count();
    settings->setValue("entries",widget.hostComboBox->count());
    for(i=0;i<widget.hostComboBox->count();i++) {
        qDebug() << "server: " << widget.hostComboBox->itemText(i);
        settings->setValue(QString::number(i),widget.hostComboBox->itemText(i));
    }
    settings->setValue("selected",widget.hostComboBox->currentIndex());
    qDebug() << "server selected: " << widget.hostComboBox->currentIndex();
    settings->setValue("rx",widget.rxSpinBox->value());
    settings->endGroup();
    settings->beginGroup("Display");
    settings->setValue("spectrumHigh",widget.spectrumHighSpinBox->value());
    settings->setValue("spectrumLow",widget.spectrumLowSpinBox->value());
    settings->setValue("fps",widget.fpsSpinBox->value());
    settings->setValue("waterfallHigh",widget.waterfallHighSpinBox->value());
    settings->setValue("waterfallLow",widget.waterfallLowSpinBox->value());
    settings->endGroup();
    settings->beginGroup("Audio");
    settings->setValue("device",widget.audioDeviceComboBox->currentIndex());
    settings->setValue("channels",widget.audioChannelsSpinBox->value());
    settings->setValue("samplerate",widget.sampleRateComboBox->currentIndex());
    settings->setValue("byteorder",widget.byteOrderComboBox->currentIndex());
    settings->endGroup();
}

void Configure::slotHostChanged(int selection) {
    emit hostChanged(widget.hostComboBox->currentText());
}

void Configure::slotReceiverChanged(int receiver) {
    emit receiverChanged(receiver);
}

void Configure::slotSpectrumHighChanged(int high) {
    emit spectrumHighChanged(high);
}

void Configure::slotSpectrumLowChanged(int low) {
    emit spectrumLowChanged(low);
}

void Configure::slotFpsChanged(int fps) {
    emit fpsChanged(fps);
}

void Configure::slotWaterfallHighChanged(int high) {
    emit waterfallHighChanged(high);
}

void Configure::slotWaterfallLowChanged(int low) {
    emit waterfallLowChanged(low);
}

void Configure::slotAudioDeviceChanged(int selection) {
    emit audioDeviceChanged(widget.audioDeviceComboBox->itemData(selection).value<QAudioDeviceInfo >(),
                            widget.sampleRateComboBox->currentText().toInt(),
                            widget.audioChannelsSpinBox->value(),
                            widget.byteOrderComboBox->currentText()=="LittleEndian"?QAudioFormat::LittleEndian:QAudioFormat::BigEndian
                            );
}

void Configure::slotSampleRateChanged(int selection) {
    emit audioDeviceChanged(widget.audioDeviceComboBox->itemData(widget.audioDeviceComboBox->currentIndex()).value<QAudioDeviceInfo >(),
                            widget.sampleRateComboBox->currentText().toInt(),
                            widget.audioChannelsSpinBox->value(),
                            widget.byteOrderComboBox->currentText()=="LittleEndian"?QAudioFormat::LittleEndian:QAudioFormat::BigEndian
                            );
}

void Configure::slotChannelsChanged(int channels) {
    emit audioDeviceChanged(widget.audioDeviceComboBox->itemData(widget.audioDeviceComboBox->currentIndex()).value<QAudioDeviceInfo >(),
                            widget.sampleRateComboBox->currentText().toInt(),
                            widget.audioChannelsSpinBox->value(),
                            widget.byteOrderComboBox->currentText()=="LittleEndian"?QAudioFormat::LittleEndian:QAudioFormat::BigEndian
                            );
}

void Configure::slotByteOrderChanged(int selection) {
    emit audioDeviceChanged(widget.audioDeviceComboBox->itemData(widget.audioDeviceComboBox->currentIndex()).value<QAudioDeviceInfo >(),
                            widget.sampleRateComboBox->currentText().toInt(),
                            widget.audioChannelsSpinBox->value(),
                            widget.byteOrderComboBox->currentText()=="LittleEndian"?QAudioFormat::LittleEndian:QAudioFormat::BigEndian
                            );
}




QString Configure::getHost() {
    return widget.hostComboBox->currentText();
}

int Configure::getReceiver() {
    return widget.rxSpinBox->value();
}

int Configure::getSpectrumHigh() {
    return widget.spectrumHighSpinBox->value();
}

int Configure::getSpectrumLow() {
    return widget.spectrumLowSpinBox->value();
}

int Configure::getFps() {
    return widget.fpsSpinBox->value();
}

int Configure::getWaterfallHigh() {
    return widget.waterfallHighSpinBox->value();
}

int Configure::getWaterfallLow() {
    return widget.waterfallLowSpinBox->value();
}

QAudioFormat::Endian Configure::getByteOrder() {
    QAudioFormat::Endian order=QAudioFormat::LittleEndian;

    switch(widget.byteOrderComboBox->currentIndex()) {
    case 0:
        order=QAudioFormat::LittleEndian;
        break;
    case 1:
        order=QAudioFormat::BigEndian;
        break;

    }

    qDebug() << "getByteOrder: " << widget.byteOrderComboBox->currentIndex() << widget.byteOrderComboBox->currentText() << " order:" << order;
    return order;
}

int Configure::getChannels() {
    return widget.audioChannelsSpinBox->value();
}

int Configure::getSampleRate() {
    return widget.sampleRateComboBox->currentText().toInt();
}
