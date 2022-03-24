#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>

int main(int argc,char *args[]){
    if(argc != 3){
        printf("** cantidad de argumentos incorrecta **\n");
        exit(1);
    }
    int fhc,port,saddrlen;
    struct sockaddr_in saddress;
    port = atoi(args[2]);
    saddrlen = sizeof(saddress);
    saddress.sin_family = AF_INET;
    saddress.sin_addr.s_addr = inet_addr(args[1]);
    saddress.sin_port = htons(port);
    if((fhc = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo en la creacion del socket **\n");
        return -1;
    }
    if(connect(fhc,(struct sockaddr*)&saddress,saddrlen) < 0){
        printf("** fallo la conexion **\n");
        return -2;
    }
return 0;}