#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/tcp.h>

#define IP_ADDR "127.0.0.1"
#define PORT htons(9090)
#define true 1
#define false 0
#define BUFFER_SIZE 225280
#define CHUNK_SIZE 225280

void awaitCommand(int);
void sendFile(int);
void listFiles(int);

#endif // SERVER_H
