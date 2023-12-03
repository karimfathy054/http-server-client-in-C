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



void post_request(char* file_path,int socket_fd);
void get_request(char* file_name,int socket_fd);
void recieve_response(int socket_fd);
void get_message_body(char* file_name,int socket_fd);
void read_commands(char* commands_file_path,int socket_fd);
void parse_command(char* command,int socket_fd);

int main(int argc, char const *argv[]){
    if(argc != 3){
        //error
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[2]);

    printf("port: %d\n", port);

    int socket_fd = socket(AF_INET,SOCK_STREAM,0);
    if(socket_fd <0){
        perror("socket failed");
    }
    //get server address
    // printf("%s\n",argv[1]);
    struct sockaddr_in server;
    server.sin_family=AF_INET;
    server.sin_port = htons(port);
    // server.sin_addr.s_addr= inet_addr(argv[1]);
    // printf("%d\n",server.sin_addr.s_addr);
    inet_pton(AF_INET,argv[1],&(server.sin_addr));

    printf("string address : %s\n",argv[1]);
    printf("numerical address : %d\n",server.sin_addr.s_addr);
    //create socket for connection
    
    //connect to server
    int status = connect(socket_fd,(struct sockaddr *)&server,sizeof(server));
    if(status < 0){
        perror("can not connect to server");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    printf("connection success\n");

    read_commands("commands.txt",socket_fd);

    return 0;
}


void get_request(char* file_name,int socket_fd){
    sprintf(out_buffer,"GET %s HTTP/1.1\r\n\r\n",file_name);
    if(send(socket_fd,out_buffer,strlen(out_buffer),0) == -1){
        perror("can't send");
    }
    recieve_response(socket_fd);
    // get_message_body(file_name,socket_fd);
}

void recieve_response(int socket_fd){
    // int status_code = 0;
    // char status_code_buffer[1024];
    // if(recv(socket_fd,status_code_buffer,sizeof(status_code_buffer),0) == -1){
    //     perror("can't recieve");
    // }
    // status_code = atoi(status_code_buffer);
    // if(status_code == 200){
    //     printf("recieve success\n");
    // }else{
    //     printf("recieve failed\n");
    // }

    int bytes_recieved = recv(socket_fd,in_buffer,BUFFER_SIZE,0);

    while((bytes_recieved = recv(socket_fd,in_buffer,BUFFER_SIZE,0)) > 0){
        printf("%s",in_buffer);
        memset(in_buffer,0,sizeof(in_buffer));
        if(bytes_recieved<BUFFER_SIZE){
            break;
        }
    }
}

void get_message_body(char* file_name,int socket_fd){
    
    FILE * fp = fopen(file_name,"w");
    if(fp == NULL){
        perror("can't open file");
    }
    int bytes_recieved = 0;
    int bytes_wrote = 0;
    while((bytes_recieved = recv(socket_fd,in_buffer,BUFFER_SIZE,0)) > 0){
        bytes_wrote = fwrite(in_buffer,bytes_recieved,sizeof(char),fp);
        if (bytes_wrote < 0)
        {
            perror("can't write in file");
        }
        if(bytes_wrote<bytes_recieved){
            perror("something went wrong in writing file");
        }
        bzero(in_buffer,BUFFER_SIZE);
        if(bytes_recieved <BUFFER_SIZE){
            break;
        }
    }
    fclose(fp);
}


void read_commands(char* commands_file_path,int socket_fd){
    FILE* commands_file = fopen(commands_file_path,"r");
    if(commands_file == NULL){
        perror("can't open file");
    }
    char command[1024];
    while(fgets(command,sizeof(command),commands_file)!= NULL){
        parse_command(command,socket_fd);
    }
    fclose(commands_file);
}

void parse_command(char* command,int socket_fd){
    char method[20] = {0};
    char file_path[20] = {0};
    char host_name[20] = {0};
    char port[20] = {0};
    sscanf(command,"%s %s %s %s",method,file_path,host_name,port);
    
    if(strcmp(method,"GET") == 0){
        get_request(file_path,socket_fd);
    }else if(strcmp(method,"POST") == 0){
        post_request(file_path,socket_fd);
    }

}

void post_request(char* file_path,int socket_fd){
    char request[BUFFER_SIZE];
    FILE* fp = fopen(file_path,"r");
    FILE *output = fopen("output.txt","w");
    if(fp == NULL){
        perror("can't open file");
    }
    int file_read_bytes;
    int sent_bytes;
    memset(out_buffer,0,BUFFER_SIZE);
    int head_of_req=0;
    while((file_read_bytes = fread(file_reader_buffer,sizeof(char),BUFFER_SIZE,fp))>0){
        if(head_of_req==0){
            sprintf(out_buffer,"POST %s HTTP/1.1\r\n\r\n\n%s",file_path,file_reader_buffer);//headers
            head_of_req=1;
        }else{
            memcpy(out_buffer,file_reader_buffer,BUFFER_SIZE);
        }
        int status = send(socket_fd,out_buffer,file_read_bytes,0);
        fwrite(out_buffer,sizeof(char),file_read_bytes,output);
        if(status == -1){
            perror("can't send");
            break;
        }
        memset(file_reader_buffer,0,BUFFER_SIZE);
        memset(out_buffer,0,BUFFER_SIZE);
        if(feof(fp)){
            bzero(file_reader_buffer,BUFFER_SIZE);
            break;
        }
    }
}


