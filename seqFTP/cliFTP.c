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
    int fhs,fhfs,fhfc,port,saddrlen,faddrlen;
    struct sockaddr_in saddress,faddress;
    char buffer[BUFFLEN] = {0},input[BUFFLEN] = {0},cmd[5] = {0},nof[NOFLEN] = {0};
    //conexion con servidor mediante socket de comandos
    port = atoi(args[2]);
    saddress.sin_family = AF_INET;
    saddress.sin_addr.s_addr = inet_addr(args[1]);
    saddress.sin_port = htons(port);
    saddrlen = sizeof(saddress);
    if((fhs = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo en la creacion del socket **\n");
        return -2;
    }
    if(connect(fhs,(struct sockaddr*)&saddress,(socklen_t)saddrlen) < 0){
        printf("** fallo la conexion **\n");
        return -3;
    }
    if((fhfs = socket(AF_INET,SOCK_STREAM,0)) < 0){         //creo ya de ahora el socket para transferencia de archivos
        printf("** fallo en la creacion del socket de escucha **\n");
        return -8;
    }
    //conexion con servidor mediante socket de transferencias
    faddress.sin_family = AF_INET;
    faddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    faddress.sin_port = htons(port + 1);
    faddrlen = sizeof(faddress);
    if(bind(fhfs,(struct sockaddr*)&faddress,(socklen_t)faddrlen) < 0){
        //cambio el puerto
        faddress.sin_port = htons(0);
        if(bind(fhfs,(struct sockaddr*)&faddress,(socklen_t)faddrlen) < 0){
            printf("** fallo el bind post cambiado de puerto **\n");
            return -35;
        }
    }
    if(listen(fhfs,3) < 0){
        printf("** fallo el listen **\n");
        return -4;
    }
    if((fhfc = accept(fhfs,(struct sockaddr*)&faddress,((socklen_t*)&faddrlen))) < 0){
        printf("** fallo el accept **\n");
        return -5;
    }
    #ifdef DEB
        printf("servidor con ip '%s' conectado\n",inet_ntoa(faddress.sin_addr));
    #endif
    //interacciones entre cliente y servidor
    if(read(fhs,buffer,sizeof(buffer)) < 0){
        printf("** fallo la recepcion del mensaje **\n");
        return -6;
    }
    #ifdef DEB
        printf("\n%s\n",buffer);      //mensaje de bienvenida
    #endif
    if(autentication(fhs) < 0)
        return -7;
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
                    return -8;
                }
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo en la recepcion de la respuesta del servidor **\n");
                    return -9;
                }
                #ifdef DEB
                    printf("\n%s\n",buffer);      
                #endif
                close(fhs);
                return 0;
            break;
            case GET:
                //mando comando RETR
                strcpy(nof,input + 4);
                memset(buffer,0,sizeof(buffer));
                sprintf(buffer,"RETR %s\r\n",nof);
                if(write(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el enviado del comando **\n");
                    return -10;
                }
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo en la recepcion de la respuesta del servidor **\n");
                    return -11;
                }
                #ifdef DEB
                    printf("\n%s\n",buffer);
                #endif
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