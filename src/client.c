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

// packet structure with header
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
	struct sockaddr_in ser_addr;
	struct hostent* ip_addr;
	int sockfd;
	
	struct packet* cli_hdr = (struct packet*)malloc(sizeof(struct packet));
	struct packet* comm4_pkt = (struct packet*)malloc(sizeof(struct packet));
	struct packet* recv_hdr;
	char* recv_buf = (char*)malloc(sizeof(char) * 8);
	
	FILE* file_ptr = fopen(argv[3], "r");
	int read_count;
	//int file_len;

	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(PORT);	
	ip_addr = gethostbyname(argv[1]);
	if(ip_addr == NULL){
		fprintf(stderr, "client/gethostbyname: unable to get host\n");
		return 2;
	}
	ser_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)ip_addr->h_addr));
	
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
	while(feof(file_ptr) == 0){ // send file contents to server
		memset(cli_hdr->payload, 0, MAXBUFSIZE-8);
		read_count = fread(cli_hdr->payload, sizeof(char), MAXBUFSIZE - 8, file_ptr);
		(cli_hdr->payload)[read_count]='\0';
		cli_hdr->len = htons(strlen((const char*)cli_hdr->payload) + 8);
		write(sockfd, cli_hdr, sizeof(struct packet));
		//printf("-----step 3-----\n");
		//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n",cli_hdr->ver, cli_hdr->user_ID, ntohs(cli_hdr->seq),ntohs(cli_hdr->len), ntohs(cli_hdr->command));

		cli_hdr->seq = htons(ntohs(cli_hdr->seq)+1);
	}
	// 4-1: command data store
	comm4_pkt->ver = 0x04;
	comm4_pkt->user_ID = 0x08;
	comm4_pkt->command = htons(0x0004);
	comm4_pkt->seq = htons(ntohs(cli_hdr->seq));
	memset(comm4_pkt->payload, 0, MAXBUFSIZE-8);
	strncpy(comm4_pkt->payload, argv[3], strlen((const char*)argv[3]));
	comm4_pkt->len = htons(strlen((const char*)comm4_pkt->payload) + 8);
	write(sockfd, comm4_pkt, sizeof(struct packet));
	//printf("-----step 4-----\n");
	//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n",comm4_pkt->ver, comm4_pkt->user_ID, ntohs(comm4_pkt->seq),ntohs(comm4_pkt->len), ntohs(comm4_pkt->command));
	
	// close connection
	close(sockfd);
	return 0;
}
