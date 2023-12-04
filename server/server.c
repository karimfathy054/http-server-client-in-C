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

#define SIZE 1024

typedef enum Status{OK = 200, Bad_Request = 400, Not_Found = 404} Status;
typedef enum Method{GET, POST} Method;

typedef struct{
    Method method;
    char fileName[50];
    char fileFormat[10];

}RequestLine;

int writeText(const char* buffer, const char* fileName);

const char* makeHeader(Status status);

void handleTextGetRequest(int sockFD, const char* fileName);

void parseRequestHeader(RequestLine* header, const char* request);

void handleTextPostRequest(int sockFD, RequestLine* requestLine, const char* initialChunk);

void handleRequest();

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
    printf("connection accepted\n");
    }

    //IMP!!!!!!! +1 is for \0 terminator
    char request[SIZE+1];
    int bytesRead = read(new_socket, request, SIZE);
    //IMP!!!!!!!!!!!
    request[bytesRead] = '\0';

    // printf("Original length = %d\n", bytesRead);
    
    // for(int i = 0; request[i] != '\0'; i++){
    //     printf("%d ", request[i]);
    // }
    // printf("\n");

    // printf("%s\n", request);
    

    char requestLineStr[SIZE];
    sscanf(request, "%[^\r\n]", requestLineStr);

    //printf("%s\n", requestLineStr);

    RequestLine header;
    parseRequestHeader(&header, requestLineStr);
    
    //printf("%d, %s, %s\n", header.method, header.fileName, header.fileFormat);

    //handleTextGetRequest(new_socket, header.fileName);

    // printf("%s\n", request);
    handleTextPostRequest(new_socket, &header, request);


    // const char* m = "TESSST";
    // write(listening_socket, m, strlen(m));

    close(listening_socket);

    //recieve requests from client
    
    //char* text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n";

    // writeText(text, "test.txt");

    //parse requests
        //print out the request + headers(i.e. everything before the message body )
    //if GET request
        //reply with http 200 OK and the file 
        //reply with http 404 NOTFOUND if there is no file
    //if POST request
        //exclude headers
    return 0;
}

int writeText(const char* buffer, const char* fileName){

    //append if file exists else write
    const char* mode = (access(fileName, F_OK) == 0)? "a" : "w";

    FILE* out = fopen(fileName, mode);
    if(out == NULL){return -1;}
    int size = fwrite(buffer, sizeof(char), strlen(buffer), out);
    fclose(out);
    return size;
}

const char* makeHeader(Status status){
    
    const char* header;

    switch (status)
    {
    case OK:
        header = "HTTP/1.1 200 OK \r\n";
        break;
    case Not_Found:
        header = "HTTP/1.1 404 Not Found\r\n";
        break;
    case Bad_Request:
        header = "HTTP/1.1 400 Bad Request\r\n";
        break;
    }
    return header;
}

void handleTextGetRequest(int sockFD, const char* fileName){
    
    char buffer[SIZE] = {0};
    
    if(access(fileName, F_OK)){
        //file does NOT exist
        const char* header = makeHeader(Not_Found);
        sprintf(buffer, "%s\n<h1>404: File Not Found :(</h1>", header);

        write(sockFD, buffer, strlen(buffer));

        // printf("AFTER sending\n");

    }else{

        FILE* file = fopen(fileName, "r");

        if(file == NULL){
            perror("Cannot open file.");
            fclose(file);
            exit(1);
        }

        char header[SIZE];
        sprintf(header, "%s\n", makeHeader(OK));

        write(sockFD, header, strlen(header));

        while(fgets(buffer, SIZE, file) != NULL){

            if(send(sockFD, buffer, strlen(buffer), 0) < 0){
                perror("Error sending file");
                fclose(file);
                exit(1);
            }
            memset(buffer, '\0', SIZE);
        }
        fclose(file);
    }
}


Method parseMethod(const char* methodString){
    if(!strcmp(methodString, "GET")){
        return GET;
    }else if(!strcmp(methodString, "POST")){
        return POST;
    }
    return Bad_Request;
}
void parseRequestHeader(RequestLine* header, const char* request){
    char methodStr[SIZE];
    // printf("%s\n", request);
    sscanf(
        request,
        "%s /%s HTTP/1.1\r\n",
        methodStr,
        header->fileName
    );
    header->method = parseMethod(methodStr);
    sscanf(
        header->fileName,
        "%[^.].%s",
        methodStr,
        header->fileFormat
    );
}

void handleTextPostRequest(int sockFD, RequestLine* requestLine, const char* initialChunk){
    FILE* file = fopen(requestLine->fileName, "w");
    if(file == NULL){
        perror("Could not write to file");
        exit(1);
    }

    // printf("%s\n", initialChunk);

    char buffer[SIZE+1];
    const char* ptr = initialChunk;
    ssize_t contentLength = -1, temp;
    while(sscanf(ptr, "%[^\r\n]", buffer) == 1){
        // printf("current: %s\n", buffer);

        if(sscanf(buffer, "Content-Length: %ld", &temp) == 1){
            contentLength = temp;
        }

        ptr += strlen(buffer);
        // printf("Next is: %s\n", ptr);
        //to escape \r\n
        ptr += 2;
    }

    if(contentLength < 0){
        perror("Content length not provided");
        fclose(file);
        exit(1);
    }

    printf("CONTENT LENGTH: %ld\n", contentLength);

    //escaping separator line
    while(sscanf(ptr, "%[ \r\n]", buffer) == 1){ptr++;}
    
    printf("CONTENT: %s\n", ptr);
    printf("length: %ld\n", strlen(ptr));
    fwrite(ptr, sizeof(char), strlen(ptr), file);

    memset(buffer, '\0', SIZE);
    ssize_t bytesRead, totalBytesRead = 0;

    char* contentBuffer = (char*)malloc(contentLength);
    if(contentBuffer == NULL){
        perror("Could not allocate buffer");
        fclose(file);
        exit(1);
    }

    while(totalBytesRead < contentLength){
        bytesRead = read(sockFD, contentBuffer + totalBytesRead, contentLength-totalBytesRead);
        if(bytesRead <= 0){break;}
        totalBytesRead += bytesRead;
        printf("read %ld up till now\n", totalBytesRead);
        printf("READ: %s\n", contentBuffer);
    }   
    contentBuffer[totalBytesRead] = '\0';
    fwrite(contentBuffer, sizeof(char), strlen(contentBuffer), file);
    fclose(file);
    free(contentBuffer);
}