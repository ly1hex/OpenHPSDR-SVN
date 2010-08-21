/** 
* @file client.c
* @brief client network interface
* @author John Melton, G0ORX/N6LYT, Doxygen Comments Dave Larsen, KV0S
* @version 0.1
* @date 2009-04-12
*/
// client.c

/* Copyright (C) 
* 2009 - John Melton, G0ORX/N6LYT, Doxygen Comments Dave Larsen, KV0S
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
* 
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#else
#include "pthread.h"
#include "semaphore.h"
#endif

#include "client.h"
#include "ozy.h"
#include "audiostream.h"
#include "main.h"
#include "soundcard.h"
#include "dttsp.h"
#include "buffer.h"

static pthread_t client_thread_id;

static int client_terminate=0;

#define BASE_PORT 8000

static int port=BASE_PORT;

static int serverSocket;
static int clientSocket;
static struct sockaddr_in server;
static struct sockaddr_in client;

#ifdef __linux__
static socklen_t addrlen;
#else
int addrlen;
#endif

#define SAMPLE_BUFFER_SIZE 4096
static float spectrumBuffer[SAMPLE_BUFFER_SIZE];

int audio_buffer_size;
unsigned char* audio_buffer;
int send_audio=0;

static float meter;

static sem_t network_semaphore;

void* client_thread(void* arg);

void client_send_samples(int size);
void client_set_samples(float* samples,int size);

#define PREFIX 48
static unsigned char* client_samples;


float getFilterSizeCalibrationOffset() {
    int size=1024; // dspBufferSize
    float i=log10((double)size);
    return 3.0f*(11.0f-i);
}

/**
 * - Set port = 8000 + receiver
 * - Set clientSocket = -1
 * - Launch client_thread
 */
void client_init(int receiver) {
    int rc;

    sem_init(&network_semaphore,0,1);

#ifdef __linux__
    signal(SIGPIPE, SIG_IGN);
#endif

    audio_buffer_size=2000;
    audio_buffer=malloc(audio_buffer_size+PREFIX);

fprintf(stderr,"client_init audio_buffer_size=%d audio_buffer=%ld\n",audio_buffer_size,audio_buffer);

    port=BASE_PORT+receiver;
    clientSocket=-1;
    rc=pthread_create(&client_thread_id,NULL,client_thread,NULL);
    if(rc != 0) {
        fprintf(stderr,"pthread_create failed on client_thread: rc=%d\n", rc);
    }

}

/**
 * - create TCP serverSocket, 
 * - listen for connection request,
 * - accept any connection request & create a client_socket on which to receive commands from the GUI (eg jmonitor)
 * - Read commands and parse them.  Commands recognized are:
 * - getSpectrum <Number of samples>
 * - setFrequency <integer frequency in Hz>
 * - setMode <integer mode>
 * - setFilter <integer low> <integer high>
 * - setAGC <integer>
 * - setNR  true  or  false
 * - setNB  true  or  false
 * - setANF  true  or  false
 * - SetRXOutputGain <integer 0 to 100>
 * - startAudioStream <integer number of samples>  (if number is omitted, 480 is used)
 * - stopAudioStream
 */
void* client_thread(void* arg) {
    int rc;
    char *token;
    int bytesRead;
    char message[32];
    int on=1;

fprintf(stderr,"client_thread\n");

    serverSocket=socket(AF_INET,SOCK_STREAM,0);
    if(serverSocket==-1) {
        perror("client socket");
        return NULL;
    }

    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&server,0,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_port=htons(port);

    if(bind(serverSocket,(struct sockaddr *)&server,sizeof(server))<0) {
        perror("client bind");
        return NULL;
    }

fprintf(stderr,"client_thread: listening on port %d\n",port);
    if (listen(serverSocket, 5) == -1) {
        perror("client listen");
        exit(1);
    }

    while(1) 
	{
        addrlen = sizeof(client); 
		if ((clientSocket = accept(serverSocket,(struct sockaddr *)&client,&addrlen)) == -1) 
		{
			perror("client accept");
		} else 
		{	struct timeval tv;
            time_t tt;
            struct tm *tod;
            time(&tt);
            tod=localtime(&tt);
            //fprintf(stdout,"wget -O - http://api.hostip.info/get_html.php?ip=%s\n",inet_ntoa(client.sin_addr));
            fflush(stdout);
            fprintf(stderr,"%02d/%02d/%02d %02d:%02d:%02d RX%d: client connection from %s:%d\n",tod->tm_mday,tod->tm_mon+1,tod->tm_year+1900,tod->tm_hour,tod->tm_min,tod->tm_sec,receiver,inet_ntoa(client.sin_addr),ntohs(client.sin_port));


            // set timeout on receive
            
            tv.tv_sec=3;
            tv.tv_usec=0;
            rc=setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof tv);

            client_terminate=0;
            while(client_terminate==0) {
                bytesRead=recv(clientSocket, message, sizeof(message), 0);
                if(bytesRead==0) {
                    break;
                }
         
#ifndef __linux__
#define EWOULDBLOCK	WSAEWOULDBLOCK
#endif
                if(bytesRead<0) {
                    if(errno!=EWOULDBLOCK) {
                        continue;
                    }
                    break;
                }

                message[bytesRead]=0;

// print commands other than getSpectrum, which jmonitor sends regularly and frequently
	if (strstr(message, "getSpectrum") == NULL) fprintf(stderr,"client message: %s\n",message);

                token=strtok(message," ");
                    if(token!=NULL) {
                    if(strcmp(token,"getSpectrum")==0) {
                        int samples;
                        token=strtok(NULL," ");
                        if(token!=NULL) {
                            samples=atoi(token);
                            Process_Panadapter(0,spectrumBuffer);
                            meter=CalculateRXMeter(0,0,0)+multimeterCalibrationOffset+getFilterSizeCalibrationOffset();
                            client_samples=malloc(PREFIX+samples);
                            client_set_samples(spectrumBuffer,samples);
                            client_send_samples(samples);
                            free(client_samples);
                        } else {
                            fprintf(stderr,"Invalid command: '%s'\n",message);
                        }
                    } else if(strcmp(token,"setFrequency")==0) {
#ifdef __linux__
						long long frequency;
#else
                        long frequency;
#endif
                        token=strtok(NULL," ");
                        if(token!=NULL) {
#ifdef __linux__
							frequency=atoll(token);
#else
                            frequency=atol(token);
#endif
                            ozySetFrequency(frequency);
                        } else {
                            fprintf(stderr,"Invalid command: '%s'\n",message);
                        }
					} else if (strcmp(token, "selectAudio") == 0) {
						ozySelectAudio();
					}
                      else if(strcmp(token,"setMode")==0) {
                        int mode;
                        token=strtok(NULL," ");
                        if(token!=NULL) {
                            mode=atoi(token);
                            SetMode(0,0,mode);
                        } else {
                            fprintf(stderr,"Invalid command: '%s'\n",message);
                        }
                    } else if(strcmp(token,"setFilter")==0) {
                        int low,high;
                        token=strtok(NULL," ");
                        if(token!=NULL) {
                            low=atoi(token);
                            token=strtok(NULL," ");
                            if(token!=NULL) {
                              high=atoi(token);
                              SetRXFilter(0,0,(double)low,(double)high);
                            } else {
                                fprintf(stderr,"Invalid command: '%s'\n",message);
                            }
                        } else {
                            fprintf(stderr,"Invalid command: '%s'\n",message);
                        }
                    } else if(strcmp(token,"setAGC")==0) {
                        int agc;
                        token=strtok(NULL," ");
                        if(token!=NULL) {
                            agc=atoi(token);
                            SetRXAGC(0,0,agc);
                        } else {
                            fprintf(stderr,"Invalid command: '%s'\n",message);
                        }
                    } else if(strcmp(token,"setNR")==0) {
                        int nr;
                        token=strtok(NULL," ");
                        if(token!=NULL) {
                            if(strcmp(token,"true")==0) {
                                nr=1;
                            } else {
                                nr=0;
                            }
                            SetNR(0,0,nr);
                        } else {
                            fprintf(stderr,"Invalid command: '%s'\n",message);
                        }
                    } else if(strcmp(token,"setNB")==0) {
                        int nb;
                        token=strtok(NULL," ");
                        if(token!=NULL) {
                            if(strcmp(token,"true")==0) {
                                nb=1;
                            } else {
                                nb=0;
                            }
                            SetNB(0,0,nb);
                        } else {
                            fprintf(stderr,"Invalid command: '%s'\n",message);
                        }
                    } else if(strcmp(token,"setANF")==0) {
                        int anf;
                        token=strtok(NULL," ");
                        if(token!=NULL) {
                            if(strcmp(token,"true")==0) {
                                anf=1;
                            } else {
                                anf=0;
                            }
                            SetANF(0,0,anf);
                        } else {
                            fprintf(stderr,"Invalid command: '%s'\n",message);
                        }
                    } else if(strcmp(token,"SetRXOutputGain")==0) {
                        int gain;
                        token=strtok(NULL," ");
                        if(token!=NULL) {
                            gain=atoi(token);
                            SetRXOutputGain(0,0,(double)gain/100.0);
                        } else {
                            fprintf(stderr,"Invalid command: '%s'\n",message);
                        }
                    } else if(strcmp(token,"startAudioStream")==0) {
                        token=strtok(NULL," ");
                        if(token==NULL) {
                            audio_buffer_size=480;
                        } else {
                            audio_buffer_size=atoi(token);
                        }
                        free(audio_buffer);
                        audio_buffer=malloc(audio_buffer_size+PREFIX);
                        audio_stream_reset();
                        send_audio=1;
fprintf(stderr,"startAudioStream %d send_audio=%d\n",audio_buffer_size,send_audio);
                    } else if(strcmp(token,"stopAudioStream")==0) {
                        send_audio=0;
fprintf(stderr,"stopAudioStream send_audio=%d\n",send_audio);
                    } else {
                        fprintf(stderr,"Invalid command: '%s'\n",message);
                    }
                } else {
                    fprintf(stderr,"Invalid command: '%s'\n",message);
                }
            }
#ifdef __linux__
			close(clientSocket);
#else
            closesocket(clientSocket);
#endif
            ozyDisconnect();
            time(&tt);
            tod=localtime(&tt);
            fprintf(stderr,"%02d/%02d/%02d %02d:%02d:%02d RX%d: client disconnected from %s:%d\n",tod->tm_mday,tod->tm_mon+1,tod->tm_year+1900,tod->tm_hour,tod->tm_min,tod->tm_sec,receiver,inet_ntoa(client.sin_addr),ntohs(client.sin_port));
        }
        send_audio=0;
        clientSocket=-1;
//fprintf(stderr,"client disconnected send_audio=%d\n",send_audio);

    }
}

/**
 * If client_socket has been changed from initial value -1,  send client_samples on clientSocket.
 * \param size int number of samples.  Samples are a single byte each.  They're used for the spectrum and waterfall displays.
 */
void client_send_samples(int size) {
    int rc;
    if(clientSocket!=-1) {
        sem_wait(&network_semaphore);

#ifdef __linux__
            rc=send(clientSocket,client_samples,size+PREFIX,MSG_NOSIGNAL);
#else
            rc=send(clientSocket,client_samples,size+PREFIX, 0);	// Windows doesn't know MSG_NOSIGNAL
#endif
            if(rc<0) {
                // perror("client_send_samples failed");
            }
        sem_post(&network_semaphore);
    } else {
        fprintf(stderr,"client_send_samples: clientSocket==-1\n");
    }
}

/**
 * If client_socket has been changed from initial value -1, send audio_buffer on clientSocket.
 * The audio samples are a single byte each, and have been alaw-encoded.
 */
void client_send_audio() {
    int rc;
        if(clientSocket!=-1) {
            sem_wait(&network_semaphore);
                if(send_audio && (clientSocket!=-1)) {
#ifdef __linux__
                    rc=send(clientSocket,audio_buffer,audio_buffer_size+PREFIX,MSG_NOSIGNAL);
#else
                    rc=send(clientSocket,audio_buffer,audio_buffer_size+PREFIX, 0);		// Windoes doesn't know MSG_NOSIGNAL
#endif
                    if(rc!=(audio_buffer_size+PREFIX)) {
                       // fprintf(stderr,"client_send_audio sent %d bytes",rc);
                    }
                }
            sem_post(&network_semaphore);
        } else {
            //fprintf(stderr,"client_send_audio: clientSocket==-1\n");
        }
}

/**
 * Fill client_samples[] with values from param samples.
 * The first byte of client_samples[] is set as a flag = SPECTRUM_BUFFER to indicate contents to the client.
 * The sample rate and S-meter reading are placed in the buffer before the spectrum values.
 * Space has been left to send frequency, high & low filter cutoff frequencies, and demodulation mode, but
 * at present, these locations are left uninitialized.
 * \param samples float array
 * \param size int Number of spectrum samples to be placed in client_samples[].
 * size is usually smaller than SAMPLE_BUFFER_SIZE, so the largest of several adjacent samples is
 * used to determine the value for client_samples[].  The value is scaled to fit in a single byte.
 */
void client_set_samples(float* samples,int size) {
    int i,j;
    float slope;
    float max;
    int lindex,rindex;

    // first byte is the buffer type
    client_samples[0]=SPECTRUM_BUFFER;

    // first 14 bytes contain the frequency
    //sprintf(client_samples,"% 4lld.%03lld.%03lld",frequencyA/1000000LL,(frequencyA%1000000LL)/1000LL,frequencyA%1000LL);

    // next 6 bytes contain the filter low
    //sprintf(&client_samples[14],"%d",filterLow);

    // next 6 bytes contain the filter high
    //sprintf(&client_samples[20],"%d",filterHigh);

    // next 6 bytes contain the mode
    //sprintf(&client_samples[26],"%s",modeToString());

    // next 8 bytes contain the sample rate
    sprintf(&client_samples[32],"%d",sampleRate);

    // next 8 bytes contain the meter
    sprintf(&client_samples[40],"%d",(int)meter);

    slope=(float)SAMPLE_BUFFER_SIZE/(float)size;
    for(i=0;i<size;i++) {
        max=-10000.0F;
        lindex=(int)floor((float)i*slope);
        rindex=(int)floor(((float)i*slope)+slope);
        if(rindex>SAMPLE_BUFFER_SIZE) rindex=SAMPLE_BUFFER_SIZE;
        for(j=lindex;j<rindex;j++) {
            if(samples[j]>max) max=samples[j];
        }
        client_samples[i+PREFIX]=(unsigned char)-(max+displayCalibrationOffset+preampOffset);
    }

}