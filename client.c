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


typedef struct prueba { //lista enlazada para agregar los procesos del archivo en el cliente manual
    int burst;
    int prioridad;
    struct prueba * next;
} prueba;

typedef struct prueba_simple { //estructura simple de los datos de un proceso
    int burst;
    int prioridad;
} prueba_simple;

int sockfd; //conexion del socket para enviar datos
int * continuar; //variable para saber si el usuario quiere terminar el programa (solo en cliente automatico)
prueba *head = NULL; //cabeza de la lista
int minBurst = 1, maxBurst = 5, minWait = 3, maxWait = 8; //valores de entrada para el cliente automatico
pthread_mutex_t lock; //lock entre threads para sincronizar el envio de datos al servidor 



//Enviar datos del cliente manual
void *enviar(void *vargp) 
{ 
    char buff[MAX]; 
    char hello[sizeof(prueba_simple)];
    
    int waiting =  (int)((rand() % 6) + 3); //tiempo de espera dentro del thread antes de enviar los datos
    sleep(waiting);

    //Sacamos los valores del proceso del parametro
    prueba *pr = (prueba *)vargp; 
    int burst = pr->burst;
    int prioridad = pr->prioridad;
    
    //Ponemos los valores del proceso en la estructura simple del proceso y enviarla
    prueba_simple * pr2 = (prueba_simple *)malloc(sizeof(prueba_simple));
    pr2->burst = burst;
    pr2->prioridad = prioridad;
    memcpy(hello, pr2, sizeof(prueba_simple)); //copiamos la informacion en un char array
    
    pthread_mutex_lock(&lock); //bloqueamos el envio para otros threads 
    write(sockfd, hello, sizeof(hello)); //enviar los datos
    printf("Burst:%d\nPrioridad:%d\n", burst, prioridad);
    
    bzero(buff, sizeof(buff)); 
    read(sockfd, buff, sizeof(buff)); //leer mensaje del servidor
    printf("From Server: %s\n", buff); 
    fflush(stdout);
    pthread_mutex_unlock(&lock);  //liberamos el lock


    return NULL;
}

void *automatico(){
    char *minBurstc, *maxBurstc, *minWaitc, *maxWaitc;
    int waiting;
    while(*continuar){

        //random bursts y prioridades segun los datos del usuario
        prueba_simple datos = {(rand() % (maxBurst - minBurst + 1)) + minBurst, (rand() % (4)) + 1}; 
        
        //creamos un thread para el proceso a enviar
        pthread_t thread_id; 
        pthread_create(&thread_id, NULL, enviar, (void *)&datos);

        //tiempo de espera entre creacion de procesos (segun datos del usuario)
        waiting =  (rand() % (maxWait - minWait + 1)) + minWait;
        sleep(waiting);
    }
}

void leerArchivo(){

    char tmp, burst[5], prioridad[5];
    int j = 0, burst_num, prioridad_num, waiting;
    FILE *archivo = fopen("procesos.txt", "r");

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
        
        //agregamos el proceso a una lista enlazada
        //la lista enlazada esta para que los procesos se envien en orden aleatorio
        //como el archivo se lee tan rapido, con la lista enlazada se asegura que no se reemplacen otros punteros
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

        //creamos un thread para enviar la info del proceso y le enviamos el nodo actual de la lista enlazada
        pthread_t thread_id; 
        pthread_create(&thread_id, NULL, enviar, (void *)temp);  
        
        //se resetean los valores del burst y prioridad
        memset(burst, 0, sizeof(burst));
        memset(prioridad, 0, sizeof(prioridad));

        //si alcanza el final del archivo, salir
        if (feof(archivo)){break;}
    
    }
    sleep(9); //esperar a que los hilos terminen
    fclose(archivo);
}


int main() 
{ 
    
	int connfd; 
	struct sockaddr_in servaddr, cli; 

    //modos del cliente
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
	
    //creacion del TCP socket 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 

    bzero(&servaddr, sizeof(servaddr)); 

	//asignar TCP, IP, Port
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	servaddr.sin_port = htons(PORT); 

	// Conectar al servidor
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		printf("connection with the server failed...\n"); 
		exit(0); 
    } 
    else
    {
        printf("connected to the server..\n");

        //cliente automatico 	   
        if (c == 'a'){
            continuar =(int *) malloc(sizeof(int));
            *continuar = 1;
            
            //thread al cliente automatico
            pthread_t thread_id; 
            pthread_create(&thread_id, NULL, automatico, NULL);
            
            //main thread se queda esperando si el usuario quiere salir
            while(getchar()!='q'){};
            *continuar = 0;
        
        }else{
            //cliente manual
            leerArchivo();
        }
    }

    //destruir el lock de los threads
    pthread_mutex_destroy(&lock);

	//cerrar el socket
	close(sockfd); 
} 