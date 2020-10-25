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
    struct prueba * next;
} prueba;

typedef struct prueba2 {
    int burst;
    int prioridad;
} prueba2;

int sockfd;
int * continuar;
prueba *head = NULL;
int minBurst = 1, maxBurst = 5, minWait = 3, maxWait = 8;
pthread_mutex_t lock;

void *enviar(void *vargp) 
{ 
    char buff[MAX]; 
    char hello[sizeof(prueba2)];
    
    int waiting =  (int)((rand() % 6) + 3);
        sleep(waiting);

    prueba *pr = (prueba *)vargp;
    int burst = pr->burst;
    int prioridad = pr->prioridad;
    prueba2 * pr2 = (prueba2 *)malloc(sizeof(prueba2));
    pr2->burst = burst;
    pr2->prioridad = prioridad;
    memcpy(hello, pr2, sizeof(prueba2));
    pthread_mutex_lock(&lock);
    write(sockfd, hello, sizeof(hello));
    printf("Burst:%d\nPrioridad:%d\n", burst, prioridad);
    //send(sockfd , hello , sizeof(hello) , 0);
    
    bzero(buff, sizeof(buff)); 
	read(sockfd, buff, sizeof(buff));
	printf("From Server: %s\n", buff); 
    fflush(stdout);
    pthread_mutex_unlock(&lock);  


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
    	prueba *temp = head;  
        if(head==NULL){
            head = (prueba *)malloc(sizeof(prueba));
            head->burst=burst_num;
            head->prioridad=prioridad_num;
            head->next=NULL;
            temp = head;
        }else{
            while(temp->next != NULL){
            temp = temp->next;
            }
            prueba *nuevo_nodo = (prueba *)malloc(sizeof(prueba));
            nuevo_nodo->burst=burst_num;
            nuevo_nodo->prioridad=prioridad_num;
            nuevo_nodo->next=NULL;
            temp->next = nuevo_nodo;
            temp = temp->next;
        }
    	pthread_t thread_id; 
    	pthread_create(&thread_id, NULL, enviar, (void *)temp);  
    	memset(burst, 0, sizeof(burst));
    	memset(prioridad, 0, sizeof(prioridad));
    	if (feof(archivo)){break;}
    
    }
    sleep(9);
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
    if (pthread_mutex_init(&lock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        exit(0); 
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
    pthread_mutex_destroy(&lock);
	// close the socket 
	close(sockfd); 
} 
