#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<errno.h>
#include"cmds.h"

#define DIRDESTFILES "cliFiles/"                //DIRECTORIO DONDE SE VAN A ALMACENAR LOS ARCHIVOS TRANSFERIDOS

int input2cmd(char[]);                          //extrae el comando de lo ingresado por el usuario
int autentication(int);                         //realiza toda la autenticación completa del usuario que quiere acceder
int receivefile(int,char[]);

int main(int argc,char *args[]){
    if(argc != 3){
        printf("** cantidad de argumentos incorrecta **\n");
        return -4;
    }
    int fhs,fhfs,fhfc,port,saddrlen,retcode;
    struct sockaddr_in saddress;
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
    //conexion con servidor mediante socket de transferencias
    saddress.sin_family = AF_INET;
    saddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    saddress.sin_port = htons(port + 1);
    saddrlen = sizeof(saddress);
    if((fhfs = socket(AF_INET,SOCK_STREAM,0)) < 0){         //creo el socket para transferencia de archivos
        printf("** fallo en la creacion del socket de escucha **\n");
        return -8;
    }
    if(bind(fhfs,(struct sockaddr*)&saddress,(socklen_t)saddrlen) < 0){
        //cambio el puerto
        port = 0;
        saddress.sin_port = htons(port);
        saddrlen = sizeof(saddress);
        if(bind(fhfs,(struct sockaddr*)&saddress,(socklen_t)saddrlen) < 0){
            //si falló ahora es porque hay otro error y aborto
            printf("** fallo el bind post cambiado de puerto **\n");
            return -35;
        }
        getsockname(fhfs,(struct sockaddr*)&saddress,(socklen_t*)&saddrlen);
        #ifdef DEB
            printf("puerto %d ocupado por lo que cambiamos de puerto.\n",atoi(args[2])+1);
        #endif
        //aviso al servidor del cambio
        sprintf(buffer,"PORT %d\r\n",ntohs(saddress.sin_port));
        if(write(fhs,buffer,sizeof(buffer)) < 0){
            printf("** fallo el enviado del comando **\n");
            return -36;
        }
    }
    if(listen(fhfs,3) < 0){
        printf("** fallo el listen **\n");
        return -4;
    }
    if((fhfc = accept(fhfs,(struct sockaddr*)&saddress,((socklen_t*)&saddrlen))) < 0){
        printf("** fallo el accept **\n");
        return -5;
    }
    getsockname(fhfs,(struct sockaddr*)&saddress,(socklen_t*)&saddrlen);
    #ifdef DEB
        printf("canal de transferencia creado en puerto %d.\n\n",ntohs(saddress.sin_port));
        printf("servidor con ip '%s' conectado al canal de transferencias.\n",inet_ntoa(saddress.sin_addr));
    #endif
    //interacciones entre cliente y servidor
    memset(buffer,0,sizeof(buffer));
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
                close(fhs); close(fhfs); close(fhfc);
                return 0;
            break;
            case GET:
                //envio nombre de archivo y recibo tamaño
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
                retcode = atoi(buffer);                     //obtengo codigo de respuesta
                if(retcode == FILENF)
                    continue;   
                //proceso de recepción de archivo 
                if(receivefile(fhfc,nof) < 0)
                    return -12;
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo en la recepcion de la respuesta del servidor **\n");
                    return -11;
                }
                #ifdef DEB
                    printf("%s\n",buffer);
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
        printf("\n%s\n",buffer);      
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
        printf("\n%s\n",buffer);
    #endif
    if(retcode == LOGUNS)
        return -1;
return 0;}

int receivefile(int fhfc,char nof[]){
    FILE *archivito;
    int pathlen = strlen(DIRDESTFILES) + strlen(nof);
    char path[pathlen],buffer[FBUFFLEN] = {0};
    sprintf(path,"%s%s",DIRDESTFILES,nof);
    archivito = fopen(path,"wb");
    if(archivito == NULL){
        printf("** no se pudo abrir el archivo **\n");
        return -1;
    }
    while(1){
        fflush(archivito);
        if(read(fhfc,buffer,sizeof(buffer)) < 0){
            printf("** fallo la lectura del archivo en el socket de transferencia **\n");
            return -1;
        }
        if(strstr(buffer,"-1") != NULL){                    //llegue al fin de archivo
            strtok(buffer,"-1");
            fwrite(buffer,sizeof(char),sizeof(char) * strlen(buffer),archivito);
            break;
        }
        fwrite(buffer,sizeof(char),sizeof(char) * strlen(buffer),archivito);
    }
    fclose(archivito);
return 0;}