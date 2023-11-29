/*
 *          		S E R V I D O R Y8322603K
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.  
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>



#define PUERTO 22603
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_BUFFER 516
#define MAXHOST 128
#define	STATE_INIT 101
#define STATE_WAIT_HOLA 102
#define STATE_JUGANDO 103
#define STATE_WAIT_ADIOS 104
#define STATE_DONE 105

char *preguntas[] = {
	"S:250 ¿Cuántos estados hay en Estados Unidos?",
	"S:250 ¿A cuánto está la prima de riesgo hoy?",
	"S:250 ¿Cuántos años tiene el rey?",
	"S:250 ¿Cuántos años tiene la reina?",
	"S:250 ¿Cuántos años tiene el principe?",
	"S:250 ¿Cuántos años tiene la princesa?",
	"S:250 ¿Cuantos balones de oro tiene Messi?",
	"S:250 ¿Cuantos balones de oro tiene Cristiano Ronaldo?",
	"S:250 ¿Cuantos balones de oro tiene Neymar?",
	"S:250 ¿Cuantos balones de oro tiene Iniesta?",
	"S:250 ¿Cuantos Grand Slam tiene Nadal?",
	"S:250 ¿Cuantos Grand Slam tiene Federer?",
	"S:250 ¿Cuantos Grand Slam tiene Djokovic?",
	"S:250 ¿En que año acabó la segunda guerra mundial?",
	"S:250 ¿En que año empezó la segunda guerra mundial?",
};

int respuestas[] = {
	50,
	100,
	150,
	200,
	250,
	300,
	350,
	400,
	450,
	50,
	100,
	5,
	20,
	4,
	3,
};

extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */
char *obtenerPreguntaAleatoria(int * pregunta);
char * analizadorSintactico(char *mensaje, int * pregunta, int *estado);
char* obtenerPregunta();
void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDPH(int s, struct sockaddr_in clientaddr_in, socklen_t client_len);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);
void errout(char *);		/* declare error out routine */

int FIN = 0;             /* Para el cierre ordenado */
void finalizar(){ FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

    int s_TCP, s_UDP;		/* connected socket descriptor */
    int ls_TCP;				/* listen socket descriptor */
    
    int cc;				    /* contains the number of bytes read */
     
    struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
    
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in clientaddr_in;	/* for peer socket address */
	int addrlen;
	
    fd_set readmask;
    int numfds,s_mayor;
    
    char buffer[BUFFERSIZE];	/* buffer for packets to be read into */
    
    struct sigaction vec;

		/* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

    addrlen = sizeof(struct sockaddr_in);

		/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
		/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
		/* Initiate the listen on the socket so remote users
		 * can connect.  The listen backlog is set to 5, which
		 * is the largest currently supported.
		 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}
	
	
	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	   }
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	    }

		/* Now, all the initialization of the server is
		 * complete, and any user errors will have already
		 * been detected.  Now we can fork the daemon and
		 * return to the user.  We need to do a setpgrp
		 * so that the daemon will no longer be associated
		 * with the user's control terminal.  This is done
		 * before the fork, so that the child will not be
		 * a process group leader.  Otherwise, if the child
		 * were to open a terminal, it would become associated
		 * with that terminal as its control terminal.  It is
		 * always best for the parent to do the setpgrp.
		 */
	setpgrp();

	switch (fork()) {
	case -1:		/* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:     /* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
		fclose(stdin);
		fclose(stderr);

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
		if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror(" sigaction(SIGCHLD)");
            fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
            exit(1);
            }
            
		    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
        vec.sa_handler = (void *) finalizar;
        vec.sa_flags = 0;
        if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGTERM)");
            fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
            exit(1);
            }
        
		while (!FIN) {
            /* Meter en el conjunto de sockets los sockets UDP y TCP */
            FD_ZERO(&readmask);
            FD_SET(ls_TCP, &readmask);
            FD_SET(s_UDP, &readmask);
            /* 
            Seleccionar el descriptor del socket que ha cambiado. Deja una marca en 
            el conjunto de sockets (readmask)
            */ 
    	    if (ls_TCP > s_UDP) s_mayor=ls_TCP;
    		else s_mayor=s_UDP;

            if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
                if (errno == EINTR) {
                    FIN=1;
		            close (ls_TCP);
		            close (s_UDP);
                    perror("\nFinalizando el servidor. Se�al recibida en elect\n "); 
                }
            }
           else { 

                /* Comprobamos si el socket seleccionado es el socket TCP */
                if (FD_ISSET(ls_TCP, &readmask)) {
                    /* Note that addrlen is passed as a pointer
                     * so that the accept call can return the
                     * size of the returned address.
                     */
    				/* This call will block until a new
    				 * connection arrives.  Then, it will
    				 * return the address of the connecting
    				 * peer, and a new socket descriptor, s,
    				 * for that connection.
    				 */
    			s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
    			if (s_TCP == -1) exit(1);
    			switch (fork()) {
        			case -1:	/* Can't fork, just exit. */
        				exit(1);
        			case 0:		/* Child process comes here. */
                    			close(ls_TCP); /* Close the listen socket inherited from the daemon. */
        				serverTCP(s_TCP, clientaddr_in);
        				exit(0);
        			default:	/* Daemon process comes here. */
        					/* The daemon needs to remember
        					 * to close the new accept socket
        					 * after forking the child.  This
        					 * prevents the daemon from running
        					 * out of file descriptor space.  It
        					 * also means that when the server
        					 * closes the socket, that it will
        					 * allow the socket to be destroyed
        					 * since it will be the last close.
        					 */
        				close(s_TCP);
        			}
             } /* De TCP*/
          /* Comprobamos si el socket seleccionado es el socket UDP */
          if (FD_ISSET(s_UDP, &readmask)) {
                /* This call will block until a new
                * request arrives.  Then, it will
                * return the address of the client,
                * and a buffer containing its request.
                * BUFFERSIZE - 1 bytes are read so that
                * room is left at the end of the buffer
                * for a null character.
                */

                //fork for UDP
				switch (fork()) {
					case -1:	/* Can't fork, just exit. */
						exit(1);
					case 0:	{	/* Child process comes here. */

						//get client address in clientaddr_in
						struct sockaddr_in clientaddr_in;
						socklen_t addrlen = sizeof(struct sockaddr_in);
						int nc = recvfrom(s_UDP, buffer, TAM_BUFFER, 0,
							(struct sockaddr *)&clientaddr_in, &addrlen);
						if (nc == -1) {
							perror("recvfrom");
							exit(1);
						}

						//close UDP socket inherited from the daemon
						close(s_UDP);

						//efimeral port for each client
						int s_UDP_child = socket(AF_INET, SOCK_DGRAM, 0);
						if (s_UDP_child == -1) {
							perror("unable to create socket UDP child");
							exit(1);
						}

						/* Enlazar la dirección del hijo al socket con un puerto efímero */
						struct sockaddr_in myaddr_in_child;
						memset((char *)&myaddr_in_child, 0, sizeof(struct sockaddr_in));
						myaddr_in_child.sin_family = AF_INET;
						myaddr_in_child.sin_addr.s_addr = INADDR_ANY;
						myaddr_in_child.sin_port = 0;

						if (bind(s_UDP_child, (struct sockaddr *) &myaddr_in_child, sizeof(struct sockaddr_in)) == -1) {
							perror("bind UDP child");
							exit(1);
						}

						/* Obtener el puerto efímero que fue elegido */
						socklen_t len = sizeof(myaddr_in_child);
						if (getsockname(s_UDP_child, (struct sockaddr *)&myaddr_in_child, &len) == -1) {
							perror("getsockname");
							exit(1);
						}
						printf("El sistema operativo eligió el puerto efímero: %d\n", ntohs(myaddr_in_child.sin_port));

						//string to send port 
						char port[10];
						sprintf(port, "%d", ntohs(myaddr_in_child.sin_port));
						
						//send port to client
						int nc2 = sendto(s_UDP_child, port, strlen(port), 0,
							(struct sockaddr *)&clientaddr_in, addrlen);

						serverUDPH(s_UDP_child,clientaddr_in,addrlen);
						//close UDP socket child
						close(s_UDP_child);
						exit(0);
					}
					default:	/* Daemon process comes here. */
							/* The daemon needs to remember
							 * to close the new accept socket
							 * after forking the child.  This
							 * prevents the daemon from running
							 * out of file descriptor space.  It
							 * also means that when the server
							 * closes the socket, that it will
							 * allow the socket to be destroyed
							 * since it will be the last close.
							 */
							break;
					}
                }
          }
		}   /* Fin del bucle infinito de atenci�n a clientes */
        /* Cerramos los sockets UDP y TCP */
        close(ls_TCP);
        close(s_UDP);
    
        printf("\nFin de programa servidor!\n");
        
	default:		/* Parent process comes here. */
		exit(0);
	}

}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;		/* keeps count of number of requests */
	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		/* remote host's name string */

	int len, len1, status;
    struct hostent *hp;		/* pointer to host info for remote host */
    long timevar;			/* contains time returned by time() */
    
    struct linger linger;		/* allow a lingering, graceful close; */
    				            /* used when setting SO_LINGER */
    				
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	 
     status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
     if(status){
           	/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
			 /* inet_ntop para interoperatividad con IPv6 */
            if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");
             }
    /* Log a startup message. */
    time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

		/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the
		 * data sent on it has been received by the remote host.
		 */
	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
					sizeof(linger)) == -1) {
		errout(hostname);
	}

		/* Go into a loop, receiving requests from the remote
		 * client.  After the client has sent the last request,
		 * it will do a shutdown for sending, which will cause
		 * an end-of-file condition to appear on this end of the
		 * connection.  After all of the client's requests have
		 * been received, the next recv call will return zero
		 * bytes, signalling an end-of-file condition.  This is
		 * how the server will know that no more requests will
		 * follow, and the loop will be exited.
		 */

	int estado = STATE_WAIT_HOLA;
	int pregunta = 0;
	while (len = recv(s, buf, TAM_BUFFER, 0) && estado != STATE_DONE) {
		if (len == -1) errout(hostname); 
		char * request = (char *)malloc(sizeof(char) * 100);
	char * response = NULL;
		printf("Received request number %d with length: %d and string %s\n", reqcnt, len, buf);

		// while ((buf[len-1] != '\n' && buf[len-2] != '\r') {
		// 	len1 = recv(s, &buf[len], TAM_BUFFER-len, 0);
		// 	if (len1 == -1) errout(hostname);
		// 	len += len1;
		// }

		strcpy(request,buf);
		printf("Request content:%s", request);
		
		
		response = analizadorSintactico(request,&pregunta,&estado);
		printf("\nRespuesta:%s", response);
		
		/* Increment the request count. */
		reqcnt++;
		
		sleep(1);

		/* Send a response back to the client. */
		if (send(s, response, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
		printf("\nSent response number %d currentSTATE: %d", reqcnt,estado);
		printf("\nResponse: %s", response);
	}

		/* The loop has terminated, because there are no
		 * more requests to be serviced.  As mentioned above,
		 * this close will block until all of the sent replies
		 * have been received by the remote host.  The reason
		 * for lingering on the close is so that the server will
		 * have a better idea of when the remote has picked up
		 * all of the data.  This will allow the start and finish
		 * times printed in the log file to reflect more accurately
		 * the length of time this connection was used.
		 */
	close(s);

		/* Log a finishing message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}


/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */

void serverUDPH(int sockfd, struct sockaddr_in client_addr, socklen_t client_len) {
    char buffer[TAM_BUFFER];
	char * response = (char *)malloc(sizeof(char) * 100);
	int estado = STATE_WAIT_HOLA;
	int pregunta = 0;
	printf("Entrando en el analizador\n");
    while (estado != STATE_DONE) {

        // Receive a message from the client
		printf("Esperando mensaje del cliente\n");
        ssize_t recv_len = recvfrom(sockfd, buffer, TAM_BUFFER, 0, (struct sockaddr *)&client_addr, &client_len);

        if (recv_len < 0) {
            perror("Error in receiving message");
            break;
        }

        buffer[recv_len] = '\0'; // Null-terminate the received data

        printf("Received message from %s:%d: %s\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
		

		response = analizadorSintactico(buffer,&pregunta,&estado);

		
        // Process the received message (you can customize this part)
        // Here, we simply echo back the received message to the client
        sendto(sockfd, response, strlen(response), 0,
               (const struct sockaddr *)&client_addr, client_len);
    }
	
	return;
}

void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in)
{
    struct in_addr reqaddr;	/* for requested host's address */
    struct hostent *hp;		/* pointer to host info for requested host */
    int nc, errcode;

    struct addrinfo hints, *res;

	int addrlen;
    
   	addrlen = sizeof(struct sockaddr_in);

      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
		/* Treat the message as a string containing a hostname. */
	    /* Esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta. */
    errcode = getaddrinfo (buffer, NULL, &hints, &res); 
    if (errcode != 0){
		/* Name was not found.  Return a
		 * special value signifying the error. */
		reqaddr.s_addr = ADDRNOTFOUND;
      }
    else {
		/* Copy address of host into the return buffer. */
		reqaddr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
     freeaddrinfo(res);

	nc = sendto (s, &reqaddr, sizeof(struct in_addr),
			0, (struct sockaddr *)&clientaddr_in, addrlen);
	if ( nc == -1) {
         perror("serverUDP");
         printf("%s: sendto error\n", "serverUDP");
         return;
         }   
 }

char *obtenerPreguntaAleatoria(int * pregunta) 
{

    // Calcula la cantidad de preguntas en el arreglo
    size_t numPreguntas = sizeof(preguntas) / sizeof(preguntas[0]);

    // Genera un índice aleatorio
    srand(time(NULL)); // Inicializa la semilla del generador de números aleatorios
    int indiceAleatorio = rand() % numPreguntas;
	*pregunta = indiceAleatorio;

    // Devuelve la pregunta seleccionada
    return preguntas[indiceAleatorio];
}

 //funcion para analizar la comunicacion cliente-servidor (ANALIZADOR SINTACTICO)
char *analizadorSintactico(char *mensaje, int *pregunta, int *estado) {
    char *respuesta = (char *)malloc(sizeof(char) * 100);
    char *aux = (char *)malloc(sizeof(char) * 100);

    if (respuesta == NULL || aux == NULL) {
        fprintf(stderr, "Error de reserva de memoria\n");
        exit(EXIT_FAILURE);  // Termina el programa en caso de error grave de memoria
    }

    char linea[100];  // Reserva espacio para almacenar la línea
    strcpy(linea, mensaje);
    printf("Mensaje: %s\n", linea);

    switch (*estado) {
        case STATE_WAIT_HOLA:
            printf("Estado: STATE_WAIT_HOLA\n");
            if (strcmp(linea, "HOLA\r\n") == 0) {
                aux = obtenerPreguntaAleatoria(pregunta);
                strcpy(respuesta, aux);
                *estado = STATE_JUGANDO;
                printf("RespuestaAS: %s\n", respuesta);
                return respuesta;
            } else {
                *estado = STATE_WAIT_HOLA;
                strcpy(respuesta, "S:500 Error de sintaxis");
                printf("Respuesta: %s\n", respuesta);
                return respuesta;
            }
            break;
        case STATE_JUGANDO:
            printf("Estado: STATE_JUGANDO\n");
            if (strncmp(mensaje, "RESPUESTA", 9) == 0) {
                printf("Sintaxis ""RESPUESTA"" correcta\n");
                int numero;
                if (sscanf(mensaje + 10, "%d", &numero) == 1) {
                    printf("Sintaxis ""RESPUESTA (n)"" correcta\n");
                    if (numero == respuestas[*pregunta]) {
                        *estado = STATE_WAIT_ADIOS;
                        strcpy(respuesta, "S:350 ACIERTO");
                        return respuesta;
                    } else {
                        // Comparar si mayor o menor
                        if (numero > respuestas[*pregunta]) {
                            *estado = STATE_JUGANDO;
                            strcpy(respuesta, "S:354 MENOR");
                            return respuesta;
                        } else {
                            *estado = STATE_JUGANDO;
                            strcpy(respuesta, "S:354 MAYOR");
                            return respuesta;
                        }
                    }
                } else {
                    *estado = STATE_JUGANDO;
                    strcpy(respuesta, "S:500 Error de sintaxis");
                    return respuesta;
                }
            } else {
                *estado = STATE_JUGANDO;
                strcpy(respuesta, "S:500 Error de sintaxis");
                return respuesta;
            }
            break;
        case STATE_WAIT_ADIOS:
            printf("Estado: STATE_WAIT_ADIOS\n");
            if (strcmp(linea, "ADIOS\r\n") == 0) {
                strcpy(respuesta, "S:221 Cerrando el servicio");
                *estado = STATE_DONE;
                return respuesta;
            } else if (strcmp(linea, "+\r\n") == 0) {
                *estado = STATE_JUGANDO;
                aux = obtenerPreguntaAleatoria(pregunta);
                strcpy(respuesta, aux);
                return respuesta;
            } else {
                *estado = STATE_WAIT_ADIOS;
                strcpy(respuesta, "S:500 Error de sintaxis");
                return respuesta;
            }
            break;
    }
}


