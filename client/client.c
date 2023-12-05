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

#define BUFFER_SIZE 1024

char in_buffer[BUFFER_SIZE];
char out_buffer[BUFFER_SIZE];
char file_reader_buffer[BUFFER_SIZE];


void read_commands(char *commands_file_path, int socket_fd);
void execute_command(char *command, int socket_fd);
void send_request(char *method,char *file_name, int socket_fd);
int recieve_server_response(int socket_fd);
void get_request(char *file_name, int socket_fd);
void post_request(char *file_path, int socket_fd);
void send_message_body(char *file_path,int socket_fd);
void get_message_body(char *file_name, int socket_fd);

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        perror("invalid args");
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[2]);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0); // connection socket
    if (socket_fd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &(server.sin_addr));

    int status = connect(socket_fd, (struct sockaddr *)&server, sizeof(server)); // connection to server
    if (status < 0)
    {
        perror("can not connect to server");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    printf("connection success\n");
    read_commands("commands.txt", socket_fd); // execute commands

    return 0;
}
//get filename http/1.1\r\n
void send_request(char *method,char *file_name, int socket_fd){
    sprintf(out_buffer, "%s %s HTTP/1.1\r\n\r\n",method, file_name);
    while (send(socket_fd, out_buffer, strlen(out_buffer), 0) == -1)
    {
        perror("can't send\n");
        printf("resending again\n");
    }
}

void get_request(char *file_name, int socket_fd)
{
    send_request("GET",file_name,socket_fd);
    int status = recieve_server_response(socket_fd);

    if(status == true){
        get_message_body(file_name,socket_fd);
    }
    else if(status == false){
        return;
    }
    else{
        return get_request(file_name,socket_fd);
    }
}

int recieve_server_response(int socket_fd)
{
    // the server will either send:
    // HTTP/1.1 200 OK
    // or
    // HTTP/1.1 404 NOT_FOUND
    int status_code = 0;
    char server_message[1024] = {0};
    if (read(socket_fd, server_message, BUFFER_SIZE) == -1)
    {
        perror("can't recieve response");
        return -1;
    }
    printf("server: %s",server_message);
    char http[10] ={0};
    char code[10] = {0};
    char message[10] = {0};

    sscanf(server_message,"%s %s %s",http,code,message);
    status_code = atoi(code);
    if (status_code == 200)
    {
        printf("getting the file from server ...\n");
        return true;
    }
    else if(status_code == 404)
    {
        printf("file not found\n");
        return false;
    }

}

void get_message_body(char *file_name, int socket_fd)
{
    printf("downloading %s ...\n",file_name);
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        perror("can't open file");
    }
    int bytes_recieved = 0;
    int bytes_wrote = 0;
    memset(in_buffer,0,BUFFER_SIZE);
    while ((bytes_recieved = recv(socket_fd, in_buffer, BUFFER_SIZE-1, 0)) > 0)
    {
        if(strcmp(in_buffer,"\r\n") == 0){ //server terminating file
            break;
        }
        bytes_wrote = fwrite(in_buffer, bytes_recieved, sizeof(char), fp);
        if (bytes_wrote < 0)
        {
            perror("can't write in file");
        }
        // if (bytes_wrote < bytes_recieved)
        // {
        //     perror("something went wrong in writing file");
        memset(in_buffer,0,BUFFER_SIZE);
        if (bytes_recieved < BUFFER_SIZE)
        {
            break;
        }
    }
    fclose(fp);
}

void send_message_body(char *file_path,int socket_fd){
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL)
    {
        perror("can't open file\n");
        exit(EXIT_FAILURE);
    }
    int file_read_bytes;
    int sent_bytes;
    memset(out_buffer, 0, BUFFER_SIZE);
    while ((file_read_bytes = fread(file_reader_buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
    {
        memcpy(out_buffer, file_reader_buffer, BUFFER_SIZE);
        int status = send(socket_fd, out_buffer, file_read_bytes, 0);
        if (status == -1)
        {
            perror("can't send");
            break;
        }
        memset(file_reader_buffer, 0, BUFFER_SIZE);
        memset(out_buffer, 0, BUFFER_SIZE);
        if(feof(fp)) break;
    }
    sprintf(out_buffer,"\r\n");
    send(socket_fd, buff,sizeof(buff), 0);
    memset(file_reader_buffer,0, BUFFER_SIZE);
    memset(out_buffer, 0, BUFFER_SIZE);
    fclose(fp);
}

void read_commands(char *commands_file_path, int socket_fd)
{
    FILE *commands_file = fopen(commands_file_path, "r");
    if (commands_file == NULL)
    {
        perror("can't open commands file");
        exit(EXIT_FAILURE);
    }
    char command[BUFFER_SIZE];
    while (fgets(command, sizeof(command), commands_file) != NULL)
    {
        execute_command(command, socket_fd);
    }
    fclose(commands_file);
}

void execute_command(char *command, int socket_fd)
{
    char method[20] = {0};
    char file_path[20] = {0};
    char host_name[20] = {0};
    char port[20] = {0};
    sscanf(command, "%s %s %s %s", method, file_path, host_name, port);

    if (strcmp(method, "GET") == 0)
    {
        get_request(file_path, socket_fd);
    }
    else if (strcmp(method, "POST") == 0)
    {
        post_request(file_path, socket_fd);
    }
}

void post_request(char *file_path, int socket_fd)
{
    //send post request to server without the message body
    send_request("POST",file_path,socket_fd);
    //server either accepts or refuses
    //acceptance via HTTP OK
    //refuse via HTTP 404
    int status = recieve_server_response(socket_fd);
    if(status==true){//server accepts
        send_message_body(file_path,socket_fd);
    }
    else if(status == false){//server refuses
        return;
    }
    else{
        return post_request(file_path,socket_fd);
    }
}
