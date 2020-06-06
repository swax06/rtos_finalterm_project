#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<signal.h> 
#include<stdbool.h>
#include<string.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<pthread.h>
#include "server.h"


int main(int argc,char **argv){
	signal(SIGINT, handleSigint); 
	struct sockaddr_in server, client, server_v;
	pthread_t thread_ids[200];
	unsigned int sd, clientLen, sd_v;
	sd = socket(AF_INET,SOCK_STREAM,0);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_port = htons(atoi(argv[2]));
	sd_v = socket(AF_INET,SOCK_STREAM,0);
	server_v.sin_family = AF_INET;
	server_v.sin_addr.s_addr = inet_addr(argv[1]);
	server_v.sin_port = htons(atoi(argv[3]));
	bind(sd,(struct sockaddr *)&server,sizeof(server));
	listen(sd, 200);
	bind(sd_v,(struct sockaddr *)&server_v,sizeof(server_v));
	listen(sd_v, 200);
	printf("Started.....\n");
	while(1){
		int temp_sd1, temp_sd2, temp_sd3;
		struct sockets temp_sd;
		clientLen = sizeof(client);
		temp_sd.sd = accept(sd, (struct sockaddr *)&client, &clientLen);
		temp_sd.ssd = accept(sd, (struct sockaddr *)&client, &clientLen);
		temp_sd.vsd = accept(sd_v, (struct sockaddr *)&client, &clientLen);
		// printf("%d %d %d\n", temp_sd.sd, temp_sd.ssd, temp_sd.vsd);
		pthread_create(&thread_ids[cli], NULL, clientHandler, &temp_sd);
	}
	close(sd);
	return 0;
}