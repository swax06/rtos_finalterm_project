#include <sys/socket.h>
#include <pthread.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <stdbool.h>
#include <unistd.h>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
int capDev = 4;
VideoCapture cap(2);
bool inVideo = false;

void* vOut(void *inp) {
	int vsd = *(int *)inp;
	cv::Mat img;
    img = cv::Mat::zeros(480 , 640, CV_8UC1);
    int imgSize = img.total() * img.elemSize();
    int bytes = 0;
	int data_size = 15000;
	char data[data_size];
    if ( ! img.isContinuous() ) { 
        img = img.clone();
    }
    cv::namedWindow("CV Video Client", 1);
    char buff[10];
    std::string str;
	while (!inVideo);
    while (inVideo) {
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
        cv::waitKey(50);
    }
	cv::destroyAllWindows();
	return inp;
}


void* vInp(void *inp) {
	int vsd = *(int *)inp;
	cv::Mat img, flippedFrame, imgGray;
	int height = cap.get(CAP_PROP_FRAME_HEIGHT);
    int width = cap.get(CAP_PROP_FRAME_WIDTH);
    img = cv::Mat::zeros(height, width, CV_8UC1);
    int bytes = 0, data_size = 15000;;
    char data[data_size];
    // make img continuos
    if(!img.isContinuous()){ 
        img = img.clone();
    }
    std::vector<uchar> buff;
    std::vector<int> param(2);
    param[0] = cv::IMWRITE_JPEG_QUALITY;
    param[1] = 30;//default(95) 0-100
    bool status = true;  
	while (!inVideo);
    while(inVideo){
        cap >> img;
        cv::flip(img, flippedFrame, 1);
        cv::cvtColor(flippedFrame, imgGray, cv::COLOR_BGR2GRAY);
        cv::imencode(".jpg", imgGray, buff, param);
        for(int i = 0; i < buff.size(); i++){
            data[i] = buff[i];
        }
        write(vsd, data, data_size);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
	// cap.release();
	return inp;
}