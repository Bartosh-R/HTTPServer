#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>       /* memset() */
#include <arpa/inet.h>    /* htonl() */
#include <unistd.h>
#include <stdlib.h>  	/* calloc */
#include <fcntl.h> 		/* dla czytania z pliku */
#include <stdio.h>

#define SIZE 1024								// podstawowy rozmiar stringa
#define DATA_SIZE 1025							// podstawowy rommiar wczytywanego pliku
#define BUFF_SIZE 1024							// podstawowy rozmiar bufora


int checkCommand(char * header, char * path);	//prawdza otrzymaną komendę, gdy jest prawidłowa zwraca 1 w przeciwnym zwraca 0
int readFile(char *path, char * data); 			// czyta plik, zwraca 0 w przypadku powodzenia i -1 w przeciwnym
void pisz(char * napis);						// pomocnicza funkcja do wypisywania komunikatów

int sockfd;                       // deskryptor gniazda
int polfd;						  // deskryptor zaakceptowanego gniazda

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
	printf("%d\n", counter);
}



// szablon nagłówka odpowiedzi
const char HEAD[] = "HTTP/1.0 200 OK\n\
	Server: NAZWA_SERWERA\n\
	Content-Length: %ld\
	\nConnection: close\
	\nContent-Type: %s\n\n";



int main(void){
    struct sockaddr_in server_addr;	// adres serwera 
   	char msg[SIZE];             	// czytana  wiadomosc 
	char *path;						// scieżka z żadania http
	char *data;						// dane z pliku do wysłania
	
	char respond[SIZE];				// przechowuje całą odpowiedź serwera łącznie z nagłówkiem

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;					// IPv4 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	// slucham na wszystkich interfejsach 
	server_addr.sin_port = htons(9080);					// slucham na porcie 12211 
    
    // tworze gniazdo - na raznie nie zwiazane z zadnym portem/adresem 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // "podpinam" gniazdo pod konkretny port/adres 
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	listen(sockfd, 5);

    // teraz gniazdo jest gotowe i moge czytac dane 
    while(1){
		polfd = accept(sockfd, NULL, NULL);
        read(polfd, msg, 1024);
		
		write(1, msg, strlen(msg)); // wyświetlanie zapytań klienta na ekranie
		
		path = calloc(SIZE, sizeof(char));
		if(checkCommand(msg, path) == 1) {
			data = calloc(1025, sizeof(char));
			readFile(path, data);
			sprintf(respond, HEAD, strlen(data), "text/html");
			
			write(polfd, respond, strlen(respond));
			write(polfd, data, strlen(data));
			
			write(1, "Przed\n", strlen("Przed\n"));
			free(data);
			write(1, "Po\n", strlen("Po\n"));
		}
		free(path);
		close(polfd);
    }

    return 0;
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

int readFile(char *path, char *data) {

	int data_size = DATA_SIZE;						// początkowy rozmiar buffora 
	char buff[BUFF_SIZE];							// buffor do którego zapisywane są dane z pliku
	
	int all_readed = 0;								// całkowita liczba przecztanych znaków
	int readed;										// liczba przecztanych znaków
	
	int fd = open(path, O_RDONLY);					// otwiera plik tylko do odczytu
	if (fd < 0) {									// zwraca -1 gdy nie udało się otworzyć pliku
		pisz("Bląd przy otwieraniu pliku");		
		return -1;
	}		
	
	while((readed = read(fd, buff, BUFF_SIZE)) != 0) {
		all_readed += readed;
				
		if(all_readed > data_size) {
			data_size += DATA_SIZE;
			data = (char*) realloc (data, data_size * sizeof(char));
		}
		
		strncat(data, buff, 1024);
		
		memset(buff,0,strlen(buff)); // czyszcenie bufora przed ponownym użyciem
	}
		
	close(fd);
	
	return 0;
}



void pisz(char * napis){
	write(1, napis, sizeof(napis));	
}


