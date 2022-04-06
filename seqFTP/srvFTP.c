#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include"cmds.h"

#define VERSION     "1.0"
#define IP          "127.0.0.25"
#define NOFACCESS   "accesses/ftpusers.txt"     //NAME OF FILE ACCESS
#define SOURCEFILES "srvFiles/"                 //DIRECTORIO DONDE ESTAN LOS ARCHIVOS PARA TRANSFERIR                     

int buff2cmd(char[]);
int autentication(int);
int validate(char[],char[]);
long int fsize(char[]);

int main(int argc,char *args[]){
    if(argc != 2){
        printf("** cantidad de argumentos incorrecta **\n");
        exit(1);
    }
    int fhs,fhc,addrlen,port;
    long int filesz;
    struct sockaddr_in address;
    char buffer[BUFFLEN] = {0},cmd[5] = {0},nof[NOFLEN] = {0};
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
        close(fhs);//esto se tiene que ir
        return -6;
    }
    while(1){                                           //gestion de las peticiones
        memset(buffer,0,sizeof(buffer));
        if(read(fhc,buffer,sizeof(buffer)) < 0){
            printf("** fallo la lectura del comando enviado por el cliente **\n");
            return -7;
        }
        strncpy(cmd,buffer,(size_t)4);
        switch(buff2cmd(cmd)){
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
            case RETR:
                strcpy(nof,buffer + 5);
                strtok(nof,"\r\n");
                memset(buffer,0,sizeof(buffer));
                if((filesz = fsize(nof)) < 0){
                    sprintf(buffer,"%d %s: no such file or directory\r\n",FILENF,nof);
                    if(write(fhc,buffer,sizeof(buffer)) < 0){
                        printf("** fallo el envio de la respuesta al cliente **\n");
                        return -9;
                    }
                }
                else{
                    sprintf(buffer,"%d file %s size %ld Bytes\r\n",FILEFO,nof,filesz);
                    if(write(fhc,buffer,sizeof(buffer)) < 0){
                        printf("** fallo el envio de la respuesta al cliente **\n");
                        return -10;
                    }
                    //PASAR AL ENVIADO DEL ARCHIVO
                }
            break;
        }
    }
return 0;}

int buff2cmd(char str[]){
    if(strcmp(str,"USER") == 0)
        return USER;
    else if(strcmp(str,"PASS") == 0)
        return PASS;
    else if(strcmp(str,"QUIT") == 0)
        return QUIT;
    else if(strcmp(str,"RETR") == 0)
        return RETR;
return -1;}

int autentication(int fhc){
    char buffer[BUFFLEN] = {0},cmd[5] = {0},user[AUTLEN] = {0},pass[AUTLEN] = {0},aux[AUTLEN] = {0};
    if(read(fhc,buffer,sizeof(buffer)) < 0){            //recibo usuario
        printf("** fallo la lectura del comando enviado por el cliente **\n");
        return -1;
    }
    strncpy(cmd,buffer,(size_t)4);                      //obtengo el comando del buffer
    if(buff2cmd(cmd) == USER){
        strncpy(aux,buffer + 5,(size_t)BUFFLEN);        //obtengo el usuario
        strcpy(user,strtok(aux,"\r\n"));                //le saco los escapes
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
        if(buff2cmd(cmd) == PASS){
            strncpy(aux,buffer + 5,(size_t)BUFFLEN);       //obtengo la contraseña
            strcpy(pass,strtok(aux,"\r\n"));
        } else {
            printf("** comando invalido **\n");
            return -1;
        }
        memset(buffer,0,sizeof(buffer));
        //actuo segun los datos obtenidos
        if(validate(user,pass) < 0){
            sprintf(buffer,"%d Login incorrect\r\n",LOGUNS);
            if(write(fhc,buffer,sizeof(buffer)) < 0){           //mensaje de error
                printf("** fallo el enviado del mensaje al cliente **\n");
                return -1;
            }
            close(fhc);
            return -1;
        } else {
            sprintf(buffer,"%d User %s logged in\r\n",LOGSUC,user);
            if(write(fhc,buffer,sizeof(buffer)) < 0){           //mensaje de error
                printf("** fallo el enviado del mensaje al cliente **\n");
                return -1;
            }
        }
    } else {
        printf("** comando invalido **\n");
        return -1;
    }
return 0;}

int validate(char user[],char pass[]){
    FILE *archivito = fopen(NOFACCESS");
    char read[AUTLEN + 1] = {0},aux[AUTLEN] = {0};
    if(archivito == NULL){
        printf("** error al abrir el archivo **\n");
        return -1;
    }
    //realizo el proceso de busqueda
    while(fscanf(archivito,"%[^:]",read) != EOF){
        if(strcmp(read,user) == 0){
            memset(read,0,sizeof(read));
            fscanf(archivito,"%s",read);
            strcpy(aux,read + 1);                           //porque read[1] == ":"
            if(strcmp(aux,pass) == 0)
                return 0;
            else
                return -1;
        }
        fscanf(archivito,"%s",read);                        //avanzo al puntero hacia el fin de línea
        fseek(archivito,1,SEEK_CUR);                        //avanzo al puntero una posición para que se posicione en la línea siguiente
        memset(read,0,sizeof(read));
    }
    fclose(archivito);
return -1;}

long int fsize(char nof[]){
    FILE *archivito;
    int pathlen = strlen(SOURCEFILES) + strlen(nof);
    char path[pathlen];
    long int sz;
    sprintf(path,"%s%s",SOURCEFILES,nof);
    archivito = fopen(path,"r");
    if(archivito == NULL)
        return -1;
    fseek(archivito,0L,SEEK_END);
    sz = ftell(archivito);
return sz;}