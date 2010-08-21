/* 
 * File:   Audio.cpp
 * Author: john
 * 
 * Created on 16 August 2010, 11:19
 */

#include "Audio.h"

Audio::Audio() {
    audio_output=NULL;
    audio_out=NULL;
    sampleRate=8000;
    audio_channels=1;
}

Audio::~Audio() {
}

void Audio::initialize_audio(int buffer_size) {
    qDebug() << "initialize_audio " << buffer_size;

    decoded_buffer.resize(buffer_size*2);

    init_decodetable();

    audio_format.setFrequency(sampleRate);
    audio_format.setChannels(audio_channels);
    audio_format.setSampleSize(16);
    audio_format.setCodec("audio/pcm");
    audio_format.setByteOrder(QAudioFormat::BigEndian);
    audio_format.setSampleType(QAudioFormat::SignedInt);

}

void Audio::get_audio_devices(QComboBox* comboBox) {

    qDebug() << "Audio::get_audio_devices";
    int i=0;
    foreach(const QAudioDeviceInfo &device_info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        qDebug() << "Audio::get_audio_devices: " << device_info.deviceName();

        qDebug() << "    Codecs:";
        QStringList codecs=device_info.supportedCodecs();
        for(int j=0;j<codecs.size();j++) {
            qDebug() << "        " << codecs.at(j).toLocal8Bit().constData();
        }

        qDebug() << "    Byte Orders";
        QList<QAudioFormat::Endian> byteOrders=device_info.supportedByteOrders();
        for(int j=0;j<byteOrders.size();j++) {
            qDebug() << "        " << (byteOrders.at(j)==QAudioFormat::BigEndian?"BigEndian":"LittleEndian");
        }

        qDebug() << "    Sample Type";
        QList<QAudioFormat::SampleType> sampleTypes=device_info.supportedSampleTypes();
        for(int j=0;j<sampleTypes.size();j++) {
            if(sampleTypes.at(j)==QAudioFormat::Unknown) {
                qDebug() << "        Unknown";
            } else if(sampleTypes.at(j)==QAudioFormat::SignedInt) {
                qDebug() << "        SignedInt";
            } else if(sampleTypes.at(j)==QAudioFormat::UnSignedInt) {
                qDebug() << "        UnSignedInt";
            } else if(sampleTypes.at(j)==QAudioFormat::Float) {
                qDebug() << "        Float";
            }
        }

        qDebug() << "    Sample Rates";
        QList<int> sampleRates=device_info.supportedFrequencies();
        for(int j=0;j<sampleRates.size();j++) {
            qDebug() << "        " << sampleRates.at(j);
        }

        qDebug() << "    Sample Sizes";
        QList<int> sampleSizes=device_info.supportedSampleSizes();
        for(int j=0;j<sampleSizes.size();j++) {
            qDebug() << "        " << sampleSizes.at(j);
        }

        comboBox->addItem(device_info.deviceName(),qVariantFromValue(device_info));
        if(i==0) {
            audio_device=device_info;
        }
        i++;
    }

    qDebug() << "Audio::get_audio_devices: default is " << audio_device.deviceName();

    audio_output = new QAudioOutput(audio_device, audio_format, this);
    audio_out = audio_output->start();
}

void Audio::select_audio(QAudioDeviceInfo info,int rate,int channels) {
    qDebug() << "selected audio " << info.deviceName() <<  "sampleRate " << rate;

    sampleRate=rate;
    audio_channels=channels;

    if(audio_output!=NULL) {
        audio_output->stop();
        audio_output->disconnect(this);
        audio_output=NULL;
        audio_out=NULL;
    }



    audio_device=info;
    audio_format.setFrequency(rate);
    audio_format.setChannels(audio_channels);
    audio_output = new QAudioOutput(audio_device, audio_format, this);
    audio_out = audio_output->start();

}

void Audio::process_audio(char* header,char* buffer,int length) {
    //qDebug() << "process audio";
    int written=0;
    aLawDecode(buffer,length);
    if(audio_out) {
        //qDebug() << "writing audio data length=: " <<  decoded_buffer.length();
        while(written<decoded_buffer.length()) {
            written+=audio_out->write(&decoded_buffer.data()[written],decoded_buffer.length()-written);
        }

    }
}

void Audio::aLawDecode(char* buffer,int length) {
    int i;
    short v;

    //qDebug() << "aLawDecode " << decoded_buffer.length();
    decoded_buffer.clear();

    for (i=0; i < length; i++) {
        v=decodetable[buffer[i]&0xFF];

        // assumes BIGENDIAN
        decoded_buffer.append((char)((v>>8)&0xFF));
        decoded_buffer.append((char)(v&0xFF));

    }

}


void Audio::init_decodetable() {
    qDebug() << "init_decodetable";
    for (int i = 0; i < 256; i++) {
        int input = i ^ 85;
        int mantissa = (input & 15) << 4;
        int segment = (input & 112) >> 4;
        int value = mantissa + 8;
        if (segment >= 1) {
            value += 256;
        }
        if (segment > 1) {
            value <<= (segment - 1);
        }
        if ((input & 128) == 0) {
            value = -value;
        }
        decodetable[i] = (short) value;

    }
}

