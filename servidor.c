#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "message.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#define MAX_BUFFER 1024

int Abrir_Socket_UDP(int *puerto);
void Escribir_Puerto(int puerto);
int Abrir_Socket_TCP(int *puerto);

/* FUNCION MAIN DEL PROGRAMA SERVIDOR */
int main(int argc, char* argv[]){

   /* El esqueleto de la funcion principal muestra, de forma esquematica la secuencia 
   de operaciones y la correspondiente traza que se sugiere */

   /* Variables para UDP */
   int puerto_UDP;
   int socket_UDP;
   UDP_Msg mensaje_UDP;
   int tamano_mensaje_UDP;

   /* Variables para TCP */
   int puerto_TCP;
   int socket_TCP;
   
   /* Otras variables */
   struct sockaddr_in dir_cliente;
   char buffer[MAX_BUFFER]; /* Buffer para el servidor */
   socklen_t tamano_dir_cliente;
   int recibir;
   int mensaje_recibido;
   int aux_socket;
   int bytes_leidos;
   int bytes_escritos;
   int bytes_que_quedan;

   /* Realizamos todo el prodecidimiento para crea un socket UDP */
   socket_UDP = Abrir_Socket_UDP(&puerto_UDP);

   /* Escribimos el puerto de servicio */
   Escribir_Puerto(puerto_UDP);

   /* Realizamos todo el prodecidimiento para crea un socket TCP */
   socket_TCP = Abrir_Socket_TCP(&puerto_TCP);

   bzero((char *)&dir_cliente, sizeof(dir_cliente));
   tamano_dir_cliente = sizeof(dir_cliente);
   bzero(&mensaje_UDP, sizeof(UDP_Msg)); 
   tamano_mensaje_UDP = sizeof(UDP_Msg);

   /* Aceptamos conexiones por el socket */
   fprintf(stdout, "SERVIDOR: Aceptacion de peticiones: ");

   /* Comprobamos la aceptacion de peticiones */
   if(listen(socket_TCP, 100) < 0){
      fprintf(stdout, "ERROR\n");
      exit(1);
   }
   else{
      fprintf(stdout, "OK\n");
   }
   
   /* Puerto TCP ya disponible */
   fprintf(stdout, "SERVIDOR: Puerto TCP reservado: ");

   /* Comprobamos si el puerto_TCP esta disponible */
   if(puerto_TCP < 0){
      fprintf(stdout, "ERROR\n");
      exit(1);
   }
   else{
      fprintf(stdout, "OK\n");
   }

   while(1 /* Bucle de procesar peticiones */){

      fprintf(stdout, "SERVIDOR: Esperando mensaje.\n");

      /* Recibimos el mensaje */
      recibir = recvfrom(socket_UDP, (char *) &mensaje_UDP, tamano_mensaje_UDP, 0, (struct sockaddr *) &dir_cliente, &tamano_dir_cliente);

      /* Recibo mensaje */
      fprintf(stdout, "SERVIDOR: Mensaje del cliente: ");

      /* Comprobacion de si se ha recibido bien el mensaje */
      if(recibir < 0){
         fprintf(stdout, "ERROR\n");
         exit(1);
      }
      else{
         fprintf(stdout, "OK\n");
      }

      /* Si recibimos el mensaje QUIT */
      if(ntohl(mensaje_UDP.op) == QUIT){

         mensaje_UDP.op = htonl(OK);
         mensaje_recibido = sendto(socket_UDP, (char *) &mensaje_UDP, tamano_dir_cliente, 0, (struct sockaddr *) &dir_cliente, tamano_dir_cliente);
         fprintf(stdout,"SERVIDOR: QUIT\n");

         /* Envio del resultado */
         fprintf(stdout, "SERVIDOR: Enviando del resultado ");

         /* Comprobaciones del envio */
         if (mensaje_recibido < 0){
            mensaje_UDP.op = htonl(ERROR);
            fprintf(stdout, "[ERROR]: ERROR\n");
         }
         else{ 
            mensaje_UDP.op = htonl(OK);
            fprintf(stdout, "[OK]: OK\n");
         }

         close(socket_TCP); 
         close(socket_UDP); 
         break;
      }

      /* Si recibimos otro mensaje que no sea QUIT */
      else{

         /* Abrimos el mensaje recibido */
         mensaje_recibido = open(mensaje_UDP.remoto, O_RDONLY);

         fprintf(stdout, "SERVIDOR: REQUEST(%s,%s)\n", mensaje_UDP.local, mensaje_UDP.remoto);

         /* Envio del resultado */
         fprintf(stdout, "SERVIDOR: Enviando del resultado ");

         /* Comprobaciones del envio */
         if (mensaje_recibido < 0){
            mensaje_UDP.op = htonl(ERROR);
            fprintf(stdout, "[ERROR]: ERROR\n");
         }
         else{ 
            mensaje_UDP.op = htonl(OK);
            fprintf(stdout, "[OK]: OK\n");
         }

         mensaje_UDP.puerto = htons(puerto_TCP);
         sendto(socket_UDP, (char *) &mensaje_UDP, tamano_mensaje_UDP, 0, (struct sockaddr *) &dir_cliente, tamano_dir_cliente);

         /* Esperamos la llegada de una conexion */
         fprintf(stdout, "SERVIDOR: Llegada de un mensaje: ");

         /* Comprobaciones de la llegada del mensaje */
         if(mensaje_UDP.op == ERROR){
            fprintf(stdout, "ERROR\n");
            continue;
         }
         else{
            fprintf(stdout, "OK\n");
         }

         aux_socket = accept(socket_TCP, (struct sockaddr *) &dir_cliente, &tamano_dir_cliente);

         while ((bytes_leidos = read(mensaje_recibido, buffer, MAX_BUFFER)) > 0){
   
            bytes_que_quedan = bytes_leidos;

            while (bytes_leidos > 0) {

               bytes_escritos = write(aux_socket, buffer, bytes_leidos);

               if(bytes_escritos < 0){
                  if(bytes_que_quedan == bytes_leidos) {
                     exit(1);
                  }
                  else{
                     break;
                  }
               }
               else if( bytes_escritos == 0){
                  break;
               }
               else{
                  bytes_leidos = bytes_leidos - bytes_escritos;
                  *buffer = *buffer + bytes_escritos;
               }
            }
         }

         close(aux_socket);
         close(mensaje_recibido);
         bzero((char *) &dir_cliente, sizeof(dir_cliente));
         tamano_dir_cliente = sizeof(dir_cliente);
         bzero(&mensaje_UDP, sizeof(mensaje_UDP)); 
         tamano_mensaje_UDP = sizeof(mensaje_UDP);

      }
   }
  
   fprintf(stdout, "SERVIDOR: Finalizado\n");
   exit(0);
}

/* Funcion auxiliar que realiza la gestion y todos los procesos para
obtener el socket de tipo UDP */

int Abrir_Socket_UDP (int *puerto){

   /* Variables */
   int socket_UDP;
   struct sockaddr_in dir_servidor_UDP;
   socklen_t tamano_dir_UDP;
   int aux_bind;

   /* Tamaño de la direccion UDP */
   tamano_dir_UDP = sizeof(dir_servidor_UDP);

   /* Creacion del socket UDP */
   socket_UDP = socket(AF_INET, SOCK_DGRAM, 0);

   fprintf(stdout, "SERVIDOR: Creacion del socket UDP: ");

   /* Comprobacion de la creacion del socket */
   if(socket_UDP < 0){
      fprintf(stdout, "ERROR\n");
      exit(1);
   }
   else{
      fprintf(stdout, "OK\n");
   }

   /* Asignacion de la direccion local (del servidor) Puerto UDP */
   bzero((char *) &dir_servidor_UDP, sizeof(dir_servidor_UDP));

   dir_servidor_UDP.sin_family = AF_INET;
   dir_servidor_UDP.sin_addr.s_addr = inet_addr(HOST_SERVIDOR);
   dir_servidor_UDP.sin_port = htons(0);

   fprintf(stdout, "SERVIDOR: Asignacion del puerto servidor: ");

   /* Asociamos una direccion local */
   aux_bind = bind(socket_UDP, (struct sockaddr*) &dir_servidor_UDP, sizeof(dir_servidor_UDP));

   /* Comprobacion de la asociacion de una direccion local */
   if(aux_bind < 0){
      fprintf(stdout, "ERROR\n");
      close(socket_UDP);
      exit(1);
   }
   else{
      fprintf(stdout, "OK\n");
   }

   /* Ponemos la direccion del servidor a 0s */
   bzero((char *) &dir_servidor_UDP, sizeof(dir_servidor_UDP));

   /* Obtenemos la direccion */
   getsockname(socket_UDP, (struct sockaddr *) &dir_servidor_UDP, &tamano_dir_UDP);
   *puerto = ntohs((int) dir_servidor_UDP.sin_port);

   return socket_UDP;

}

/* Funcion auxiliar que escribe un numero de puerto en el fichero */

void Escribir_Puerto (int puerto){

   int fd;
  
   if((fd = creat(FICHERO_PUERTO,0660)) >= 0){
  
      write(fd,&puerto,sizeof(int));
      close(fd);
      fprintf(stdout,"SERVIDOR: Puerto guardado en fichero %s: OK\n",FICHERO_PUERTO);
   }
}

/* Funcion auxiliar que realiza la gestion y todos los procesos para
obtener el socket de tipo TCP */

int Abrir_Socket_TCP (int *puerto){

   /* Variables */
   int socket_TCP;
   struct sockaddr_in dir_servidor_TCP;
   socklen_t tamano_dir_TCP;
   int aux_bind;

   /* Tamaño de la direccion TCP */
   tamano_dir_TCP = sizeof(dir_servidor_TCP);

   /* Creacion del socket TCP */
   socket_TCP = socket(AF_INET, SOCK_STREAM, 0);

   fprintf(stdout, "SERVIDOR: Creacion del socket TCP: ");

   /* Comprobacion de la creacion del socket */
   if(socket_TCP < 0){
      fprintf(stdout, "ERROR\n");
      exit(1);
   }
   else{
      fprintf(stdout, "OK\n");
   }

   /* Asignacion de la direccion local (del servidor) Puerto TCP */
   bzero((char *) &dir_servidor_TCP, sizeof(dir_servidor_TCP));

   dir_servidor_TCP.sin_family = AF_INET;
   dir_servidor_TCP.sin_addr.s_addr = inet_addr(HOST_SERVIDOR);
   dir_servidor_TCP.sin_port = htons(0);

   fprintf(stdout, "SERVIDOR: Asignacion del puerto servidor: ");

   /* Asociamos una direccion local */
   aux_bind = bind(socket_TCP, (struct sockaddr*) &dir_servidor_TCP, sizeof(dir_servidor_TCP));

   /* Comprobacion de la asociacion de una direccion local */
   if(aux_bind < 0){
      fprintf(stdout, "ERROR\n");
      close(socket_TCP);
      exit(1);
   }
   else{
      fprintf(stdout, "OK\n");
   }

   /* Ponemos la direccion del servidor a 0s */
   bzero((char *) &dir_servidor_TCP, sizeof(dir_servidor_TCP));

   /* Obtenemos la direccion */
   getsockname(socket_TCP, (struct sockaddr *) &dir_servidor_TCP, &tamano_dir_TCP);
   *puerto = ntohs((int) dir_servidor_TCP.sin_port);

   return socket_TCP;

}
