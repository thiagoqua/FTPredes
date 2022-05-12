//COMANDOS PARA LA INTERACCION SERVIDOR-CLIENTE. LOS VALORES NO TIENEN RELEVANCIA.
#define QUIT    69
#define USER    70
#define PASS    71
#define GET     72
#define RETR    73
#define PORT    74
#define LCD     75
#define DIRLS   76
#define MKDIR   77
#define RMDIR   78

//CODIGOS DE RESPUESTA DEL SERVIDOR
#define CONSUC 220              //CONNECTION SUCCESSFUL
#define CONFIN 221              //CONNECTION FINISH
#define LOGSUC 230              //LOG IN SUCCESSFUL
#define PASREQ 331              //PASSWORD REQUIRED
#define LOGUNS 530              //LOG IN UNSUCCESSFUL
#define FILEFO 299              //FILE FOUND
#define FILENF 550              //FILE NOT FOUND
#define TRASUC 226              //TRANSFER SUCCESSFUL
#define PRTSUC 220              //PORT COMMAND SUCCESSFUL

//MACROS EXTRA
#define true     1
#define false    0
#define BUFFLEN  60             //longitud del buffer de comandos
#define FBUFFLEN 512            //longitud del buffer de transferencia de archivo
#define AUTLEN   20             //longitud de los arreglos de autenticacion
#define NOFLEN   20             //longitud del nombre del archivo para transferir
#define NODLEN   20

typedef short int bool;