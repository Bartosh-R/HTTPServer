all:
	gcc -g -Wall -Wextra server.c
term:
	telnet -4 localhost 9080
kill:
	killall -9 a.out
start:
	./a.out&

