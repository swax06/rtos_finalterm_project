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
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
// #include "client_video.h"
#include "client_sound.h"
#include <sys/socket.h>
#include <pthread.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "Twofish.h"

sem_t semaphore;
using namespace cv;
int capDev = 0;
VideoCapture cap(capDev);
bool inVideo = false;

int sd, vsd, ssd;

bool video = false;

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
	if(sig == 2 && !inCall){
		write(sd, "-exit\0", 6);
		close(sd);
		close(ssd);
		close(vsd);
		exit(0);
	}
	if(sig == 2 && inCall){
		write(sd, "-exit\0", 6);
		inCall = false;
		inVideo = false;
		printf("-call ended\n");
	}
} 

void* vOut(void *inp) {
	int vsd = *(int *)inp;
	bool status = true;
	cv::Mat img;
    img = cv::Mat::zeros(480 , 640, CV_8UC1);
	int data_size = 15000;
	char data[data_size];
    if ( ! img.isContinuous() ) { 
        img = img.clone();
    }
    cv::namedWindow("CV Video Client", 1);
    std::string str;
	while (!inVideo);
    while (status) {
        read(vsd, data, data_size);
        std::vector<uchar> vec(data_size);
        for (int i = 0; i < data_size; i++) {
            vec[i] = data[i];
        }
        img = cv::imdecode(vec, cv::IMREAD_GRAYSCALE);
		if (!img.data ) {
			std::cout <<  "Could not open or find the image" << std::endl;
			continue;
		}
        cv::imshow("CV Video Client", img);
        cv::waitKey(1);
		sem_wait(&semaphore);
        if(!inVideo) status = false;
        sem_post(&semaphore);
    }
	cv::destroyAllWindows();
	return inp;
}


void* vInp(void *inp) {
	int vsd = *(int *)inp;
	bool status = true;
	cv::Mat img, flippedFrame, imgGray;
	int height = cap.get(CAP_PROP_FRAME_HEIGHT);
    int width = cap.get(CAP_PROP_FRAME_WIDTH);
    img = cv::Mat::zeros(height, width, CV_8UC1);
    int data_size = 15000;
    char data[data_size];
    // make img continuos
    if(!img.isContinuous()){ 
        img = img.clone();
    }
    std::vector<uchar> buff;
    std::vector<int> param(2);
    param[0] = cv::IMWRITE_JPEG_QUALITY;
    param[1] = 20;//default(95) 0-100
	while (!inVideo);
    while(status){
        cap >> img;
        cv::flip(img, flippedFrame, 1);
        cv::cvtColor(flippedFrame, imgGray, cv::COLOR_BGR2GRAY);
        cv::imencode(".jpg", imgGray, buff, param);
        for(int i = 0; i < data_size; i++){
            data[i] = buff[i];
        }
        write(vsd, data, data_size);
		sem_wait(&semaphore);
        if(!inVideo) status = false;
        sem_post(&semaphore);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
	cap.release();
	return inp;
}



void* reader(void *inp) {
	int sd = *(int*) inp;
	char buff[1024];
	pthread_t thread_id_s[2], thread_id_v[2];
	while(1){
		myRead(sd, buff);
		printf("%s\n", buff);
		if(strcmp(buff, "-connecting") == 0) {
			inCall = true;
			pthread_create(&thread_id_s[0], NULL, sOut, &ssd);
			pthread_create(&thread_id_s[1], NULL, sInp, &ssd);
			
			if(video) {
				inVideo = true;
				pthread_create(&thread_id_v[0], NULL, vOut, &vsd);
				pthread_create(&thread_id_v[1], NULL, vInp, &vsd);
				video = false;
				
			}
		}
		if(strcmp(buff, "-video") == 0) {
			video = true;
		}
		if(strcmp(buff, "-incoming call") == 0){
			printf("enter -yes to start\nenter -no to stop\n");
		}

		if(strcmp(buff, "-call ended") == 0){
			inCall = false;
			sem_wait(&semaphore);
                inVideo = false;
			sem_post(&semaphore);
		}
		
		if(strcmp(buff, "-server_c") == 0){
			close(sd);
			exit(0);
		}
	}
	return inp;
}

int main(int argc, char **argv){
	sem_init(&semaphore, 0, 1);
	signal(SIGINT, handle_sigint); 
	struct sockaddr_in server, server_v;
	char buff[256];
	pthread_t thread_id;

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
	
	// std::string key(16, 0);
	// // auto Twofish::twofish = Twofish(key);
	// SymAlg *instance = new Twofish(key);
	while(1) {
		// fflush(stdin); 
		scanf("%255[^\n]", buff); 
		getchar();
		// instance.encrypt(buff);
		write(sd, buff, strlen(buff));
		write(sd, "\0", 1);

		if(strcmp(buff, "-exit") == 0){
			if(!inCall){
				close(sd);
				break;
			}
			inCall = false;
		}

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
