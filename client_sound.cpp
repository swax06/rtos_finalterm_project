#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

bool inCall = false;

static const pa_sample_spec ss = {
	.format = PA_SAMPLE_S16LE,
	.rate = 8000,
	.channels = 1
};

void* sInp(void *inp) {
    int ssd = *(int *)inp;
	pa_simple *s_read;
	char buff[256];
	int error;
	while(!inCall);
	if (!(s_read = pa_simple_new(NULL, "VoIP" , PA_STREAM_RECORD , NULL, "record", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    }
	while(inCall){
		if(pa_simple_read(s_read,buff,sizeof(buff),&error)<0){
			fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
		}
		write(ssd, buff, sizeof(buff));
	}
	pa_simple_free(s_read);
	return inp;
}

void* sOut(void *inp) {
    int ssd = *(int *)inp;
	char buff[256];
	int error;
	pa_simple *s_write;
	while(!inCall);
	if (!(s_write = pa_simple_new(NULL, "VoIP", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    }
	while(inCall){
		read(ssd, buff, sizeof(buff));
		if(pa_simple_write(s_write,buff,sizeof(buff),&error)<0){
			fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
		}
	}
	pa_simple_free(s_write);
	return inp;
}