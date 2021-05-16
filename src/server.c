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
#include <wait.h>

#define MAXBUFSIZE 65000
#define MAXFILESIZE 5000000
#define BACKLOG 20

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
	int PORT = atoi(argv[1]);
	int sockfd;
	struct sockaddr_in ser_addr;
	int connfd;
	int cli_addr;
	int cli_len;
	pid_t cli_pid;
	char* recv_buf = (char*)malloc(sizeof(char) * MAXBUFSIZE);
	char* recv_file_buf = (char*)malloc(sizeof(char) * MAXFILESIZE);
	char* recv_file_helper = recv_file_buf;
	struct packet* recv_hdr;
	int option;
	struct packet* ser_hdr = (struct packet*)malloc(sizeof(struct packet));

	FILE* new_fp;
	char file_name[MAXBUFSIZE];

	int status;
	// socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		fprintf(stderr, "server/socket: failed\n");
		return 2;
	}

	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(PORT);
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	option = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	// bind
	if(bind(sockfd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) < 0){
		fprintf(stderr, "server/bind: failed\n");
		return 2;
	}

	// listen
	if(listen(sockfd, BACKLOG) < 0){
		fprintf(stderr, "server/listen: failed\n");
		return 2;
	} 

	// set header
	ser_hdr->ver = 0x04;
	ser_hdr->user_ID = 0x08;

	// accept
	while(1){
		cli_len = sizeof(cli_addr);
		connfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);
		if(connfd < 0){
			//fprintf(stderr, "server/accept: failed\n");
			return 2;
		}
		cli_pid = fork();
		while(waitpid(cli_pid, &status, WNOHANG) == 0){
			; // check zombie process
		}
		if(cli_pid == 0){ // child process
			close(sockfd);
			
			// 1-2: receive and write with client
			if(recv(connfd, recv_buf, 8, 0) < 0){
				//fprintf(stderr, "server/recv: failed\n");
				return 2;
			}
			recv_hdr = (struct packet*)recv_buf;
			//printf("-----step 1-----\n");
			//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n", recv_hdr->ver, recv_hdr->user_ID, ntohs(recv_hdr->seq), ntohs(recv_hdr->len), ntohs(recv_hdr->command));
			// 2-1: server hello
			ser_hdr->len = htons(8);
			ser_hdr->command = htons(0x0002);
			ser_hdr->seq = htons(ntohs(recv_hdr->seq) + 1);
			write(connfd, ser_hdr, sizeof(ser_hdr));
			//printf("-----step 2-----\n");
			//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n", ser_hdr->ver, ser_hdr->user_ID, ntohs(ser_hdr->seq), ntohs(ser_hdr->len), ntohs(ser_hdr->command));
			// 3-2: receive file and store in buffer
			memset(recv_buf, 0, sizeof(recv_buf));
			read(connfd, recv_buf, MAXBUFSIZE);
			recv_buf[MAXBUFSIZE] = '\0';
			recv_hdr = (struct packet*)recv_buf;
			//printf("-----step 3-----\n");
			//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n", recv_hdr->ver, recv_hdr->user_ID, ntohs(recv_hdr->seq), ntohs(recv_hdr->len), ntohs(recv_hdr->command));
			
			memset(recv_file_buf, 0, sizeof(recv_file_buf));
			strncpy(recv_file_buf, recv_hdr->payload, strlen(recv_hdr->payload));
			while(htons(recv_hdr->command) == 3){
				recv_file_helper += strlen(recv_hdr->payload);
				memset(recv_buf, 0, sizeof(recv_buf));
				sleep(0.5);//
				read(connfd, recv_buf, MAXBUFSIZE);
				recv_buf[MAXBUFSIZE] = '\0';
				recv_hdr = (struct packet*)recv_buf;
				//printf("-----step 3-----\n");
				//printf("ver: %d ID: %d seq: %d len: %d comm: %d\n", recv_hdr->ver, recv_hdr->user_ID, ntohs(recv_hdr->seq), ntohs(recv_hdr->len), ntohs(recv_hdr->command));
				
				if(ntohs(recv_hdr->command) == 4) break;
				// copy to file buffer
				strncpy(recv_file_helper, recv_hdr->payload, strlen(recv_hdr->payload));
			}
			// 4-2: got file name, store in server's side
			strncpy(file_name, recv_hdr->payload, strlen(recv_hdr->payload));
			new_fp = fopen(file_name, "w");
			fputs(recv_file_buf, new_fp);
			fclose(new_fp);	
			return 2;
		}
	}
	close(connfd);
    return 0;
}
