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
#define NOFACCESS   "accesses/ftpusers.txt"     //NAME OF FILE ACCESS
#define DIRSRCFILES "srvFiles/"              //DIRECTORIO DONDE ESTAN LOS ARCHIVOS PARA TRANSFERIR      
#define IP "127.0.0.5"         

int buff2cmd(char[]);                           //extrae el comando del buffer leido
int autentication(int);                         //realiza toda la autenticación completa del usuario que quiere acceder
int validate(char[],char[]);                    //valida que el user y la psw estén en el archivo de acceso
long int fsize(char[]);                         //deuelve el tamaño del archivo solicitado
int sendfile(int,char[]);                       //envia el archivo

int main(int argc,char *args[]){
    if(argc != 2){
        printf("** cantidad de argumentos incorrecta **\n");
        return -1;
    }
    int fhs,fhc,fhfs,caddrlen,faddrlen,port;
    long int filesz;
    struct sockaddr_in caddress,faddress;
    char buffer[BUFFLEN] = {0},cmd[5] = {0},nof[NOFLEN] = {0},cIP[16] = {0},aux[10] = {0};
    port = atoi(args[1]);
    //conexion con cliente mediante socket de comandos
    caddress.sin_family = AF_INET;
    caddress.sin_addr.s_addr = inet_addr(IP);
    caddress.sin_port = htons(port);
    caddrlen = sizeof(caddress);
    if((fhs = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo en la creacion del socket **\n");
        return -2;
    }
    if(bind(fhs,(struct sockaddr*)&caddress,(socklen_t)caddrlen) < 0){
        printf("** fallo el bind **\ncodigo de error %d.\n",errno);
        return -3;
    }
    if(listen(fhs,3) < 0){
        printf("** fallo el listen **\n");
        return -4;
    }
    if((fhc = accept(fhs,(struct sockaddr*)&caddress,(socklen_t*)&caddrlen)) < 0){
        printf("** fallo el accept **\n");
        return -5;
    }
    strcpy(cIP,inet_ntoa(caddress.sin_addr));           //obtengo la IP del cliente
    //conexion con cliente mediante socket de transferencias
    faddress.sin_family = AF_INET;
    faddress.sin_addr.s_addr = inet_addr(cIP);
    faddress.sin_port = htons(port + 1);
    faddrlen = sizeof(faddress);
    if((fhfs = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo la creacion del socket de transferencia **\n");
        return -6;
    }
    if(connect(fhfs,(struct sockaddr*)&faddress,(socklen_t)faddrlen) < 0){
        //checkeo si el cliente me mando un cambio de puerto
        if(read(fhc,buffer,sizeof(buffer)) < 0){
            printf("** fallo la lectura del comando enviado por el cliente **\n");
            return -34;
        }
        strncpy(cmd,buffer,(size_t)4);
        if(buff2cmd(cmd) == PORT){
            strcpy(aux,buffer + 5);
            strtok(aux,"\r\n");
            port = atoi(aux);
            faddress.sin_port = htons(port);
            faddrlen = sizeof(faddress);
        }
        if(connect(fhfs,(struct sockaddr*)&faddress,(socklen_t)faddrlen) < 0){
            //si falló ahora es porque hay otro error y aborto
            printf("** fallo el connect de transferencia post cambio de puerto **\n");
            printf("puerto al que intentamos conectar fue %d\n",ntohs(faddress.sin_port));
            return -35;
        }
    }
    //interaccion entre servidor-cliente
    memset(buffer,0,sizeof(buffer));
    memset(cmd,0,sizeof(cmd));
    sprintf(buffer,"%d %s version %s\r\n",CONSUC,args[0],VERSION);
    if(write(fhc,buffer,sizeof(buffer)) < 0){           //mensaje de bienvenida
        printf("** fallo el enviado del mensaje de bienvenida **\n");
        return -8;
    }
    if(autentication(fhc) < 0){
        close(fhs);//esto se tiene que ir
        return -9;
    }
    while(1){                                           //gestion de las peticiones
        memset(buffer,0,sizeof(buffer));
        if(read(fhc,buffer,sizeof(buffer)) < 0){
            printf("** fallo la lectura del comando enviado por el cliente **\n");
            return -10;
        }
        strncpy(cmd,buffer,(size_t)4);
        switch(buff2cmd(cmd)){
            case QUIT:
                memset(buffer,0,sizeof(buffer));
                sprintf(buffer,"%d Goodbye.\r\n",CONFIN);
                if(write(fhc,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el envio de la respuesta al cliente **\n");
                    return -11;
                }
                close(fhc); close(fhs); close(fhfs);
                return 0;
            break;
            case RETR:
                //obtengo nombre de archivo y envio tamaño
                strcpy(nof,buffer + 5);
                strtok(nof,"\r\n");
                memset(buffer,0,sizeof(buffer));
                if((filesz = fsize(nof)) < 0){
                    sprintf(buffer,"%d %s: no such file or directory\r\n",FILENF,nof);
                    if(write(fhc,buffer,sizeof(buffer)) < 0){
                        printf("** fallo el envio de la respuesta al cliente **\n");
                        return -12;
                    }
                    continue;
                }
                sprintf(buffer,"%d file %s size %ld Bytes\r\n",FILEFO,nof,filesz);
                if(write(fhc,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el envio de la respuesta al cliente **\n");
                    return -13;
                }
                //proceso de enviado del achivo
                if(sendfile(fhfs,nof) < 0)
                    return -14;
                memset(buffer,0,sizeof(buffer));
                sprintf(buffer,"%d Transfer complete\r\n",TRASUC);
                if(write(fhc,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el envio de la respuesta al cliente **\n");
                    return -14;
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
    else if(strcmp(str,"PORT") == 0)
        return PORT;
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
    FILE *archivito = fopen(NOFACCESS,"r");
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
    int pathlen = strlen(DIRSRCFILES) + strlen(nof);
    char path[pathlen];
    long int sz;
    sprintf(path,"%s%s",DIRSRCFILES,nof);
    archivito = fopen(path,"r");
    if(archivito == NULL)
        return -1;
    fseek(archivito,0L,SEEK_END);
    sz = ftell(archivito);
    fclose(archivito);
return sz;}

int sendfile(int fhfs,char nof[]){
    FILE *archivito;
    int pathlen = strlen(DIRSRCFILES) + strlen(nof);
    char path[pathlen],buffer[FBUFFLEN] = {0};
    sprintf(path,"%s%s",DIRSRCFILES,nof);
    archivito = fopen(path,"rb");
    if(archivito == NULL){
        printf("** no se pudo abrir el archivo **\n");
        return -1;
    }
    while(!feof(archivito)){
        fflush(archivito);
        fread(buffer,sizeof(char),(size_t)FBUFFLEN,archivito);
        if(write(fhfs,buffer,sizeof(char) * strlen(buffer)) < 0){
            printf("** fallo el enviado del archivo al cliente **\n");
            fclose(archivito);
            return -1;
        }
    }
    if(write(fhfs,"-1",sizeof("-1")) < 0){                  //indico el fin de archivo
        printf("** fallo el enviado del EOF al cliente **\n");
        fclose(archivito);
        return -1;
    }
    fclose(archivito);
return 0;}