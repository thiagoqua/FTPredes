#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<termios.h>
#include<dirent.h>
#include"cmds.h"

#define IP "127.0.0.1"                      //IP PARA EL CANAL DE TRANSFERENCIAS

int input2cmd(char[]);                      //extrae el comando de lo ingresado por el usuario
bool validinputarg(char[]);                 //valida el argumento del comando que se ingresa
int autentication(int);                     //realiza toda la autenticación completa del usuario que quiere acceder
int newtransfersock(int*,int);              //crea socket de transferencia pero no devuelve al fhfs conectado con el servidor
int receivefile(int,char[],char[]);
char* fromip(char[]);                       //devuelve la ip que se le pasa sin los puntos. para comando PORT
int* fromport(int);                         //devuelve los numeros a usar para que el sirvor calcule el puerto tras comando PORT.
int exitconn(int);                          //terminar la conexión mediante comando QUIT

int main(int argc,char *args[]){
    if(argc != 3){
        printf("** cantidad de argumentos incorrecta **\n");
        return -1;
    }
    int fhs,fhfs,fhfc,port,tport,saddrlen,retcode;
    struct sockaddr_in saddress;
    char buffer[BUFFLEN] = {0},input[BUFFLEN] = {0},cmd[6] = {0},nof[NOFLEN] = {0},
    nod[NODLEN] = {0},dirdestfiles[NODLEN * 3] = {0},aux[NODLEN * 3] = {0};
    DIR* dir;
    strcpy(dirdestfiles,"cliFiles/");       //DIRECTORIO DONDE SE VAN A ALMACENAR LOS ARCHIVOS TRANSFERIDOS
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
    getsockname(fhs,(struct sockaddr*)&saddress,(socklen_t*)&saddrlen);
    port = ntohs(saddress.sin_port);
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
        memset(nod,0,sizeof(nod));
        memset(nof,0,sizeof(nof));
        memset(cmd,0,sizeof(cmd));
        printf("\noperación: ");
        fgets(input,sizeof(input),stdin);
        printf("\n");
        strtok(input,"\n");
        strncpy(cmd,input,(size_t)5);
        switch(input2cmd(cmd)){
            case QUIT:
                if(exitconn(fhs) < 0)
                    return -6;
                else
                    return 0;
            break;
            case GET:
                if(!validinputarg(input)){          //chequeo si sus argumentos son válidos
                    printf("* argumento del comando inválido *\n\n");
                    break;
                }
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
                    printf("\n%s\n",buffer);
                #endif
                //obtengo código de respuesta del archivo solicitado
                retcode = atoi(buffer);
                if(retcode == FILENF){
                    printf("\n* archivo no encontrado *\n\n");
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
                    if(receivefile(fhfc,nof,dirdestfiles) < 0)
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
            case CD:
                if(!validinputarg(input)){          //chequeo si sus argumentos son válidos
                    printf("* argumento del comando inválido *\n\n");
                    break;
                }
                strcpy(nod,(input+3));                          //obtengo el nombre del directorio
                sprintf(buffer,"CWD %s\r\n",nod);
                if(write(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el enviado del comando **\n\n");
                    return -16;
                }
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo la lectura del comando **\n\n");
                    return -17;
                }
                #ifdef DEB
                    printf("\n%s\n",buffer);
                #endif
                retcode = atoi(buffer);
                if(retcode == CWDSUC)
                    printf("directorio de trabajo cambiado en servidor de manera correcta!\n\n");
                else
                    printf("* no se pudo cambiar el directorio de trabajo en el servidor *\n\n");
            break;
            case LCD:
                if(!validinputarg(input)){          //chequeo si sus argumentos son válidos
                    printf("* argumento del comando inválido *\n\n");
                    break;
                }
                strcpy(nod,(input+4));                          //obtengo el nombre del directorio
                if((strlen(dirdestfiles) + strlen(nod)) > sizeof(dirdestfiles)){
                    printf("* no podemos movernos otro directorio más *\n");
                    break;
                }
                strcpy(aux,dirdestfiles);                       //para chequear con aux si existe o no
                concatdir(aux,nod);
                if((dir = opendir(aux)) != NULL){       //el directorio existe
                    memset(dirdestfiles,0,sizeof(dirdestfiles));
                    strcpy(dirdestfiles,aux);           //como existe, guardo en la variable la info verdadera
                    if(closedir(dir) < 0)
                        printf("* no se pudo cerrar el directorio *\n");
                    printf("\nworking directory now is '%s'\n\n",dirdestfiles);
                }
                else if(dir == NULL && errno == ENOENT)         //el directorio no existe
                    printf("\n%s: No such file or directory\n\n",nod);
                else{                                           //el opendir falló
                    printf("** falló el opendir **\n");
                    exitconn(fhs);
                    return -16;
                }
            break;
            case DIRLS:
                strcpy(buffer,"LIST\r\n");
                if(write(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el enviado del comando **\n\n");
                    return -16;
                }
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo la lectura del comando **\n\n");
                    return -17;
                }
                #ifdef DEB
                    printf("\n%s\n",buffer);
                #endif
                //recibo el comando 
                memset(buffer,0,sizeof(buffer));
                do{
                    if((retcode = read(fhs,buffer,sizeof(buffer))) < 0){
                        printf("** fallo la lectura de la respuesta del dir **\n\n");
                        return -18;
                    }
                    printf("%s",buffer);
                    memset(buffer,0,sizeof(buffer));
                }
                while(retcode == sizeof(buffer));
                printf("\n");
            break;
            case MKDIR:
                strcpy(nod,(input+6));                          //obtengo el nombre del directorio
                sprintf(buffer,"MKD %s\r\n",nod);
                if(write(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo el enviado del comando **\n\n");
                    return -19;
                }
                memset(buffer,0,sizeof(buffer));
                if(read(fhs,buffer,sizeof(buffer)) < 0){
                    printf("** fallo la lectura del comando **\n\n");
                    return -17;
                }
                #ifdef DEB
                    printf("\n%s\n",buffer);
                #endif
            break;
            case RMDIR:
                //no chequeo argumentos porque en tal caso que estén mal, rmdir vuelve < 0
                strcpy(nod,(input+6));                          //obtengo el nombre del directorio
                strcpy(aux,dirdestfiles);
                concatdir(aux,nod);                             //obtengo el path del directorio a borrar
                if(rmdir(aux) < 0){
                    if(errno == ENOTEMPTY)
                    printf("\ndirectorio '%s' no vacío. no se pudo borrar.\n\n",aux);
                    else
                    printf("\n* no se pudo borrar el directorio *\n\n");
                }
                else
                    printf("\ndirectorio '%s' borrado exitosamente!\n\n",aux);
            break;
            default:
                printf("* operación incorrecta. reingrese *\n\n");
            break;
        }
    }
return 0;}

int input2cmd(char str[]){
    strtok(str," ");
    if(strcmp(str,"quit") == 0)
        return QUIT;
    else if(strcmp(str,"get") == 0)
        return GET;
    else if(strcmp(str,"cd") == 0)
        return CD;
    else if(strcmp(str,"lcd") == 0)
        return LCD;
    else if(strcmp(str,"dir") == 0)
        return DIRLS;
    else if(strcmp(str,"mkdir") == 0)
        return MKDIR;
    else if(strcmp(str,"rmdir") == 0)
        return RMDIR;
return -1;}

bool validinputarg(char in[]){
    int len = strlen(in);
    bool cmdskipped = false;
    for(int i = 0;i < len;++i){
        if(in[i] >= 'a' && in[i] <= 'z' && !cmdskipped)
            continue;
        else if(in[i] == ' ' && !cmdskipped){
            cmdskipped = true;
            continue;
        }
        else if(in[i] != ' ' && in[i] != '\0' && in[i] != '\n'  && cmdskipped)
            return true;
    }
return false;}

int autentication(int fhs){
    int retcode;
    char input[AUTLEN],buffer[BUFFLEN] = {0};
    struct termios term;                                    //para ocultar la contraseña
    printf("username: ");
    fgets(input,sizeof(input),stdin);
    strtok(input,"\n");
    if(strcmp(input,"\n") == 0)
        strcpy(input,"null");
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
    //proceso de ocultado de escritura de contraseña
    tcgetattr(fileno(stdin),&term);
    term.c_lflag &= ~ECHO;
    tcsetattr(fileno(stdin),0,&term);
    fgets(input,sizeof(input),stdin);
    term.c_lflag |= ECHO;
    tcsetattr(fileno(stdin),0,&term);
    //se continúa
    strtok(input,"\n");
    if(strcmp(input,"\n") == 0)
        strcpy(input,"null");
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
        printf("\n\n%s\n",buffer);
    #endif
    if(retcode == LOGUNS){
        printf("** datos de login incorrectos **\n");
        return -1;
    }
return 0;}

int newtransfersock(int *port,int fhs){
    int fhfs,saddrlen,newport,*forport;
    char buffer[BUFFLEN] = {0},*forip;
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
        forip = fromip(inet_ntoa(saddress.sin_addr));
        forport = fromport(newport);
        if(forip == NULL || forport == NULL){
            printf("** fallo el formateo de la ip o del puerto para el comando PORT **\n");
            return -1;
        }
        sprintf(buffer,"PORT %s,%d,%d\r\n",forip,*(forport),*(forport+1));
        if(write(fhs,buffer,sizeof(buffer)) < 0){
            printf("** fallo el enviado del comando **\n");
            return -1;
        }
        memset(buffer,0,sizeof(buffer));
        if(read(fhs,buffer,sizeof(buffer)) < 0){
            printf("** fallo el enviado del comando **\n");
            return -1;
        }
        #ifdef DEB
            printf("\n%s\n",buffer);
        #endif
        free(forip); free(forport);
    }
    *port = newport;                    //guardo el puerto al que me conecté satisfactoriamente
return fhfs;}

int receivefile(int fhfc,char nof[],char dirdestfiles[]){
    FILE *archivito;
    int pathlen = strlen(dirdestfiles) + strlen(nof),readed;
    char path[pathlen],buffer[FBUFFLEN] = {0};
    readed = FBUFFLEN;                  //para que entre al while
    sprintf(path,"%s%s",dirdestfiles,nof);
    archivito = fopen(path,"wb");
    if(archivito == NULL){
        printf("** no se pudo abrir el archivo **\n");
        return -1;
    }
    while(readed == FBUFFLEN){
        fflush(archivito);
        fsync(fhfc);
        if((readed = read(fhfc,buffer,sizeof(buffer))) < 0){
            printf("** fallo la lectura del archivo en el socket de transferencia **\n");
            return -1;
        }
        fwrite(buffer,sizeof(char),sizeof(char) * readed,archivito);
        memset(buffer,0,sizeof(buffer));
    }
    fclose(archivito);
return 0;}

char* fromip(char ip[]){
    int i;
    char *ret = (char*)malloc(INET_ADDRSTRLEN * sizeof(char));
    if(ret == NULL){
        printf("** fallo la asignacion de dinamic memory para el string **\n");
        return NULL;
    }
    for(i = 0;ip[i] != '\0';++i){
        if(ip[i] != '.')
            *(ret + i) = ip[i];
        else
            *(ret + i) = ',';
    }
    for(i = 0;i < INET_ADDRSTRLEN;++i){
        if(*(ret + i) == ip[i] || *(ret + i) == ',')
            continue;
        else
            *(ret + i) = '\0';
    }
return ret;}

int* fromport(int port){
    int *ret = (int*)malloc(2 * sizeof(int));
    if(ret == NULL){
        printf("** fallo la asignacion de dinamic memory para los numeros**\n");
        return NULL;
    }
    *ret = port / 256;
    *(ret + 1) = port % 256;
return ret;}

int exitconn(int fhs){
    char buffer[BUFFLEN] = {0};
    strcpy(buffer,"QUIT\r\n");
    if(write(fhs,buffer,sizeof(buffer)) < 0){
        printf("** fallo el enviado del comando **\n");
        return -1;
    }
    memset(buffer,0,sizeof(buffer));
    if(read(fhs,buffer,sizeof(buffer)) < 0){
        printf("** fallo en la recepcion de la respuesta del servidor **\n");
        return -2;
    }
    #ifdef DEB
        printf("\n%s\n",buffer);      
    #endif
    close(fhs);
return 0;}