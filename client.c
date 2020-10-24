#include <stdlib.h> 
#include <pthread.h> 
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <unistd.h> 
#include <arpa/inet.h> 
#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr  


typedef struct prueba {
    int burst;
    int prioridad;
} prueba;

int sockfd;
int * continuar;
int minBurst = 1, maxBurst = 5, minWait = 3, maxWait = 8;


void *enviar(void *vargp) 
{ 
    char buff[MAX]; 
    char hello[sizeof(prueba)];
    
    sleep(2);

    prueba *pr = (prueba *)vargp;
    int burst = pr->burst;
    int prioridad = pr->prioridad;

    memcpy(hello, pr, sizeof(prueba));
    write(sockfd, hello, sizeof(hello));
    printf("Burst:%d\nPrioridad:%d\n", burst, prioridad);
    //send(sockfd , hello , sizeof(hello) , 0);
    
    bzero(buff, sizeof(buff)); 
	read(sockfd, buff, sizeof(buff)); 
	printf("From Server: %s\n", buff); 
    fflush(stdout);


    //send(sock , hello , sizeof(hello) , 0);
    //printf("%d\n", burst); 
    //printf("%d\n", prioridad);
   /* printf("Message sent\n"); 
    valread = read( sock , buffer, 1024); 
    printf("%s\n",buffer );*/

    //memcpy(&ptr, hello, sizeof(hello));
     //printf("%d\n%d\n",ptr.b,ptr.p); 

    return NULL;
}

void *automatico(){
    char *minBurstc, *maxBurstc, *minWaitc, *maxWaitc;
    int waiting;
    while(*continuar){
        prueba datos = {(rand() % (maxBurst - minBurst + 1)) + minBurst, (rand() % (4)) + 1};
        //printf("\n");
        pthread_t thread_id; 
        pthread_create(&thread_id, NULL, enviar, (void *)&datos);
        waiting =  (rand() % (maxWait - minWait + 1)) + minWait; 
        sleep(waiting);
    }
}

void leerArchivo(){
    char tmp, burst[5], prioridad[5];
    int j = 0, burst_num, prioridad_num, waiting;
    int counter_pid;
    FILE *archivo = fopen("procesos.txt", "r");
    counter_pid = 1;
    while((tmp = fgetc(archivo)) != '\n'){ //primera linea
    }
    while(tmp != EOF){
    	j = 0;
    	while((tmp = fgetc(archivo)) != '\t' && !feof(archivo)){ //numero burst
    		*(burst+j) = tmp;
    		j++;
    	}
    	burst_num = atoi(burst);
    	j = 0;
    	while((tmp = fgetc(archivo)) != '\n' && !feof(archivo)){ //numero prioridad
    		*(prioridad+j) = tmp;
    		j++;
    	}
    	prioridad_num = atoi(prioridad);
    	prueba datos = {burst_num, prioridad_num};
    	pthread_t thread_id; 
    	pthread_create(&thread_id, NULL, enviar, (void *)&datos);
        waiting =  (int)((rand() % 6) + 3);
    	sleep(waiting);  
    	memset(burst, 0, sizeof(burst));
    	memset(prioridad, 0, sizeof(prioridad));
    	if (feof(archivo)){break;}
    
    }
    fclose(archivo);
}


int main() 
{ 
	int connfd; 
	struct sockaddr_in servaddr, cli; 
        printf("Desea el modo automatico(a) o Manual(m)?");
        char c = getchar();
        if(c == 'a'){
            printf("Elija el tiempo mínimo para el burst: ");
            scanf("%d", &minBurst);
            printf("\nElija el tiempo máximo para el burst: ");
            scanf("%d", &maxBurst);
            printf("\nElija el tiempo mínimo para la espera: ");
            scanf("%d", &minWait);
            printf("\nElija el tiempo máximo para la espera: ");
            scanf("%d", &maxWait);
    }
	// socket create and varification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	servaddr.sin_port = htons(PORT); 

	// connect the client socket to server socket 
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		printf("connection with the server failed...\n"); 
		exit(0); 
	} 
	else{
            printf("connected to the server..\n"); 	   
            if (c == 'a'){
            continuar =(int *) malloc(sizeof(int));
            *continuar = 1;
            pthread_t thread_id; 
            pthread_create(&thread_id, NULL, automatico, NULL);
            while(getchar()!='q'){}
            *continuar = 0;
        }else{
	       leerArchivo();
        }
    }

	// close the socket 
	close(sockfd); 
} 