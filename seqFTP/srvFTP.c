#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<errno.h>

#define VERSION "1.0"
#define BUFFLEN 30

int main(int argc,char *args[]){
    if(argc != 2){
        printf("** cantidad de argumentos incorrecta **\n");
        exit(1);
    }
    int fhs,fhc,addrlen,port;
    struct sockaddr_in address;
    char buffer[BUFFLEN] = {0};
    port = atoi(args[1]);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.25");
    address.sin_port = htons(port);
    addrlen = sizeof(address);
    if((fhs = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo en la creacion del socket **\n");
        return -1;
    }
    if(bind(fhs,(struct sockaddr*)&address,(socklen_t)addrlen) < 0){
        printf("** fallo el bind **\ncodigo de error %d.\n",errno);
        return -2;
    }
    if(listen(fhs,3) < 0){
        printf("** fallo el listen **\n");
        return -3;
    }
    if((fhc = accept(fhs,(struct sockaddr*)&address,(socklen_t*)&addrlen)) < 0){
        printf("** fallo el accept **\n");
        return -4;
    }
    sprintf(buffer,"220 %s version %s.",args[0],VERSION);
    if(write(fhc,buffer,sizeof(buffer)) < 0){
        printf("** fallo el enviado del mensaje **\n");
        return -5;
    }
return 0;}