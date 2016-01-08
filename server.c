#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>       						/* memset() */
#include <arpa/inet.h>    						/* htonl() */
#include <unistd.h>
#include <stdlib.h>  							/* calloc */
#include <fcntl.h> 								/* dla czytania z pliku */
#include <stdio.h>
#include <signal.h>								/**/

#define SIZE 1024								// podstawowy rozmiar stringa
#define DATA_SIZE 1025							// podstawowy rommiar wczytywanego pliku
#define BUFF_SIZE 1024							// podstawowy rozmiar bufora


int checkCommand(char * header, char * path);	// sprawdza otrzymaną komendę, gdy jest prawidłowa zwraca 1, w przeciwnym 0
int readFile(char *path); 						// czyta plik, w przypadku powodzenia zwraca 0,  w przeciwnym -1

void onRespond(char * request);					// funkcja obsługująca żądanie kilenta
void onInterrupt(int signal);					// funkcja wywoływana po otrzymaniu sygnału

void getExtension(char * path, char * result);

int sockfd;                       				// deskryptor gniazda
int polfd;						  				// deskryptor zaakceptowanego gniazda

int childpid;									// przechowuje pid pozyskany z forka

char *data;										// dane z pliku do wysłania

void print_num(int n){
	char cyfra[1];
	if(n > 0) {
		print_num(n/10);
		cyfra[0] = (n%10)+48;
		write(1,cyfra, 1);
	}
}

void test(char *napis) {
	int counter = 0;
	while(*napis != '\0') {
		counter++;
		napis++;
	}
	dprintf(1,"%d\n", counter);
}



// szablon nagłówka odpowiedzi pomyślnej
const char HEAD[] = "HTTP/1.0 200 OK\n\
	Server: NAZWA_SERWERA\n\
	Content-Length: %ld\
	\nConnection: close\
	\nContent-Type: %s\n\n";

// szablony nagłówka odpowiedzi niepomyślnej
const char ERROR_HEAD[] = "HTTP/1.1 403 Forbidden\n\
	Server: NAZWA_SERWERA\n\
	Content-Length:7\
	\nConnection: close\
	\nContent-Type: text\\html\n\nNO FILE";



int main(void){
    struct sockaddr_in server_addr;	// adres serwera 
	
	char request[SIZE];             // przechowuje odebranie żądanie 

    memset(&server_addr, 0, sizeof(server_addr));		// "zerowanie" struktury sockaddr_in
    server_addr.sin_family = AF_INET;					// IPv4 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	// slucham na wszystkich interfejsach 
	server_addr.sin_port = htons(9080);					// slucham na porcie 9080 
    
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);								// tworze gniazdo - na raznie nie zwiazane z zadnym portem/adresem 
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));		// "podpinam" gniazdo pod konkretny port/adres 
	listen(sockfd, 5);														// próbuje nawiązać połączenie z serwerem
	
	struct sigaction sa;
	sa.sa_handler = onInterrupt;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	
	sigaction(SIGINT, &sa, NULL);


    while(1){
		polfd = accept(sockfd, NULL, NULL);							// akcepotowanie połączenia i uzyskanie deskryptora
        read(polfd, request, 1024);									// zapisywanie otrzymanego żądania do zmiennej request
		write(1, request, strlen(request)); 						// wypisanie otrzymanego żądania w konsoli
		 if((childpid =fork()) == 0) {
			onRespond(request);										// stworzenie potomka który obsłuży żądanie
			close(polfd);
			close(sockfd);
			exit(0);												// zakończenie procesu po wykonaniu zadania
		} else {
			close(polfd);											// zamknięcie nie potrzebnego deskryptora
			// tutaj nic nie robi;
		}
    }

    return 0;
}

void onRespond(char * request) {
	char respond[SIZE];											// przechowuje całą odpowiedź serwera łącznie z nagłówkiem
	char *path;													// scieżka z żądania http
	
	path = calloc(SIZE, sizeof(char));							// wyświetlanie zapytań klienta na ekranie
	if(checkCommand(request, path) == 1) {
		data = calloc(1025, sizeof(char));
		if(readFile(path) == 0) {
			// char last[5];
// 			getExtension(path, last);
// 			printf("%s", last);
			
			sprintf(respond, HEAD, strlen(data), "text/html");
			write(polfd, respond, strlen(respond));
			write(polfd, data, strlen(data));
		} else {
			write(polfd, ERROR_HEAD, strlen(ERROR_HEAD));
		}
		free(data);
	}
	free(path);
}

void getExtension(char * path, char * result){
	char tmp[5];
	while(*path != '.' && *path != '\n') {
		path++;
	}
	
	int i = 0;
	while(*path != '\0') {
		*tmp = *path;
		tmp[i] = *path;
		path++;
		i++;
	}
	
	strcpy(result, tmp);
}



void onInterrupt(int signal){
	close(sockfd);
	kill(childpid, SIGTERM);
	exit(0);
}

int checkCommand(char * header, char * path){
	char * pch;
	
	strcat(path,".");			// katalog root dla serwera 
		
    pch = strtok (header," "); // sprawdzenie czy żądanie zaczyna się od słowa GET
	if(strcmp(pch, "GET") != 0)
		return 0;
		
	pch = strtok (NULL, " "); // pobranie i zapisanie ścieżki z żądania
	strcat(path,pch);
	
	if(strcmp(path, "./") == 0) // ustawienie index.html jako domyślnego pliku przy zapytaniach
		strcat(path, "index.html");
	
	pch = strtok (NULL, " \r\n"); // sprawdzenie czy żadanie kończy się HTTP/1.0 lub HTTP/1.1
	if(strcmp(pch, "HTTP/1.1") == 0 ||
		 strcmp(pch, "HTTP/1.0") == 0 )
		return 1;
		
	return 0; // zwraca 0 gdy żądanie nie posiada HTTP/1.0 lub HTTP/1.1
}

int readFile(char *path) {

	int data_size = DATA_SIZE;						// początkowy rozmiar buffora 
	char buff[BUFF_SIZE];							// buffor do którego zapisywane są dane z pliku
	
	int all_readed = 0;								// całkowita liczba przecztanych znaków
	int readed;										// liczba przecztanych znaków
	
	int fd = open(path, O_RDONLY);					// otwiera plik tylko do odczytu
	
	if (fd < 0) 
		return -1;									// zwraca -1 gdy nie udało się otworzyć pliku	
		
	while((readed = read(fd, buff, BUFF_SIZE)) != 0) {
		all_readed += readed;
				
		if(all_readed > data_size) {
			data_size += DATA_SIZE;
			data = (char*) realloc (data, data_size * sizeof(char));
		}
		
		strncat(data, buff, 1024);
		memset(buff,0,strlen(buff)); 				// czyszcenie bufora przed ponownym użyciem
	}
	
	test(data);		
	close(fd);
	
	return 0;
}




