#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define VERSION 1.0

int main(int argc,char *args[]){
    if(argc != 2){
        printf("** cantidad de argumentos incorrecta **\n");
        exit(1);
    }
    int fhs,fhc,addrlen,port;
    struct sockaddr_in address;
    port = atoi(args[1]);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    addrlen = sizeof(address);
    if((fhs = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo en la creacion del socket **");
        return -1;
    }
    if(bind(fhs,(struct sockaddr*)&address,(socklen_t)addrlen) < 0){
        printf("** fallo el bind **");
        return -2;
    }
    if(listen(fhs,3) < 0){
        printf("** fallo el listen **");
        return -3;
    }
    if((fhc = accept(fhs,(struct sockaddr*)&address,(socklen_t*)&addrlen)) < 0){
        printf("** fallo el accept **");
        return -4;
    }
return 0;}