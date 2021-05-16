#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>

#define MAXBUFSIZE 65000
#define _CRT_SECURE_NO_WARNINGS //neglect sprintf secure warning

struct packet{
	unsigned char ver;
	unsigned char user_ID;
	unsigned short seq;
	unsigned short len;
	unsigned short command;
	unsigned char payload[MAXBUFSIZE-8];
};

int main(int argc, char *argv[]) {
	/* Change as you wish */
 	int PORT = atoi(argv[2]);
	int num_of_req = atoi(argv[3]);
	pid_t pid_list[num_of_req]; // pid list
	struct sockaddr_in ser_addr;
	struct hostent* ip_addr;
	int sockfd;
	
	struct packet* cli_hdr = (struct packet*)malloc(sizeof(struct packet));
	//struct packet* comm4_pkt = (struct packet*)malloc(sizeof(struct packet));
	struct packet* recv_hdr;
	char* recv_buf = (char*)malloc(sizeof(char) * 8);

	char inform_pid[30];
	char pid_str[10];
	//int file_len;
	
	int i;

	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(PORT);	
	ip_addr = gethostbyname(argv[1]);
	if(ip_addr == NULL){
		fprintf(stderr, "client/gethostbyname: unable to get host\n");
		return 2;
	}
	ser_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)ip_addr->h_addr));
	
	// fork childs * num_of_req. Then, each child process runs
	for(i=0; i<num_of_req; i++){
		// create child process
		sleep(0.1); // sleep 100ms for each iteration
		//printf("process %d forks!\n",getpid());//
		//sleep(3);// get rid of this later
		pid_list[i] = fork();
		if(pid_list[i] == 0){// child
			//printf("I am child process %d from %d!!\n",getpid(),getppid());//
			// socket
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if(sockfd < 0){
				fprintf(stderr, "client/socket: failed\n");
				exit(1);
			}

			// connect
			if(connect(sockfd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) < 0){
				fprintf(stderr, "client/connect: failed\n");
				exit(1);
			}
		
			// set header
			cli_hdr->ver = 0x04;
			cli_hdr->user_ID = 0x08;
			srand(time(NULL));
			cli_hdr->seq = htons(rand());
			// we later set the length and command 
	
			// receive and write with server
	
			// 1-1: client hello
			cli_hdr->len = htons(8);
			cli_hdr->command = htons(0x0001);
			write(sockfd, cli_hdr, sizeof(cli_hdr));
			sleep(1);//
			//printf("-----step 1-----\n");
			//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n",cli_hdr->ver, cli_hdr->user_ID, ntohs(cli_hdr->seq), ntohs(cli_hdr->len), ntohs(cli_hdr->command));
			// 2-2: check server hello
			if(recv(sockfd, recv_buf, 8, 0) < 0){
				fprintf(stderr, "client/recv: failed\n");
				return 2;
			}
			recv_hdr = (struct packet*)recv_buf;
			//printf("-----step 2-----\n");
			//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n",recv_hdr->ver, recv_hdr->user_ID, ntohs(recv_hdr->seq),ntohs(recv_hdr->len), ntohs(recv_hdr->command));
			if(ntohs(recv_hdr->command)!=2){
				cli_hdr->command = htons(0x0005);
				write(sockfd, cli_hdr, sizeof(struct packet));
				//printf("somewhat wrong in step 2\n");//
			}
			// 3-1: data delivery
			cli_hdr->command = htons(0x0003);
			cli_hdr->seq = htons(ntohs(recv_hdr->seq)+1);
			//printf("%d\n",getpid());//
			sprintf(inform_pid, "I am a process %d", getpid());
			memset(cli_hdr->payload, 0, MAXBUFSIZE-8);
			strncpy(cli_hdr->payload, inform_pid, strlen((const char*)inform_pid));
			cli_hdr->len = htons(strlen((const char*)inform_pid) + 8);
			write(sockfd, cli_hdr, sizeof(struct packet));
			//printf("-----step 3-----\n");
			//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n",cli_hdr->ver, cli_hdr->user_ID, ntohs(cli_hdr->seq),ntohs(cli_hdr->len), ntohs(cli_hdr->command));
			// 4-1: command data store
			//comm4_pkt->ver = 0x04;
			//comm4_pkt->user_ID = 0x08;
			cli_hdr->command = htons(0x0004);
			cli_hdr->seq = htons(ntohs(cli_hdr->seq)+1);
			sprintf(pid_str, "%d.txt", getpid());
			memset(cli_hdr->payload, 0, MAXBUFSIZE-8);
			strncpy(cli_hdr->payload, pid_str, strlen((const char*)pid_str));
			cli_hdr->len = htons(strlen((const char*)pid_str) + 8);
			write(sockfd, cli_hdr, sizeof(struct packet));
			//printf("-----step 4-----\n");
			//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n",comm4_pkt->ver, comm4_pkt->user_ID, ntohs(comm4_pkt->seq),ntohs(comm4_pkt->len), ntohs(comm4_pkt->command));
	
			// close connection
			close(sockfd);
			return 1;
		}
	}
	return 0;
}
