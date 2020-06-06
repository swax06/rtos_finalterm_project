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
#include "server.h"
#include "server_ext.h"
#include "opencv2/opencv.hpp"
#include<pthread.h>

int cli = 0, temp_ind = 0, grp = 0;
struct client *clients[200];
struct group groups[20], *glptr;

void handleSigint(int sig){ 
    printf("Exiting... %d\n", sig); 
	if(sig == 2){
		exit(0);
	}
}

void* clientHandler(void* input) {
	int j = 0, cl = 100, i = 0, mode = 1;
	struct sockets sid = *(struct sockets*)input;
	bool f = false, scall = false, vcall = false;
	char buff[256], buff1[290], sbuff[256];
	struct session session;
	struct client *This;
	pthread_t thread_id_s, thread_id_v;
	myRead(sid.sd, buff);
	while(i < cli){
		if(strcmp(buff, clients[i] -> name) == 0){
			f = true;
			break;
		}
		i++;
	}
	if(!f) {
		clients[cli] = (struct client *) malloc(sizeof(struct client));
		clients[cli] -> sockts = (struct sockets *) malloc(sizeof(struct sockets));
		strcpy(clients[cli] -> name, buff);
		clients[cli] -> g = 0;
		clients[cli] -> inCall = false;
		cli++;
	}
	clients[i] -> sockts -> sd = sid.sd;
	clients[i] -> sockts -> ssd = sid.ssd;
	clients[i] -> sockts -> vsd = sid.vsd;
	clients[i] -> online = true;
	This = clients[i];
	This -> ptr = &session;

	printf("%s connected\n", This -> name);
	
	while(1){
		myRead(sid.sd, buff);
		
		if(buff[0] == '-'){
			if(strcmp(buff, "-users") == 0){
				send_names(sid.sd);
			}

			else if(strcmp(buff, "-groups") == 0){
				send_groups(sid.sd);
			}

			else if(strcmp(buff, "-send msg") == 0){
				send_names(sid.sd);
				write(sid.sd, "server: select a user\0", 22);
				myRead(sid.sd, buff);
				if(buff[0] > '9' || buff[0] <= '0'){
					continue;
				}
				cl = buff[0] - '0' - 1; 
				cl = clients[cl] -> sockts -> sd;
				write(sid.sd,"connected\0",10);
				mode = 1;
			}

			else if(strcmp(buff, "-send grp msg") == 0){
				send_groups(sid.sd);
				write(sid.sd, "server: select a group\0", 23);
				myRead(sid.sd,buff);
				if(buff[0] > '9' || buff[0] <= '0'){
					continue;
				}
				cl = buff[0] - '0' - 1; 
				write(sid.sd, "connected to group\ntype -end to end conversation\0",49);
				mode = 2;
			}

			else if(strcmp(buff, "-yes") == 0) {
				write(This -> sockts -> sd, "-connecting\0", 12);
				This -> inCall = true;
				scall = true;
			}

			else if(strcmp(buff, "-no") == 0) {
				This -> inCall = false;
				This -> ptr -> count--;
			}

			else if(strcmp(buff, "-call") == 0) {
				This -> inCall = true;
				send_names(sid.sd);
				write(sid.sd, "server: select a user\0", 22);
				myRead(sid.sd, buff);

				if(buff[0] > '9' || buff[0] <= '0'){
					write(sid.sd, "invalid input\0", 14);
				}
				cl = buff[0] - '0' - 1; 

				if(clients[cl] -> inCall || !clients[cl] -> online){
					write(sid.sd, "server: cannot place call\0", 26);
					write(sid.sd, "-call ended\0", 11);
					continue;
				}
				clients[cl] -> ptr = &session;
				This -> ptr = &session;
				session.mem[0] = clients[cl];
				session.mem[1] = This;
				session.count = 2;
				// write(sid.sd, "server: video call?\0", 20);
				// myRead(sid.sd, buff);
				
				// if(strcmp(buff, "1")) 
				// 	vcall = true;
				write(clients[cl] -> sockts -> sd, "-incoming call\0", 15);
				write(sid.sd, "-connecting\0", 12);
				scall = true;
				// vcall = true;
			}

			else if(strcmp(buff, "-grp call") == 0) {
				This -> inCall = true;
				send_groups(sid.sd);
				write(sid.sd, "server: select a group\0", 23);
				myRead(sid.sd, buff);

				if(buff[0] > '9' || buff[0] <= '0'){
					write(sid.sd,"invalid input\0",14);
					continue;
				}
				cl = buff[0] - '0' - 1;
				This -> ptr = &session;
				session.count = 1;
				session.mem[0] = This;
				j = 1;
				i = 0;

				while(i < groups[cl].count) {
					if(!groups[cl].mem[i] -> inCall){
						groups[cl].mem[i] -> ptr = &session;
						session.mem[j] = groups[cl].mem[i];
						session.count ++;
						write(groups[cl].mem[i] -> sockts -> sd, "-incoming call\0", 15);
					}
					i++;
				}
				if(session.count == 1){
					write(This -> sockts -> sd, "-none available\0", 16);
					continue;
				}
				write(This -> sockts -> sd, "-connecting\0", 12);
				scall = true;
			}
			
			else if(strcmp(buff, "-video") == 0){
				This -> inCall = true;
				send_names(sid.sd);
				write(sid.sd, "server: select a user\0", 22);
				myRead(sid.sd, buff);

				if(buff[0] > '9' || buff[0] <= '0'){
					write(sid.sd, "invalid input\0", 14);
				}
				cl = buff[0] - '0' - 1; 

				if(clients[cl] -> inCall || !clients[cl] -> online){
					write(sid.sd, "server: cannot place call\0", 26);
					write(sid.sd, "-call ended\0", 11);
				}
				clients[cl] -> ptr = &session;
				This -> ptr = &session;
				session.mem[0] = clients[cl];
				session.mem[1] = This;
				session.count = 2;
				write(clients[cl] -> sockts -> sd, "-video\0", 7);
				write(clients[cl] -> sockts -> sd, "-incoming call\0", 15);
				write(sid.sd, "-connecting\0", 12);
				scall = true;
				vcall = true;
			}

			else if(strcmp(buff, "-end") == 0) {
				cl = 100;
			}

			else if(strcmp(buff, "-exit") == 0){
				if(!This -> inCall) {
					close(sid.sd);
					break;
				}
				This -> inCall = false;
				This -> ptr -> count--;
			}

			else if(strcmp(buff, "-make grp") == 0){
				write(sid.sd, "server: name your group\0", 24);
				myRead(sid.sd, buff);
				strcpy(groups[grp].name, buff);
				send_names(sid.sd);
				write(sid.sd, "server: select the users for group\nserver: type -end to exit selection\0", 71);
				groups[grp].count = 0;

				while(strcmp(buff, "-end") != 0){
					myRead(sid.sd, buff);
					if(buff[0] > '9' || buff[0] <= '0'){
						continue;
					}
					cl = buff[0] - '0' - 1; 
					clients[cl] -> gp[clients[cl] -> g] = grp;
					clients[cl] -> g++;
					groups[grp].mem[groups[grp].count] = clients[cl];
					groups[grp].count++;
				}
				grp++;
				write(sid.sd,"server: group created\0",22);
			}

			else{
				write(sid.sd,"server: invalid input\nenter -h for help\0",40);
			}
			// if(vcall) {
			// 	vcall = false;
			// 	pthread_create(&thread_id_v, NULL, v_call, This);
			// }
			if(scall) {
				scall = false;
				pthread_create(&thread_id_s, NULL, s_call, This);
				
				if(vcall) {
					vcall = false;
					pthread_create(&thread_id_v, NULL, v_call, This);
				}
			}
		}
		else {
			if(mode == 1) { 
				buff[220] = '\0';
				sprintf(buff1, "%s : %s%c", This -> name, buff, '\0');
				write(cl, buff1, strlen(buff1) + 1);
			}
			if(mode == 2) {
				i = 0;
				while (i < groups[cl].count){
					sprintf(buff1, "%s@%s : %s%c", This -> name, groups[cl].name, buff, '\0');
					write(groups[cl].mem[i] -> sockts -> sd, buff1, strlen(buff1) + 1);
					i++;
				}	
			}
		}
	}
	printf("%s disconnected\n", This -> name);
	This -> online = false;
	This -> sockts -> sd = -1;
	return NULL;
}




