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
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>


#define SIZE 1024
#define BUFFER_SIZE 1024

pthread_mutex_t lock;
float sock_counter = 0;
fd_set sockets_list;
struct timeval timeout;

// typedef enum Status{OK = 200, Bad_Request = 400, Not_Found = 404} Status;
// typedef enum Method{GET, POST} Method;

// typedef struct{
//     Method method;
//     char fileName[50];
//     char fileFormat[10];

// }RequestLine;

// int writeText(const char* buffer, const char* fileName);

// const char* makeHeader(Status status);

// void handleTextGetRequest(int sockFD, const char* fileName);

// void parseRequestHeader(RequestLine* header, const char* request);

// void handleTextPostRequest(int sockFD, RequestLine* requestLine, const char* initialChunk);

// void handleRequest();

void *handle_connection(void *);

int main(int argc, char const *argv[])
{
    if (argc < 2)
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

    while(true){
        socklen_t addres_size = sizeof(client_address);
        int new_socket = accept(listening_socket,(struct sockaddr*)&client_address,&addres_size);
        if(new_socket<0){
            perror("can't create new socket");
            exit(EXIT_FAILURE);
        }
        if(new_socket>0){
        printf("connection accepted\n");
        }

        pthread_mutex_lock(&lock);
        FD_SET(new_socket,&sockets_list);
        sock_counter ++;
        pthread_mutex_unlock(&lock);

        pthread_t thread_id;
        int *socket_number = malloc(sizeof(int));
        *socket_number = new_socket;
        pthread_create(&thread_id,NULL,handle_connection,socket_number);

    }
    return 0;
}

void send_content_body(char* file_name,int socket_fd){
    
    FILE *fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        perror("can't open file\n");
        // exit(EXIT_FAILURE);
        return;
    }

    char out_buffer[BUFFER_SIZE];
    char file_reader_buffer[BUFFER_SIZE];
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
    char *end_of_file = "\r\n";
    char buff[BUFFER_SIZE]={0};
    sprintf(buff,"\r\n");
    send(socket_fd, buff, BUFFER_SIZE, 0);
    memset(file_reader_buffer,0, BUFFER_SIZE);
    memset(out_buffer, 0, BUFFER_SIZE);
    fclose(fp);
}

void exec_get_response(int socket_fd,char* file_name){
    
    if(access(file_name,F_OK)){
        char* response = "HTTP/1.1 404 Not Found\r\n\r\n";
        int status = send(socket_fd,response,strlen(response),0);
        if(status<0){
            perror("cant send response");
            return;
        }
    }
    else{
        char* response = "HTTP/1.1 200 OK\r\n\r\n";
        int status = write(socket_fd,response,strlen(response));
        if(status<0){
            perror("cant send response");
            return;
        }
        send_content_body(file_name,socket_fd);
    }
}

void recv_content_body(char *file_name,int socket_fd){
    printf("downloading %s ...\n",file_name);
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        perror("can't open file");
        return;
    }
    int bytes_recieved = 0;
    char in_buffer[BUFFER_SIZE] = {0};
    memset(in_buffer,0,BUFFER_SIZE);
    while ((bytes_recieved = recv(socket_fd, in_buffer, BUFFER_SIZE, 0)) > 0)
    {
        if(strcmp(in_buffer,"\r\n") == 0){ //server terminating file
            printf("terminating file :%s",file_name);
            break;
        }
        bytes_wrote = fwrite(in_buffer, bytes_recieved, sizeof(char), fp);
        if (bytes_wrote < 0)
        {
            perror("can't write in file");
        }
        if (bytes_wrote < bytes_recieved)
        {
            perror("something went wrong in writing file");
        }
        memset(in_buffer,0,BUFFER_SIZE);
        if (bytes_recieved < BUFFER_SIZE)
        {
            break;
        }
    }
    fclose(fp);
}

void exec_post_request(int socket_fd,char *file_name){
    char* response = "HTTP/1.1 200 OK\r\n\r\n";
    int status = send(socket_fd,response,strlen(response),0);
    if(status<0){
        perror("cant send response");
        return;
    }
    recv_content_body(file_name,socket_fd);
}


int recieve_client_request(int socket_fd)
{
    // the client will either send:
    // GET FILE HTTP/1.1\r\n\r\n
    // or
    // POST FILE HTTP/1.1\r\n\r\n
    int status_code = 0;
    char client_request[1024] = {0};
    int status = read(socket_fd, client_request, sizeof(client_request));
    if (status == -1)
    {
        perror("can't recieve response");
        return -1;
    }
    printf("%s",client_request);
    char method[10] ={0};
    char file_name[20] = {0};

    sscanf(client_request,"%s %s ",method,file_name);
    if(strcmp(method,"GET")==0){
        exec_get_response(socket_fd,file_name);
    }
    if(strcmp(method,"POST")==0){
        exec_post_request(socket_fd,file_name);
    }

}


void *handle_connection(void *socket_fd){
    pthread_mutex_lock(&lock);
    timeout.tv_sec = 10/sock_counter;
    pthread_mutex_unlock(&lock);

    int connection_socket = *((int *)socket_fd);
    while(true){
        int t = select(FD_SETSIZE,&sockets_list,NULL,NULL,&timeout);
        if(t == 0){
            printf("timeout\n");
            close(connection_socket);
        }
        //execute request
        recieve_client_request(connection_socket);
    }
    pthread_mutex_lock(&lock);
    sock_counter--;
    timeout.tv_sec = 3 / sock_counter;
    pthread_mutex_unlock(&lock);
}

// int writeText(const char* buffer, const char* fileName){

//     //append if file exists else write
//     const char* mode = (access(fileName, F_OK) == 0)? "a" : "w";

//     FILE* out = fopen(fileName, mode);
//     if(out == NULL){return -1;}
//     int size = fwrite(buffer, sizeof(char), strlen(buffer), out);
//     fclose(out);
//     return size;
// }

// const char* makeHeader(Status status){
    
//     const char* header;

//     switch (status)
//     {
//     case OK:
//         header = "HTTP/1.1 200 OK \r\n";
//         break;
//     case Not_Found:
//         header = "HTTP/1.1 404 Not Found\r\n";
//         break;
//     case Bad_Request:
//         header = "HTTP/1.1 400 Bad Request\r\n";
//         break;
//     }
//     return header;
// }

// void handleTextGetRequest(int sockFD, const char* fileName){
    
//     char buffer[SIZE] = {0};
    
//     if(access(fileName, F_OK)){
//         //file does NOT exist
//         const char* header = makeHeader(Not_Found);
//         sprintf(buffer, "%s\n<h1>404: File Not Found :(</h1>", header);

//         write(sockFD, buffer, strlen(buffer));

//         // printf("AFTER sending\n");

//     }else{

//         FILE* file = fopen(fileName, "r");

//         if(file == NULL){
//             perror("Cannot open file.");
//             fclose(file);
//             exit(1);
//         }

//         char header[SIZE];
//         sprintf(header, "%s\n", makeHeader(OK));

//         write(sockFD, header, strlen(header));

//         while(fgets(buffer, SIZE, file) != NULL){

//             if(send(sockFD, buffer, strlen(buffer), 0) < 0){
//                 perror("Error sending file");
//                 fclose(file);
//                 exit(1);
//             }
//             memset(buffer, '\0', SIZE);
//         }
//         fclose(file);
//     }
// }


// Method parseMethod(const char* methodString){
//     if(!strcmp(methodString, "GET")){
//         return GET;
//     }else if(!strcmp(methodString, "POST")){
//         return POST;
//     }
//     return Bad_Request;
// }
// void parseRequestHeader(RequestLine* header, const char* request){
//     char methodStr[SIZE];
//     // printf("%s\n", request);
//     sscanf(
//         request,
//         "%s /%s HTTP/1.1\r\n",
//         methodStr,
//         header->fileName
//     );
//     header->method = parseMethod(methodStr);
//     sscanf(
//         header->fileName,
//         "%[^.].%s",
//         methodStr,
//         header->fileFormat
//     );
// }

// void handleTextPostRequest(int sockFD, RequestLine* requestLine, const char* initialChunk){
//     FILE* file = fopen(requestLine->fileName, "w");
//     if(file == NULL){
//         perror("Could not write to file");
//         exit(1);
//     }

//     // printf("%s\n", initialChunk);

//     char buffer[SIZE+1];
//     const char* ptr = initialChunk;
//     ssize_t contentLength = -1, temp;
//     while(sscanf(ptr, "%[^\r\n]", buffer) == 1){
//         // printf("current: %s\n", buffer);

//         if(sscanf(buffer, "Content-Length: %ld", &temp) == 1){
//             contentLength = temp;
//         }

//         ptr += strlen(buffer);
//         // printf("Next is: %s\n", ptr);
//         //to escape \r\n
//         ptr += 2;
//     }

//     if(contentLength < 0){
//         perror("Content length not provided");
//         fclose(file);
//         exit(1);
//     }

//     printf("CONTENT LENGTH: %ld\n", contentLength);

//     //escaping separator line
//     while(sscanf(ptr, "%[ \r\n]", buffer) == 1){ptr++;}
    
//     printf("CONTENT: %s\n", ptr);
//     printf("length: %ld\n", strlen(ptr));
//     fwrite(ptr, sizeof(char), strlen(ptr), file);

//     memset(buffer, '\0', SIZE);
//     ssize_t bytesRead, totalBytesRead = 0;

//     char* contentBuffer = (char*)malloc(contentLength);
//     if(contentBuffer == NULL){
//         perror("Could not allocate buffer");
//         fclose(file);
//         exit(1);
//     }

//     while(totalBytesRead < contentLength){
//         bytesRead = read(sockFD, contentBuffer + totalBytesRead, contentLength-totalBytesRead);
//         if(bytesRead <= 0){break;}
//         totalBytesRead += bytesRead;
//         printf("read %ld up till now\n", totalBytesRead);
//         printf("READ: %s\n", contentBuffer);
//     }   
//     contentBuffer[totalBytesRead] = '\0';
//     fwrite(contentBuffer, sizeof(char), strlen(contentBuffer), file);
//     fclose(file);
//     free(contentBuffer);
// }