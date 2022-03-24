#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>

#define BUFFLEN 30

int main(int argc,char *args[]){
    if(argc != 3){
        printf("** cantidad de argumentos incorrecta **\n");
        exit(1);
    }
    int fhs,port,saddrlen;
    struct sockaddr_in saddress;
    char buffer[BUFFLEN] = {0};
    port = atoi(args[2]);
    saddrlen = sizeof(saddress);
    saddress.sin_family = AF_INET;
    saddress.sin_addr.s_addr = inet_addr(args[1]);
    saddress.sin_port = htons(port);
    if((fhs = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo en la creacion del socket **\n");
        return -1;
    }
    if(connect(fhs,(struct sockaddr*)&saddress,saddrlen) < 0){
        printf("** fallo la conexion **\n");
        return -2;
    }
    if(read(fhs,buffer,sizeof(buffer)) < 0){
        printf("** fallo la recepcion del mensaje **\n");
        return -3;
    }
    printf("message from server: %s\n",buffer);
return 0;}