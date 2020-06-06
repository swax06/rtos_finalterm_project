#include "opencv2/opencv.hpp"     // Basic OpenCV structures (cv::Mat)
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h> 
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <sys/ioctl.h>
#include <net/if.h>


using namespace cv;

static const pa_sample_spec ss = {
	.format = PA_SAMPLE_S16LE,
	.rate = 8000,
	.channels = 1
};

int capDev = 0;
VideoCapture cap(capDev); // open the default camera

int sd, vsd, ssd;
bool inCall = false, state = false, video = false;
int myRead(int sd, char *buff) {
	int i = 0;
	char c;
	while(i < 256){
		read(sd, &c, 1);
		buff[i] = c;
		i++;
		if(c == '\0'){	
			return 0;
		}
	}
	buff[255] = '\0';
	return 0;
}
void handle_sigint(int sig) { 
    // printf("Caught signal %d\n", sig); 
	if(sig == 2 && !inCall){
		write(sd, "-exit\0", 6);
		close(sd);
		close(ssd);
		close(vsd);
		system("clear");
		exit(0);
	}
	if(sig == 2 && inCall){
		write(sd, "-exit\0", 6);
		inCall = false;
		video = false;
	}
} 

void* s_inp(void *inp) {
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
	return inp;
}

void* s_out(void *inp) {
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
	return inp;
}

void* v_out(void *inp) {
	Mat img;
	img = Mat::zeros(480 , 640, CV_8UC3);    
	int imgSize = img.total() * img.elemSize();
	uchar *iptr = img.data;
	int bytes = 0;
	int key;

	//make img continuos
	if ( ! img.isContinuous() ) { 
		img = img.clone();
	}
		
	printf("Image Size: %d\n", imgSize);
	namedWindow("CV Video Client", 1);
	while(!inCall);
	while (inCall) {
        if ((bytes = recv(vsd, iptr, imgSize , 0)) == -1) {
            std::cerr << "recv failed, received bytes = " << bytes << std::endl;
        }
        cv::imshow("CV Video Client", img); 
      	cv::waitKey(10);
    } 
	// close(vsd);
	return inp;
}

void* v_inp(void *inp) {
	Mat img, flippedFrame, imgGray;
	int height = cap.get(CAP_PROP_FRAME_HEIGHT);
    int width = cap.get(CAP_PROP_FRAME_WIDTH);
	img = Mat::zeros(height, width, CV_8UC3);
    int bytes = 0;
	uchar *buff;
    // make img continuos
    if(!img.isContinuous()){ 
        img = img.clone();
    }
	int codec = cv::VideoWriter::fourcc('H', '2', '6', '4');
    cap.set(CAP_PROP_FOURCC, codec);

	int imgSize = img.total() * img.elemSize();
	while(!inCall);
    while(inCall){
        // get a frame from the camera
        cap >> img;
		// cv::imencode(".jpg", img, buff, cv::IMWRITE_JPEG_QUALITY, 85);
		cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);
        flip(imgGray, flippedFrame, 1);
        if ((bytes = send(vsd, flippedFrame.data, imgSize, 0)) < 0){
            std::cerr << "bytes = " << bytes << std::endl;
            break;
        }
    }
	return inp;
}

void* reader(void *inp) {
	char buff[1024];
	pthread_t thread_id_s[2], thread_id_v[2];
	while(1){
		myRead(sd, buff);
		printf("%s\n", buff);
		if(strcmp(buff, "-connecting") == 0) {
			pthread_create(&thread_id_s[0], NULL, s_out, &ssd);
			pthread_create(&thread_id_s[1], NULL, s_inp, &ssd);
			if(video) {
				pthread_create(&thread_id_v[0], NULL, v_out, &vsd);
				pthread_create(&thread_id_v[1], NULL, v_inp, &vsd);
			}
			inCall = true;
		}
		if(strcmp(buff, "-video") == 0) {
			video = true;
		}
		if(strcmp(buff, "-incoming call") == 0){
			printf("enter -yes to start\nenter -no to stop\n");
		}

		if(strcmp(buff, "-call ended") == 0){
			inCall = false;
			video = false;
		}
		
		if(strcmp(buff, "-server_c") == 0){
			close(sd);
			exit(0);
		}
	}
	return inp;
}



int main(int argc, char **argv){
	signal(SIGINT, handle_sigint); 
	struct sockaddr_in server, server_v;
	int msgLength;
	char buff[256];
	char result;
	pthread_t thread_id, thread_id_s, thread_id_v;

	// connection establishment
	if ((sd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		printf("socket() failed\n");
	}
	// sd = socket(AF_INET,SOCK_STREAM,0);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=inet_addr(argv[1]);
	server.sin_port=htons(atoi(argv[2]));
	// connect(sd,(struct sockaddr *)&server,sizeof(server));
	if (connect(sd,(struct sockaddr *)&server,sizeof(server)) < 0) {
        printf("connect() failed!\n");
    }
	// for ssd
	if ((ssd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket() failed\n");
	}
	if (connect(ssd,(struct sockaddr *)&server,sizeof(server)) < 0) {
		printf("connect() failed!\n");
	}
	// for vsd
	server_v.sin_family=AF_INET;
	server_v.sin_addr.s_addr=inet_addr(argv[1]);
	server_v.sin_port=htons(atoi(argv[3]));
	if ((vsd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket() failed\n");
	}
	if (connect(vsd,(struct sockaddr *)&server_v,sizeof(server_v)) < 0) {
		printf("connect() failed!\n");
	}
	printf("Enter your name:\n");
	pthread_create(&thread_id, NULL, reader, &sd);
	
	while(1) {
		// fflush(stdin); 
		scanf("%255[^\n]", buff); 
		getchar();
		write(sd, buff, strlen(buff));
		write(sd, "\0", 1);

		if(strcmp(buff, "-exit") == 0){
			if(!inCall){
				close(sd);
				break;
			}
			inCall = false;
		}

		// if(strcmp(buff, "-yes") == 0){
		// 	state = true;
		// }

		// if(state){
		// 	pthread_create(&thread_id_s, NULL, s_inp, &ssd);
		// 	if(video) {
		// 		pthread_create(&thread_id_v, NULL, v_inp, &vsd);
		// 	}
		// 	state = false;
		// }

		if(strcmp(buff, "-call") == 0) {
			if(inCall){
				printf("Already in call");
				continue;
			}
		}

		if(strcmp(buff, "-video") == 0){
			if(inCall){
				printf("Already in call");
				continue;
			}
			video = true;
		}
	}
	return 0;
}
