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

#define DIRDESTFILES "cliFiles/"            //DIRECTORIO DONDE SE VAN A ALMACENAR LOS ARCHIVOS TRANSFERIDOS
#define IP "127.0.0.1"                      //IP PARA EL CANAL DE TRANSFERENCIAS

int input2cmd(char[]);                      //extrae el comando de lo ingresado por el usuario
int autentication(int);                     //realiza toda la autenticación completa del usuario que quiere acceder
int newtransfersock(int*,int);              //crea socket de transferencia pero no devuelve al fhfs conectado con el servidor
int receivefile(int,char[]);
char* spacing(char[]);                      //devuelve la ip que se le pasa sin los puntos. para comando PORT
int* calculito(int);                        //devuelve los numeros a usar para que el sirvor calcule el puerto tras comando PORT.

int main(int argc,char *args[]){
    if(argc != 3){
        printf("** cantidad de argumentos incorrecta **\n");
        return -1;
    }
    int fhs,fhfs,fhfc,port,tport,saddrlen,retcode;
    struct sockaddr_in saddress;
    char buffer[BUFFLEN] = {0},input[BUFFLEN] = {0},cmd[5] = {0},nof[NOFLEN] = {0};
    //conexion con servidor mediante socket de comandos
    port = atoi(args[2]);
    saddress.sin_family = AF_INET;
    saddress.sin_addr.s_addr = inet_addr(args[1]);
    saddress.sin_port = htons(port);
    saddrlen = sizeof(saddress);
    if((fhs = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo en la creacion del socket de comandos **\n");
        return -2;
    }
    if(connect(fhs,(struct sockaddr*)&saddress,(socklen_t)saddrlen) < 0){
        printf("** fallo la conexion del socket de comandos **\n");
        return -3;
    }
    //interacciones entre cliente y servidor
    memset(buffer,0,sizeof(buffer));
    if(read(fhs,buffer,sizeof(buffer)) < 0){
        printf("** fallo la recepcion del mensaje **\n");
        return -4;
    }
    #ifdef DEB
        printf("\n%s\n",buffer);        //mensaje de bienvenida
    #endif
    if(autentication(fhs) < 0)
        return -5;
    while(true){                        //se generan y gestionan las peticiones al servidor
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
                    printf("\n%s\n",buffer);      
                #endif
                close(fhs);
                return 0;
            break;
            case GET:
                //creo socket de transferencias
                tport = port;
                if((fhfs = newtransfersock(&tport,fhs)) < 0){
                    printf("** no se pudo crear el canal de transferencias **\n");
                    return -8;
                }
                saddress.sin_family = AF_INET;
                saddress.sin_addr.s_addr = inet_addr(IP);
                saddress.sin_port = htons(tport);
                saddrlen = sizeof(saddress);
                //envio RETR, nombre de archivo y recibo tamaño
                strcpy(nof,input + 4);
                memset(buffer,0,sizeof(buffer));
                sprintf(buffer,"RETR %s\r\n",nof);
                if(write(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el enviado del comando **\n");
                    return -9;
                }
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo en la recepcion de la respuesta del servidor **\n");
                    return -10;
                }
                #ifdef DEB
                    getsockname(fhfc,(struct sockaddr*)&saddress,(socklen_t*)&saddrlen);
                    printf("\n%s\n",buffer);
                #endif
                //obtengo código de respuesta del archivo solicitado
                retcode = atoi(buffer);
                if(retcode == FILENF){
                    printf("* archivo no encontrado *\n\n");
                    close(fhfc);
                    continue;
                }
                else if(retcode == FILEFO){
                    //acepto al servidor en el socket de transferencias
                    if(listen(fhfs,3) < 0){
                        printf("** fallo el listen **\n");
                        return -11;
                    }
                    if((fhfc = accept(fhfs,(struct sockaddr*)&saddress,((socklen_t*)&saddrlen))) < 0){
                        printf("** fallo el accept **\n");
                        return -12;
                    }
                    #ifdef DEB
                        printf("servidor con ip '%s' conectado al canal de transferencias.\n\n",inet_ntoa(saddress.sin_addr));
                    #endif
                    //recibo el archivo 
                    if(receivefile(fhfc,nof) < 0)
                        return -13;
                    memset(buffer,0,sizeof(buffer));
                    if(read(fhs,buffer,sizeof(buffer)) < 0){
                        printf("** fallo en la recepcion de la respuesta del servidor **\n");
                        return -14;
                    }
                    #ifdef DEB
                        printf("%s\n",buffer);
                    #endif
                }
                else{
                    printf("** codigo de retorno del servidor inválido **\n");
                    close(fhfs); close(fhfc);
                    return -15;
                }
                close(fhfs); close(fhfc);
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

int newtransfersock(int *port,int fhs){
    int fhfs,saddrlen,newport,*forport;
    char buffer[BUFFLEN] = {0},*spacedip;
    struct sockaddr_in saddress;
    newport = *port + 1;
    saddress.sin_family = AF_INET;
    saddress.sin_addr.s_addr = inet_addr(IP);
    saddress.sin_port = htons(newport);
    saddrlen = sizeof(saddress);
    if((fhfs = socket(AF_INET,SOCK_STREAM,0)) < 0){         //creo el socket para transferencia de archivos
        printf("** fallo en la creacion del socket de escucha **\n");
        return -1;
    }
    if(bind(fhfs,(struct sockaddr*)&saddress,(socklen_t)saddrlen) < 0){
        //cambio el puerto
        newport = 0;
        saddress.sin_port = htons(newport);
        saddrlen = sizeof(saddress);
        if(bind(fhfs,(struct sockaddr*)&saddress,(socklen_t)saddrlen) < 0){
            //si falló ahora es porque hay otro error y aborto
            printf("** fallo el bind post cambiado de puerto **\n");
            return -1;
        }
        //aviso al servidor del cambio
        getsockname(fhfs,(struct sockaddr*)&saddress,(socklen_t*)&saddrlen);
        newport = ntohs(saddress.sin_port);
        #ifdef DEB
            printf("\npuerto %d ocupado por lo que cambiamos de puerto.\n",*port + 1);
            printf("nuevo puerto para la conexión es %d.\n",newport);
        #endif
        spacedip = spacing(inet_ntoa(saddress.sin_addr));
        forport = calculito(newport);
        if(spacedip == NULL || forport == NULL){
            printf("** fallo el formateo de la ip o del puerto para el comando PORT **\n");
            return -1;
        }
        sprintf(buffer,"PORT %s %d %d\r\n",spacedip,*(forport),*(forport+1));
        if(write(fhs,buffer,sizeof(buffer)) < 0){
            printf("** fallo el enviado del comando **\n");
            return -1;
        }
        free(spacedip); free(forport);
    }
    *port = newport;                    //guardo el puerto al que me conecté satisfactoriamente
return fhfs;}

int receivefile(int fhfc,char nof[]){
    FILE *archivito;
    int pathlen = sizeof(DIRDESTFILES) + strlen(nof);
    char path[pathlen],buffer[FBUFFLEN] = {0};
    sprintf(path,"%s%s",DIRDESTFILES,nof);
    archivito = fopen(path,"wb");
    if(archivito == NULL){
        printf("** no se pudo abrir el archivo **\n");
        return -1;
    }
    while(true){
        fflush(archivito);
        fsync(fhfc);
        if(read(fhfc,buffer,sizeof(buffer)) < 0){
            printf("** fallo la lectura del archivo en el socket de transferencia **\n");
            return -1;
        }
        if(strstr(buffer,"-EOF-") != NULL){                    //llegue al fin de archivo
            strtok(buffer,"-EOF-");
            fwrite(buffer,sizeof(char),sizeof(char) * strlen(buffer),archivito);
            break;
        }
        fwrite(buffer,sizeof(char),sizeof(char) * strlen(buffer),archivito);
        memset(buffer,0,sizeof(buffer));
    }
    fclose(archivito);
return 0;}

char* spacing(char ip[]){
    char *ret = (char*)malloc(INET_ADDRSTRLEN * sizeof(char));
    if(ret == NULL){
        printf("** fallo la asignacion de dinamic memory para el string **\n");
        return NULL;
    }
    for(int i = 0;ip[i] != '\0';++i){
        if(ip[i] != '.')
            *(ret + i) = ip[i];
        else
            *(ret + i) = ' ';
    }
    *(ret + (INET_ADDRSTRLEN - 1)) = '\0';
return ret;}

int* calculito(int port){
    int *ret = (int*)malloc(2 * sizeof(int));
    if(ret == NULL){
        printf("** fallo la asignacion de dinamic memory para los numeros**\n");
        return NULL;
    }
    *ret = port / 256;
    *(ret + 1) = port % 256;
return ret;}