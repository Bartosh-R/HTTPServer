#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

#define SIZE 1024
#define BUFF_SIZE 1024


int checkCommand(char * header, char * path);
int sendFile(char *path);

void onRespond(char * request);
void onInterrupt(int signal);

int getExtension(char * path);
long getFileSize(char *path);

int sockfd;
int polfd;

int childpid;


char * extensions[] = {
	".gif", ".jpg",".jpeg" ".png", ".zip",
 ".gz", ".tar", ".htm", ".html"};


char * filetypes[] = {
 	"image/gif", "image/jpeg","image/jpeg", "image/png", "image/zip",
  "image/gz", "image/tar", "text/html", "text/html"};



const char HEAD[] = "HTTP/1.0 200 OK\n\
	Server: NAZWA_SERWERA\n\
	Content-Length: %ld\
	\nConnection: close\
	\nContent-Type: %s\n\n";


const char ERROR_HEAD[] = "HTTP/1.1 403 Forbidden\n\
	Server: NAZWA_SERWERA\n\
	Content-Length:7\
	\nConnection: close\
	\nContent-Type: text\\html\n\nNO FILE";



int main(void){

  struct sockaddr_in server_addr;

	char request[SIZE];

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(9080);


  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	listen(sockfd, SOMAXCONN);

	struct sigaction sa;
	sa.sa_handler = onInterrupt;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, NULL);


    while(1){
			polfd = accept(sockfd, NULL, NULL);
			memset(request, 0, sizeof(request));
	    read(polfd, request, 1024);
			write(1, request, strlen(request));
			write(1,"\n\n", sizeof("\n\n"));
			 if((childpid =fork()) == 0) {
				onRespond(request);
				close(polfd);
				close(sockfd);
				exit(0);
			} else {
				close(polfd);
			}
    }

    return 0;
}

void onRespond(char * request) {
	char respond[SIZE];
	char *path;

	path = calloc(SIZE, sizeof(char));
	if(checkCommand(request, path) == 1) {
		long fileSize;
		if((fileSize = getFileSize(path)) != -1) {
			int type = getExtension(path);
			sprintf(respond, HEAD, fileSize, filetypes[type]);
			write(polfd, respond, strlen(respond));
			sendFile(path);
		} else {
			write(polfd, ERROR_HEAD, strlen(ERROR_HEAD));
		}
	}
	free(path);
}

int getExtension(char * path){
	path++;
	while(*path != '\0' && *path != '.') {
		path++;
	}

	int i;
	for(i = 0; i<9; i++) {
		if(strcmp(path, extensions[i]) == 0){
			return i;
		}
	}

	return 8;
}


void onInterrupt(int signal){
	(void) signal;
	close(sockfd);
	kill(childpid, SIGTERM);
	exit(0);
}

int checkCommand(char * header, char * path){
	char * pch;

	strcat(path,".");

    pch = strtok (header," ");
	// if(strcmp(pch, "GET") != 0)
	// 	return 0;

	pch = strtok (NULL, " ");
	strcat(path,pch);

	if(strcmp(path, "./") == 0)
		strcat(path, "index.html");

	pch = strtok (NULL, " \r\n");
	if(strcmp(pch, "HTTP/1.1") == 0 ||
		 strcmp(pch, "HTTP/1.0") == 0 )
		return 1;

	return 0;
}

int sendFile(char *path) {
	char buff[BUFF_SIZE];
	int readed;

	int fd = open(path, O_RDONLY);

	if (fd < 0)
		return -1;

	while((readed = read(fd, buff, BUFF_SIZE)) != 0) {
		write(polfd, buff, readed);
		memset(buff,0,strlen(buff));
	}

	close(fd);
	return 0;
}

long getFileSize(char *path){
	int result = 0;
	int readed;

	char buff[BUFF_SIZE];

	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	while((readed = read(fd, buff, BUFF_SIZE)) != 0) {
		result += readed;
	}

	return result;
}
