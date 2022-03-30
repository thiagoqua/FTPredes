#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include"cmds.h"

#define BUFFLEN 30

int input2cmd(char[]);

int main(int argc,char *args[]){
    if(argc != 3){
        printf("** cantidad de argumentos incorrecta **\n");
        return -1;
    }
    int fhs,port,saddrlen;
    struct sockaddr_in saddress;
    char buffer[BUFFLEN] = {0},input[BUFFLEN] = {0};
    port = atoi(args[2]);
    saddrlen = sizeof(saddress);
    saddress.sin_family = AF_INET;
    saddress.sin_addr.s_addr = inet_addr(args[1]);
    saddress.sin_port = htons(port);
    if((fhs = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo en la creacion del socket **\n");
        return -2;
    }
    if(connect(fhs,(struct sockaddr*)&saddress,saddrlen) < 0){
        printf("** fallo la conexion **\n");
        return -3;
    }
    if(read(fhs,buffer,sizeof(buffer)) < 0){
        printf("** fallo la recepcion del mensaje **\n");
        return -4;
    }
    printf("%s\n",buffer);      //mensaje de bienvenida
    while(1){
        memset(buffer,0,sizeof(buffer));
        memset(input,0,sizeof(input));
        printf("Operación: ");
        scanf("%s",input);
        switch(input2cmd(input)){
            case QUIT:
                strcpy(buffer,"QUIT\r\n");
                if(write(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el enviado del comando **\n");
                    return -5;
                }
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo en la recepcion de la respuesta del servidor **\n");
                    return -6;
                }
                printf("%s",buffer);
                close(fhs);
                return 0;
            break;
            default:
                printf("\n* operación incorrecta. reingrese *\n\n");
            break;
        }
    }
return 0;}

int input2cmd(char input[]){
    if(strcmp(input,"quit") == 0)
        return QUIT;
return -1;}