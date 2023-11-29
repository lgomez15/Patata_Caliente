#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern int errno;

#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define PUERTO 22603
#define TIMEOUT 6
#define TAM_BUFFER 512
#define MAX_LINE_LENGTH 100

void handler()
{
 printf("Alarma recibida \n");
}

char* readLineFromFile(FILE* file) {
    char* line = NULL;
    size_t lineLength = 0;
    ssize_t bytesRead;

    // Read a line from the file
    bytesRead = getline(&line, &lineLength, file);

    if (bytesRead == -1) {
        // End of file or error occurred
        free(line);
        return NULL;
    }

    // Remove newline character, if present
    if (bytesRead > 0 && line[bytesRead - 1] == '\n') {
        line[bytesRead - 1] = '\0';
    }

    // Allocate memory for the line and copy it
    char* result = (char*)malloc(strlen(line) + 1);
    if (result == NULL) {
        perror("Memory allocation failed");
        free(line);
        return NULL;
    }
    strcpy(result, line);

    return result;
}

int main(int argc, char* argv[]) {
    // Check for the correct number of command-line arguments
    if (argc != 4) {
        printf("Usage: %s server protocol filename\n", argv[0]);
        return 1;
    }

    // Extract the command-line arguments
    char* server = argv[1];
    char* protocol = argv[2];
    char* filename = argv[3];

    // Open the text file for reading
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Unable to open the file");
        return 1;
    }

    // Read lines from the file and process them
    char* line;
    while ((line = readLineFromFile(file)) != NULL) {
        // Process each line as needed
        printf("Server: %s, Protocol: %s, Line: %s\n", server, protocol, line);

        // Free the memory allocated for the line
        free(line);
    }

    // Close the file
    fclose(file);
    return 0;
}

void clientudp(char *server, char *protocol, char *filename)
{
	int i, errcode;
	int retry = RETRIES;		/* holds the retry count */
	int s;				/* socket descriptor */
	long timevar;                       /* contains time returned by time() */
	struct sockaddr_in myaddr_in;	/* for local socket address */
	struct sockaddr_in servaddr_in;	/* for server socket address */
	struct in_addr reqaddr;		/* for returned internet address */
	int	addrlen, n_retry;
	struct sigaction vec;
   	char buffer[TAM_BUFFER];
	char bufResp[TAM_BUFFER];
   	struct addrinfo hints, *res;


	// Extract the command-line arguments

		/* Create the socket. */
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		printf("Error al crear el socket cliente UDP\n");
		exit(1);
	}
	
    /* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	
			/* Bind socket to some local address so that the
		 * server can send the reply back.  A port number
		 * of zero will be used so that the system will
		 * assign any available port number.  An address
		 * of INADDR_ANY will be used so we do not have to
		 * look up the internet address of the local host.
		 */
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		printf("Error al hacer bind cliente UDP\n"); 
		exit(1);
	   }
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
			printf("Error al leer la direccion del socket cliente UDP\n");
            exit(1);
    }

            /* Print out a startup message for the user. */
    time(&timevar);
            /* The port number must be converted first to host byte
             * order before printing.  On most hosts, this is not
             * necessary, but the ntohs() call is included here so
             * that this program could easily be ported to a host
             * that does require it.
             */
    printf("Cliente UDP iniciado en el puerto %d\n", ntohs(myaddr_in.sin_port));

	/* Set up the server address. */
	servaddr_in.sin_family = AF_INET;
		/* Get the host information for the server's hostname that the
		 * user passed in.
		 */
      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
 	 /* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo (server, NULL, &hints, &res); 
    if (errcode != 0){
		printf("Error al obtener la direccion del servidor\n");
		exit(1);
      }
    else {
			/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	 }
     freeaddrinfo(res);
     /* puerto del servidor en orden de red*/
	 servaddr_in.sin_port = htons(PUERTO);

	/* Set up the signal handler. */
	vec.sa_handler = handler;
	vec.sa_flags = 0;
	sigemptyset(&vec.sa_mask);
    if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
            printf("Error al configurar la alarma\n");
            exit(1);
    }

	//send the name of the client
	if (sendto(s, server, strlen(server), 0, (struct sockaddr *) &servaddr_in, sizeof(struct sockaddr_in)) == -1) {
		printf("Error al enviar el nombre del cliente\n");
		exit(1);
	}

	//wait for server to send the port 
	if (recvfrom(s, buffer, TAM_BUFFER, 0, (struct sockaddr *) &servaddr_in, &addrlen) == -1) {
		printf("Error al recibir el puerto del servidor\n");
		exit(1);
	}

	//convert to buffer to string
	buffer[strlen(buffer)] = '\0';
	printf("Puerto del servidor: %s\n", buffer);

	
	//set server port 
	servaddr_in.sin_port = htons(atoi(buffer));

	printf("Puerto del servidor: %d\n", ntohs(servaddr_in.sin_port));

	//clean buffer
	memset(buffer, 0, TAM_BUFFER);

	//leer el archivo
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		printf("Error al abrir el archivo\n");
		exit(1);
	}

	// Read lines from the file and process them
	char* line;
	while ((line = readLineFromFile(file)) != NULL) {
		
		//copy line to buffer
		strcpy(buffer, line);
		strcat(buffer, "\r\n");

		//send message
		if (sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *) &servaddr_in, sizeof(struct sockaddr_in)) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to send message\n", argv[0]);
			exit(1);
		}


		// Wait for a reply
		//clean bufResp
		memset(bufResp, 0, TAM_BUFFER);

		n_retry = 0;
		do {
			alarm(TIMEOUT);
			if (recvfrom(s, bufResp, TAM_BUFFER, 0, (struct sockaddr *) &servaddr_in, &addrlen) == -1) {
				if (errno == EINTR) {
					n_retry++;
					printf("%s\n", bufResp);
					fflush(stdout);
				} else {
					perror(argv[0]);
					fprintf(stderr, "%s: unable to receive message\n", argv[0]);
					exit(1);
				}
			} else {
				n_retry = 0;
			}
		} while (n_retry != 0 && n_retry < RETRIES);

		if (n_retry == 0) {
			alarm(0);
			if(strcmp(bufResp, "S:221 Cerrando el servicio") == 0)
				{
					printf("%s\n", bufResp);
					exit(0);
				}
			printf("%s\n", bufResp);
		} else {
			printf("No response from server\n");
		}
	}
}