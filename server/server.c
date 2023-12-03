#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        perror("invalid args");
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    struct sockaddr_in server_address,client_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1",&server_address.sin_addr);
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(listening_socket<0){
        perror("can't create socket");
        exit(EXIT_FAILURE);
    }
    if(bind(listening_socket, (struct sockaddr *)&server_address, sizeof(server_address))!=0){
        perror("can't bind");
        exit(EXIT_FAILURE);
    }
    if(listen(listening_socket, 5)!=0){
        perror("can't listen");
        exit(EXIT_FAILURE);
    }
    socklen_t addres_size = sizeof(client_address);
    int new_socket = accept(listening_socket,(struct sockaddr*)&client_address,&addres_size);
    if(new_socket<0){
        perror("can't create new socket");
        exit(EXIT_FAILURE);
    }
    if(new_socket>0){
    printf("connection accepted");
    }

    return 0;
}
