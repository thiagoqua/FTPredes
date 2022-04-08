#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include"cmds.h"

#define DESTFILES   "cliFiles/"                 //DIRECTORIO DONDE SE VAN A ALMACENAR LOS ARCHIVOS TRANSFERIDOS

int input2cmd(char[]);                          //extrae el comando de lo ingresado por el usuario
int autentication(int);                         //realiza toda la autenticación completa del usuario que quiere acceder

int main(int argc,char *args[]){
    if(argc != 3){
        printf("** cantidad de argumentos incorrecta **\n");
        return -1;
    }
    int fhs,port,saddrlen;
    struct sockaddr_in saddress;
    char buffer[BUFFLEN] = {0},input[BUFFLEN] = {0},cmd[5] = {0},nof[NOFLEN] = {0};
    //conexion con servidor
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
    //interacciones entre cliente y servidor
    if(read(fhs,buffer,sizeof(buffer)) < 0){
        printf("** fallo la recepcion del mensaje **\n");
        return -4;
    }
    #ifdef DEB
        printf("\n%s\n",buffer);      //mensaje de bienvenida
    #endif
    if(autentication(fhs) < 0)
        return -5;
    while(1){                   //se generan y gestionan las peticiones al servidor
        memset(buffer,0,sizeof(buffer));
        memset(input,0,sizeof(input));
        printf("operación: ");
        fgets(input,sizeof(input),stdin);
        strtok(input,"\n");
        strncpy(cmd,input,(size_t)4);
        switch(input2cmd(cmd)){
            case QUIT:
                strcpy(buffer,"QUIT\r\n");
                if(write(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el enviado del comando **\n");
                    return -6;
                }
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo en la recepcion de la respuesta del servidor **\n");
                    return -7;
                }
                #ifdef DEB
                    printf("\n%s\n",buffer);      //mensaje de bienvenida
                #endif
                close(fhs);
                return 0;
            break;
            case GET:
                strcpy(nof,input + 4);
                memset(buffer,0,sizeof(buffer));
                sprintf(buffer,"RETR %s\r\n",nof);
                if(write(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el enviado del comando **\n");
                    return -7;
                }
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo en la recepcion de la respuesta del servidor **\n");
                    return -7;
                }
                #ifdef DEB
                    printf("\n%s\n",buffer);      //mensaje de bienvenida
                #endif
                //FALTA LA CONTINUACION
            break;
            default:
                printf("\n* operación incorrecta. reingrese *\n\n");
            break;
        }
    }
return 0;}

int input2cmd(char str[]){
    if(strcmp(str,"quit") == 0)
        return QUIT;
    else if(strcmp(str,"get ") == 0)
        return GET;
return -1;}

int autentication(int fhs){
    int retcode;
    char input[AUTLEN],buffer[BUFFLEN] = {0};
    printf("username: ");
    fgets(input,sizeof(input),stdin);
    strtok(input,"\n");
    sprintf(buffer,"USER %s\r\n",input);
    if(write(fhs,buffer,sizeof(buffer)) < 0){               //envio usuario al servidor
        printf("** fallo el enviado del comando **\n");
        return -1;
    }
    memset(buffer,0,sizeof(buffer));
    memset(input,0,sizeof(input));
    if(read(fhs,buffer,sizeof(buffer)) < 0){                //espero resupesta que requiere contraseña
        printf("** fallo en la recepcion de la respuesta del servidor **\n");
        return -1;
    }
    #ifdef DEB
        printf("\n%s\n",buffer);      //mensaje de bienvenida
    #endif
    printf("password: ");
    fgets(input,sizeof(input),stdin);
    strtok(input,"\n");
    memset(buffer,0,sizeof(buffer));
    sprintf(buffer,"PASS %s\r\n",input);
    if(write(fhs,buffer,sizeof(buffer)) < 0){               //envio contraseña al servidor
        printf("** fallo el enviado del comando **\n");
        return -1;
    }
    memset(buffer,0,sizeof(buffer));
    if(read(fhs,buffer,sizeof(buffer)) < 0){                //espero codigo de respuesta
        printf("** fallo en la recepcion de la respuesta del servidor **\n");
        return -1;
    }
    retcode = atoi(buffer);                                 //me quedo con el codigo que devolvió el servidor
    #ifdef DEB
        printf("\n%s\n",buffer);      //mensaje de bienvenida
    #endif
    if(retcode == LOGUNS)
        return -1;
return 0;}