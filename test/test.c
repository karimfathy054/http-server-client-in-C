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

void send_message_body(char *file_path,int socket_fd){
    FILE *fp = fopen(file_path, "r");
    FILE *output = fopen("output.jpeg", "w");
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
        int f = fwrite(out_buffer, 1, file_read_bytes, output);
        // int status = send(socket_fd, out_buffer, file_read_bytes, 0);
        // if (status == -1)
        // {
        //     perror("can't send");
        //     break;
        // }
        memset(file_reader_buffer, 0, BUFFER_SIZE);
        memset(out_buffer, 0, BUFFER_SIZE);
        if(feof(fp)) break;
    }
    char *end_of_file = "/r/n";
    char buffer[BUFFER_SIZE] = {0};
    sprintf(buffer,"\r\n");
    if(strcmp(buffer,"\r\n") == 0){
        printf("true");

    }else{
        printf("false");
    }
    // send(socket_fd, end_of_file, strlen(end_of_file), 0);
    memset(file_reader_buffer,0, BUFFER_SIZE);
    memset(out_buffer, 0, BUFFER_SIZE);
    fclose(fp);
}
int main(int argc, char const *argv[])
{
    send_message_body("download.jpeg",1);

    return 0;
}


