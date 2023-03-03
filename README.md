# Guía de ejecución
1. Compilar el servidor
`gcc srvFTP.c -o build/server`
2. Compilar el cliente
`gcc cliFTP.c -o build/client`
3. En una terminal, ejecutar el servidor. Como argumento le pasamos el puerto en donde va a estar escuchando clientes. Puede ser cualquiera superior a 1024.
`./build/server <puerto>`
4. En otra terminal, ejecutar el cliente. Como argumento le pasamos la dirección IP del servidor, que por defecto está en el localhost, y también el mismo puerto que le pasamos al servidor a la hora de ejecutarlo.
`./build/client 127.0.0.1 <puerto>`
5. Nos logueamos con username y password. Los datos de logueo se deben almacenar en el archivo acceses/ftpusers.txt de la forma "username":"password".
6. Realizamos las operaciones que queramos.
7. Salimos con el comando `quit`.
# Archivos a transferir
Los archivos que tiene disponible el servidor para transferir se deben almacenar en el directorio srvFiles.
Los archivos que son tranferidos, el cliente los almacena en el directorio cliFiles
# Comandos disponibles desde el lado cliente
- `quit` finaliza la conexión.
- `get <nombre de archivo>` transfiere un archivo desde el servidor.
- `cd <directorio>` cambia el directorio en el servidor.
- `dir` lista los archivos del directorio actual del servidor.
- `mkdir <nombre de directorio>` crea un directorio vacío en el directorio actual del servidor.
- `rmdir <nombre de directorio>` elimina el directorio en cuestión del lado del servidor.
