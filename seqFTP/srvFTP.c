#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include"cmds.h"

#define VERSION "1.0"
#define IP "127.0.0.25"

int autentication(int);

int main(int argc,char *args[]){
    if(argc != 2){
        printf("** cantidad de argumentos incorrecta **\n");
        exit(1);
    }
    int fhs,fhc,addrlen,port;
    struct sockaddr_in address;
    char buffer[BUFFLEN] = {0},cmd[4] = {0};
    port = atoi(args[1]);
    //conexion con cliente
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(IP);
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
    //interaccion entre servidor-cliente
    sprintf(buffer,"%d %s version %s.\r\n",CONSUC,args[0],VERSION);
    if(write(fhc,buffer,sizeof(buffer)) < 0){           //mensaje de bienvenida
        printf("** fallo el enviado del mensaje de bienvenida **\n");
        return -5;
    }
    if(autentication(fhc) < 0){
        close(fhc);
        close(fhs);
        return -6;
    }
    while(1){                                           //gestion de las peticiones
        memset(buffer,0,sizeof(buffer));
        if(read(fhc,buffer,sizeof(buffer)) < 0){
            printf("** fallo la lectura del comando enviado por el cliente **\n");
            return -7;
        }
        strncpy(cmd,buffer,(size_t)4);
        cmd[4] = '\0';                                  //esto es porque strncpy no pone caracteres de fin de linea en este0o
        switch(str2cmd(cmd)){
            case QUIT:
                memset(buffer,0,sizeof(buffer));
                sprintf(buffer,"%d Goodbye.\r\n",CONFIN);
                if(write(fhc,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el envio de la respuesta al cliente **\n");
                    return -8;
                }
                close(fhc);
                close(fhs);
                return 0;
            break;
        }
    }
return 0;}

int autentication(int fhc){
    char buffer[BUFFLEN] = {0},cmd[4] = {0},user[AUTLEN] = {0},pass[AUTLEN] = {0};
    if(read(fhc,buffer,sizeof(buffer)) < 0){            //recibo usuario
        printf("** fallo la lectura del comando enviado por el cliente **\n");
        return -1;
    }
    strncpy(cmd,buffer,(size_t)4);                      //obtengo el comando del buffer
    cmd[4] = 0;
    if(str2cmd(cmd) == USER){
        strncpy(user,buffer + 5,(size_t)BUFFLEN); //obtengo el usuario
        memset(buffer,0,sizeof(buffer));
        sprintf(buffer,"%d Password required for %s\r\n",PASREQ,user);
        if(write(fhc,buffer,sizeof(buffer)) < 0){           //mensaje de requerimiento de password
            printf("** fallo el enviado del mensaje al cliente **\n");
            return -1;
        }
        memset(buffer,0,sizeof(buffer));
        if(read(fhc,buffer,sizeof(buffer)) < 0){            //recibo contraseña
            printf("** fallo la lectura del comando enviado por el cliente **\n");
            return -1;
        }
        memset(cmd,0,sizeof(cmd));
        strncpy(cmd,buffer,(size_t)4);
        cmd[4] = 0;
        if(str2cmd(cmd) == PASS){
            strncpy(pass,buffer + 5,(size_t)BUFFLEN); //obtengo la contraseña
        } else {
            printf("** comando invalido **\n");
            return -1;
        }
    } else {
        printf("** comando invalido **\n");
        return -1;
    }
return 0;}