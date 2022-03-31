//COMANDOS PARA LA INTERACCION SERVIDOR-CLIENTE
#define QUIT 69
#define USER 70
#define PASS 71

//CODIGOS DE RESPUESTA DEL SERVIDOR
#define CONSUC 220              //CONNECTION SUCCESSFULL
#define CONFIN 221              //CONNECTION FINISH
#define LOGSUC 230              //LOG IN SUCCESSFULL
#define PASREQ 331              //PASSWORD REQUIRED
#define LOGUNS 530              //LOG IN UNSUCCESSFULL

//MACROS EXTRA
#define BUFFLEN 50              //longiud del buffer
#define AUTLEN 20               //longitud de los arreglos de autenticacion

int str2cmd(char str[]);

int str2cmd(char str[]){
    if(strcmp(str,"USER") == 0)
        return USER;
    else if(strcmp(str,"PASS") == 0)
        return PASS;
    else if(strcmp(str,"QUIT") == 0)
        return QUIT;
return -1;}