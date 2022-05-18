#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include"cmds.h"

#define VERSION     "1.0"
#define NOFACCESS   "accesses/ftpusers.txt"     //NAME OF FILE ACCESS     
#define IP "127.0.0.5"                          //IP PARA EL CANAL DE COMANDOS

int buff2cmd(char[]);                           //extrae el comando del buffer escrito
int autentication(int);                         //realiza toda la autenticación completa del usuario que quiere acceder
int validate(char[],char[]);                    //valida que el user y la psw estén en el archivo de acceso
int newtransfersock(int,char[]);                //crea socket de transferencia y devuelve el fhfc ya conectado al cliente
long int fsize(char[],char[]);                  //deuelve el tamaño del archivo solicitado
int sendfile(int,char[],char[]);                //envia el archivo
char* toip(char[]);                             //obtiene la ip del buffer cuando llega el comando port
int toport(char[]);                             //obtiene los numeros para calcular el puerto del buffer

int main(int argc,char *args[]){
    if(argc != 2){
        printf("** cantidad de argumentos incorrecta **\n");
        return -1;
    }
    int fhs,fhc,fhfs,caddrlen,port,tport;
    long int filesz;
    struct sockaddr_in caddress;
    char buffer[BUFFLEN] = {0},cmd[5] = {0},nof[NOFLEN] = {0},cIP[16] = {0},*aux,nod[NODLEN],
    dirsrcfiles[NODLEN * 3] = {0};
    bool ihadport,smbdyconn;
    FILE* sh;
    DIR* dir;
    ihadport = false;               //si recibo o no el comando PORT
    smbdyconn = false;              //si tengo clientes conectados en un determinado momento
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
        if(errno == 98){
            printf("** TIME WAIT state **\n");
            return -3;
        } else {
            printf("** fallo el bind **\n");
            return -4;
        }
    }
    if(listen(fhs,3) < 0){
        printf("** fallo el listen **\n");
        return -5;
    }
    while(true){
        if((fhc = accept(fhs,(struct sockaddr*)&caddress,(socklen_t*)&caddrlen)) < 0){
            printf("** fallo el accept **\n");
            return -6;
        }
        memset(dirsrcfiles,0,sizeof(dirsrcfiles));
        strcpy(dirsrcfiles,"srvFiles/");            //directorio de trabajo para pasar los archivos
        port = ntohs(caddress.sin_port);
        tport = port;
        #ifdef DEB
            printf("\n\e[7mse me conecto un cliente\e[0m\n");
        #endif
        smbdyconn = true;
        //interaccion entre servidor-cliente
        memset(buffer,0,sizeof(buffer));
        memset(cmd,0,sizeof(cmd));
        sprintf(buffer,"%d %s version %s\r\n",CONSUC,args[0],VERSION);
        if(write(fhc,buffer,sizeof(buffer)) < 0){           //mensaje de bienvenida
            printf("** fallo el enviado del mensaje de bienvenida **\n");
            return -7;
        }
        if(autentication(fhc) < 0)
            smbdyconn = false;
        while(smbdyconn){                                       //gestion de las peticiones
            memset(buffer,0,sizeof(buffer));
            if(read(fhc,buffer,sizeof(buffer)) < 0){
                printf("** fallo la lectura del comando enviado por el cliente **\n");
                return -9;
            }
            strncpy(cmd,buffer,(size_t)4);
            switch(buff2cmd(cmd)){
                case QUIT:
                    memset(buffer,0,sizeof(buffer));
                    sprintf(buffer,"%d Goodbye.\r\n",CONFIN);
                    if(write(fhc,buffer,sizeof(buffer)) < 0){
                        printf("** fallo el envio de la respuesta al cliente **\n");
                        return -10;
                    }
                    close(fhc); close(fhfs);
                    smbdyconn = false;
                break;
                case PORT:
                    strtok(buffer,"\r\n");
                    aux = toip(buffer+5);               //obtengo la ip del comando PORT
                    tport = toport(buffer+5);           //obtengo el puerto del comando PORT
                    if(aux == NULL || tport < 0){
                        printf("** fallo la extraccion de la ip o del puerto del comando PORT **\n");
                        return -10;
                    }
                    strcpy(cIP,aux);
                    free(aux);
                    ihadport = true;
                    memset(buffer,0,sizeof(buffer));
                    sprintf(buffer,"%d PORT command successful\r\n",PRTSUC);
                    if(write(fhc,buffer,sizeof(buffer)) < 0){
                        printf("** fallo el envio de la respuesta al cliente **\n");
                        return -11;
                    }
                break;
                case RETR:
                    //obtengo nombre de archivo y envio tamaño
                    strcpy(nof,buffer+5);
                    strtok(nof,"\r\n");
                    memset(buffer,0,sizeof(buffer));
                    if((filesz = fsize(nof,dirsrcfiles)) < 0){
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
                    getsockname(fhfs,(struct sockaddr*)&caddress,(socklen_t*)&caddrlen);
                    strcpy(cIP,inet_ntoa(caddress.sin_addr));           //obtengo la IP del cliente
                    sleep(1);                                           //espero para sincronizar
                    if(ihadport == false){                              //si no recibí antes el comando PORT
                        tport += 1;                                     //tport = port + 1, me conecto al puerto siguiente
                        if((fhfs = newtransfersock(tport,cIP)) < 0)
                            return -14;
                    }
                    else{                                               //si recibi antes el comando PORT
                        //tport ya va a estar seteado por el comando PORT
                        if((fhfs = newtransfersock(tport,cIP)) < 0)
                            return -15;
                        ihadport = false;
                    }
                    //proceso de enviado del achivo
                    if(sendfile(fhfs,nof,dirsrcfiles) < 0)
                        return -16;
                    memset(buffer,0,sizeof(buffer));
                    sprintf(buffer,"%d Transfer complete\r\n",TRASUC);
                    if(write(fhc,buffer,sizeof(buffer)) < 0){
                        printf("** fallo el envio de la respuesta al cliente **\n");
                        return -17;
                    }
                    close(fhfs);
                    tport = port;
                break;
                case CWD:
                    memset(nod,0,sizeof(nod));
                    strcpy(nod,(buffer+4));                          //obtengo el nombre del directorio
                    strtok(nod,"\r\n");
                    printf("nod = '%s'\n",nod);
                    if((strlen(dirsrcfiles) + strlen(nod) + 1) > sizeof(dirsrcfiles)){
                        memset(buffer,0,sizeof(buffer));
                        sprintf(buffer,"%d CWD command failed\r\n",CWDUNS);
                        if(write(fhc,buffer,sizeof(buffer)) < 0){
                            printf("** fallo el envio de la respuesta al cliente **\n");
                            return -17;
                        }
                        break;
                    }
                    aux = (char*)malloc(sizeof(dirsrcfiles) * sizeof(char) + 1);
                    if(aux == NULL){
                        printf("** fallo el malloc del CWD **\n\n");
                        return -18;
                    }
                    strcpy(aux,dirsrcfiles);                        //para chequear con aux si existe o no
                    concatdir(aux,nod);
                    if((dir = opendir(aux)) != NULL){               //el directorio existe
                        memset(dirsrcfiles,0,sizeof(dirsrcfiles));
                        strcpy(dirsrcfiles,aux);                    //como existe, guardo en la variable la info verdadera
                        if(closedir(dir) < 0)
                            printf("* no se pudo cerrar el directorio *\n");
                        memset(buffer,0,sizeof(buffer));
                        sprintf(buffer,"%d CWD command successful\r\n",CWDSUC);
                        if(write(fhc,buffer,sizeof(buffer)) < 0){
                            printf("** fallo el envio de la respuesta al cliente **\n");
                            return -19;
                        }
                    }
                    else if(dir == NULL && errno == ENOENT){         //el directorio no existe
                        memset(buffer,0,sizeof(buffer));
                        sprintf(buffer,"%d no directory\r\n",FILENF);
                        if(write(fhc,buffer,sizeof(buffer)) < 0){
                            printf("** fallo el envio de la respuesta al cliente **\n");
                            return -20;
                        }
                    }
                    else{                                           //el opendir falló
                        printf("** falló el opendir **\n");
                        return -21;
                    }
                    free(aux);
                break;
                case LIST:
                    printf("pwd = '%s'\n",dirsrcfiles);
                    memset(buffer,0,sizeof(buffer));
                    sprintf(buffer,"%d File listing follows in ASCII mode\r\n",DIRSUC);
                    if(write(fhc,buffer,sizeof(buffer)) < 0){
                        printf("** fallo el envio de la respuesta al ccomando dir **\n");
                        return -22;
                    }
                    aux = (char*)malloc((strlen(dirsrcfiles) + 5) * sizeof(char));
                    if(aux == NULL){
                        printf("** fallo el malloc del dir **\n\n");
                        return -23;
                    }
                    sprintf(aux,"ls -l %s",dirsrcfiles);            //para que me ejecute el ls en el current work directory
                    memset(buffer,0,sizeof(buffer));
                    sh = popen(aux,"r");
                    if(sh == NULL){
                        printf("* no se pudo ejecutar el comando en sh *\n\n");
                        return -24;
                    }
                    while(!feof(sh)){
                        memset(buffer,0,sizeof(buffer));
                        //uso caddrlen para reutilizar variables
                        caddrlen = fread(buffer,sizeof(char),sizeof(buffer),sh);
                        if(write(fhc,buffer,(size_t)caddrlen) < 0){
                            printf("** fallo el envio de la respuesta al comando dir **\n");
                            return -25;
                        }
                    }
                    memset(buffer,0,sizeof(buffer));
                    sprintf(buffer,"\n%d list completed successfully.\r\n",DIRCOM);
                    if(write(fhc,buffer,sizeof(buffer)) < 0){
                        printf("** fallo el envio de la respuesta al comando dir **\n");
                        return -25;
                    }
                    fclose(sh);
                    free(aux);
                break;
                case MKD:
                    memset(nod,0,sizeof(nod));
                    strcpy(nod,buffer+4);                           //obtengo el nombre del directorio a crear
                    strtok(nod,"\r\n");
                    printf("nod = '%s'\n",nod);
                    aux = (char*)malloc(sizeof(dirsrcfiles));
                    if(aux == NULL){
                        printf("** fallo el malloc del MKD **\n\n");
                        return -26;
                    }
                    strcpy(aux,dirsrcfiles);
                    concatdir(aux,nod);                             //obtengo el path del directorio a crear
                    printf("aux = '%s'\n",aux);
                    memset(buffer,0,sizeof(buffer));
                    if(mkdir(aux,0777) < 0){                        //no se pudo crear el directorio
                        sprintf(buffer,"%d '%s' creation failed\r\n",MKDUNS,aux);
                        if(write(fhc,buffer,sizeof(buffer)) < 0){
                            printf("** fallo el envio de la respuesta al cliente **\n");
                            return -20;
                        }
                    }
                    else{
                        sprintf(buffer,"%d '%s' created successfuly\r\n",MKDSUC,aux);
                        if(write(fhc,buffer,sizeof(buffer)) < 0){
                            printf("** fallo el envio de la respuesta al cliente **\n");
                            return -20;
                        }
                    }
                    free(aux);
                break;
                case RMD:

                break;
            }
        }
    }
return 0;}

int buff2cmd(char str[]){
    strtok(str," ");
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
    else if(strcmp(str,"CWD") == 0)
        return CWD;
    else if(strcmp(str,"LIST") == 0)
        return LIST;
    else if(strcmp(str,"MKD") == 0)
        return MKD;
    else if(strcmp(str,"RMD") == 0)
        return RMD;
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
    if(strcmp(user,"null") == 0 || strcmp(pass,"null") == 0)
        return -1;
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

int newtransfersock(int port,char ip[]){
    int fhfs,caddrlen;
    struct sockaddr_in caddress;
    caddress.sin_family = AF_INET;
    caddress.sin_addr.s_addr = inet_addr(ip);
    caddress.sin_port = htons(port);
    caddrlen = sizeof(caddress);
    if((fhfs = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("** fallo la creacion del socket de transferencia **\n");
        return -1;
    }
    if(connect(fhfs,(struct sockaddr*)&caddress,(socklen_t)caddrlen) < 0){
        printf("** fallo el connect del socket de transferencia **\n");
        return -1;
    }
    #ifdef DEB
        printf("\nme conecte a %s en puerto %d\n",inet_ntoa(caddress.sin_addr),ntohs(caddress.sin_port));
    #endif
return fhfs;}

long int fsize(char nof[],char dirsrcfiles[]){
    FILE *archivito;
    int pathlen = strlen(dirsrcfiles) + strlen(nof);
    char path[pathlen];
    long int sz;
    sprintf(path,"%s%s",dirsrcfiles,nof);
    archivito = fopen(path,"r");
    if(archivito == NULL)
        return -1;
    fseek(archivito,0L,SEEK_END);
    sz = ftell(archivito);
    fclose(archivito);
return sz;}

int sendfile(int fhfs,char nof[],char dirsrcfiles[]){
    FILE *archivito;
    int pathlen = strlen(dirsrcfiles) + strlen(nof),readed,writed;
    char path[pathlen],buffer[FBUFFLEN] = {0};
    sprintf(path,"%s%s",dirsrcfiles,nof);
    archivito = fopen(path,"rb");
    if(archivito == NULL){
        printf("** no se pudo abrir el archivo **\n");
        return -1;
    }
    while(!feof(archivito)){
        fflush(archivito);
        fsync(fhfs);
        readed = fread(buffer,sizeof(char),(size_t)FBUFFLEN,archivito);
        if((writed = write(fhfs,buffer,sizeof(char) * readed)) < 0){
            printf("** fallo el enviado del archivo al cliente **\n");
            fclose(archivito);
            return -1;
        }
        memset(buffer,0,sizeof(buffer));
    }
    fclose(archivito);
return 0;}

char* toip(char buffer[]){
    short int manycomas = 0,index = 0;
    char *ip = (char*)malloc(INET_ADDRSTRLEN * sizeof(char));
    if(ip == NULL){
        printf("** fallo la asignacion de dinamic memory para el string **\n");
        return NULL;
    }
    while(true){
        if(buffer[index] == ','){
            if(manycomas == 3)
                break;
            manycomas++;
            *(ip + index) = '.';
        }
        else 
            *(ip + index) = buffer[index];
        ++index;
    }
    *(ip + INET_ADDRSTRLEN - 1) = '\0';
return ip;}

int toport(char buffer[]){
    int port,manycomas,index,a,b;
    manycomas = 0; index = 0; a = 0; b = 0;
    while(true){    //127 0 0 1 145 61
        if(manycomas > 3){
            if(buffer[index] == ','){
                ++index;
                b = atoi(buffer+index);
                break;
            }
            else if(a == 0){
                a = atoi(buffer+index);
            }
            ++index;
            continue;
        }
        if(buffer[index] == ',')
            ++manycomas;
        ++index;
        if(index > BUFFLEN)
            return -1;
    }
    port = (a * 256) + b;
return port;}