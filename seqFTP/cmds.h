//COMANDOS PARA LA INTERACCION SERVIDOR-CLIENTE
#define QUIT 69
#define USER 70
#define PASS 71
#define GET  72

//CODIGOS DE RESPUESTA DEL SERVIDOR
#define CONSUC 220              //CONNECTION SUCCESSFULL
#define CONFIN 221              //CONNECTION FINISH
#define LOGSUC 230              //LOG IN SUCCESSFULL
#define PASREQ 331              //PASSWORD REQUIRED
#define LOGUNS 530              //LOG IN UNSUCCESSFULL

//MACROS EXTRA
#define BUFFLEN 50              //longiud del buffer
#define AUTLEN 20               //longitud de los arreglos de autenticacion
#define NOFLEN 13               //longitud del nombre del archivo para pasar