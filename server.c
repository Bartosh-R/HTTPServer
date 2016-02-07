#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>	// memset()
#include <arpa/inet.h> // htonl()
#include <unistd.h>
#include <stdlib.h> // calloc 
#include <fcntl.h>  // dla czytania z pliku 
#include <stdio.h>
#include <signal.h>	// do obsługi sygnałów

#define SIZE 1024	// podstawowy rozmiar stringa
#define BUFF_SIZE 1024	// podstawowy rozmiar bufora


int checkCommand(char * header, char * path);	// sprawdza otrzymaną komendę, gdy jest prawidłowa zwraca 1, w przeciwnym 0
int sendFile(char *path);	// czyta plik podany w path i wysyła do klienta

void onRespond(char * request);	// funkcja obsługująca żądanie kilenta
void onInterrupt(int signal); // funkcja wywoływana po otrzymaniu sygnału

int getExtension(char * path); // podaje numer indeksu odpoowiedni dla wysyłanego rodzaju pliku
long getFileSize(char *path); // zwraca rozmiar pliku w bajtach

int sockfd;	// deskryptor gniazda
int polfd;	// deskryptor zaakceptowanego gniazda

int childpid;	// przechowuje pid pozyskany z forka

// przechowuje znane rozszerzenia plików
char * extensions[] = {
	".gif", ".jpg",".jpeg" ".png", ".zip",
 ".gz", ".tar", ".htm", ".html"};
 
// przechowuje odpowiadające 
char * filetypes[] = {
 	"image/gif", "image/jpeg","image/jpeg", "image/png", "image/zip",
  "image/gz", "image/tar", "text/html", "text/html"};


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
	
	char request[SIZE];	// przechowuje odebranie żądanie
	char buff[255]; 
	int rozmiar;

    memset(&server_addr, 0, sizeof(server_addr)); // "zerowanie" struktury sockaddr_in
    server_addr.sin_family = AF_INET; // IPv4 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // slucham na wszystkich interfejsach 
	server_addr.sin_port = htons(9080);	// slucham na porcie 9080 
    
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // tworze gniazdo - na razie nie zwiazane z zadnym portem/adresem 
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)); // "podpinam" gniazdo pod konkretny port/adres 
	listen(sockfd, SOMAXCONN); // próbuje nawiązać połączenie z serwerem
	
	struct sigaction sa; // tworzenie struktry do obsługi sygnałów
	sa.sa_handler = onInterrupt; // przypisanie funkcji onInterrupt jako handler (uchwyt)
	sigemptyset(&sa.sa_mask); // "czyszczenie maski"
	sa.sa_flags = 0; // ustawienie maski
	
	sigaction(SIGINT, &sa, NULL); // ustawienie obsługi Ctrl-C przez funkcję onInterrupt


    while(1){
		polfd = accept(sockfd, NULL, NULL);	// akcepotowanie połączenia i uzyskanie deskryptora
		memset(request, 0, sizeof(request));
		while((rozmiar = read(polfd, buff, 255)) != 0) {
			write(1, buff, rozmiar);
		}
		write(1, request, strlen(request)); // wypisanie otrzymanego żądania w konsoli
		 if((childpid =fork()) == 0) {
			onRespond(request);	// stworzenie potomka który obsłuży żądanie
			close(polfd); // zamykanie deskryptora połączenia w potomku
			close(sockfd); // zamykanie socketu
			exit(0); // zakończenie procesu po wykonaniu zadania
		} else {
			close(polfd); // zamknięcie nie potrzebnego deskryptora
			// tutaj nic nie robi;
		}
    }

    return 0;
}

void onRespond(char * request) {
	char respond[SIZE];	// przechowuje całą odpowiedź serwera łącznie z nagłówkiem
	char *path; // scieżka z żądania http
	
	path = calloc(SIZE, sizeof(char)); // wyświetlanie zapytań klienta na ekranie
	if(checkCommand(request, path) == 1) { // sprawdza poprawność otrzymanego żądanie (1 - poprawne, 0 - niepoprawne)
		long fileSize; // przechowuje rozmiar pliku 
		if((fileSize = getFileSize(path)) != -1) { // pobranie rozmiaru pliku i sprawdzenie czy istnieje ( -1 świady o braku możliwości otworzenia)
			int type = getExtension(path); // pobranie indeksu tablicy oznaczonego 
			sprintf(respond, HEAD, fileSize, filetypes[type]);
			write(polfd, respond, strlen(respond)); // 
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
	
	return 8; // w przypadku nie znanego rozszerzenia oznacza jako text/html
}



void onInterrupt(int signal){
	(void) signal; // aby pozbyć się ostrzeżenia o nieużywanej zmiennej
	close(sockfd);	// zamknięcie socketu
	kill(childpid, SIGTERM); // zabicie procesu potomnego jeżeli nie zrobił tego sam
	exit(0); // wyjście z aplikacji
}

int checkCommand(char * header, char * path){
	char * pch; // zmienne przechowująca aktualny fragment analizowanego nagłówka
	
	strcat(path,"."); // katalog root dla serwera 
		
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

int sendFile(char *path) {
	char buff[BUFF_SIZE]; // buffor do którego zapisywane są dane z pliku
	int readed;	// liczba przecztanych znaków
	
	int fd = open(path, O_RDONLY); // otwiera plik tylko do odczytu
	
	if (fd < 0) 
		return -1; // zwraca -1 gdy nie udało się otworzyć pliku	
		
	while((readed = read(fd, buff, BUFF_SIZE)) != 0) {
		write(polfd, buff, readed);	// wysyłanie danych do klienta
		memset(buff,0,strlen(buff)); // czyszcenie bufora przed ponownym użyciem
	}
		
	close(fd); // zamkykanie deskryptora
	return 0;
}

long getFileSize(char *path){
	int result = 0;	// całkowita liczba przecztanych znaków
	int readed;	// liczba przecztanych znaków
	
	char buff[BUFF_SIZE]; // buffor do czytania pliku;
	
	int fd = open(path, O_RDONLY); // otwiera plik tylko do odczyt
	if (fd < 0) 
		return -1; // zwraca -1 gdy nie udało się otworzyć pliku	
		
	while((readed = read(fd, buff, BUFF_SIZE)) != 0) {
		result += readed; // dodawanie przeczytanych danych do zmiennej przechowującej ogólny rozmiar
	}

	return result;
}




