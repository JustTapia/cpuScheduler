#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <pthread.h> 
#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 


typedef struct prueba {//La estructura que recibe del socket
    int burst;
    int prioridad;
} prueba;

typedef struct node { //La estructura del proceso una vez en ready
    int pid;
    int burst;
    int prioridad;
    time_t llegada;
    time_t salida;//Para calcular el TAT
    int tiempo_ejecutado;
    struct node * next; 
} node_t;

static node_t *head = NULL;
static node_t *headend = NULL;

int connfd;
int continuar;
int bloquear_cola = 0;

void *printList(){//Imprime la lista de procesos en espera
    printf("Lista de procesos en cola:\n");
    printf("\n");
    fflush(stdout);
	while(bloquear_cola){}
    bloquear_cola = 1;
    node_t *temp = head;
    while(temp!= NULL){
    	printf("PID:%d Burst: %d Prioridad: %d\n",temp->pid,temp->burst,temp->prioridad);
        printf("\n");
        fflush(stdout);
        temp = temp->next;
    }
    bloquear_cola = 0;

} 

void agregarALista(int pid, int burst, int prioridad) {//Agrega el proceso a la lista de ready
    if(head == NULL){
        head = (node_t *)malloc(sizeof(node_t));
        head->next = NULL;
        head->pid = pid;
        head->burst = burst;
        head->prioridad = prioridad;
        head->llegada = time(NULL);
        head->tiempo_ejecutado = 0;
    }else{
        node_t *temp = head;
        while(temp->next != NULL){
            temp = temp->next;
        }
        node_t *nuevo_nodo = (node_t *)malloc(sizeof(node_t));
        nuevo_nodo->next = NULL;
        nuevo_nodo->pid = pid;
        nuevo_nodo->burst = burst;
        nuevo_nodo->prioridad = prioridad;
        nuevo_nodo->llegada = time(NULL);
        nuevo_nodo->tiempo_ejecutado = 0;
        temp->next = nuevo_nodo;
    }
} 

void *receptorProcesos() { //Este recibe los procesos del servidor
    int n; 
    int counter_pid = 1;
    prueba ptr = {2,3};
    char *recibido = "Recibido Proceso: ";
    char num_pid[5];
    char message_server[30];
    char* buffer[sizeof(prueba)]; 
    char hello[sizeof(prueba)];
    // infinite loop for chat 
    for (;;) { 
        memset(buffer, 0, sizeof(prueba));
        // read the message from client and copy it in buffer 

        int bytesrecv = read(connfd, buffer, sizeof(prueba)); //Recibe los datos en string
        //recv(sockfd, buffer, sizeof(prueba), 0); 
        if (bytesrecv == 0) {   
            break; 
        }else{
            memcpy(&ptr, buffer, sizeof(prueba));//Lo transforma a tipo prueba
            sprintf(num_pid, "%d", counter_pid);//Imprime los valores recibidos
            strcpy(message_server, recibido);
            strcat(message_server, num_pid);
        	write(connfd, message_server, sizeof(message_server));//Escrive el mensaje de recibido
            while(bloquear_cola){};
            bloquear_cola = 1;
            agregarALista(counter_pid, ptr.burst, ptr.prioridad);//Llamaa a agregar a la lista de ready
            bloquear_cola = 0;
            counter_pid++;
        }
    }
} 

void anadirAterminado(node_t *terminado){//Añade a la lista de procesos terminados
    terminado->next=NULL;
    if(headend==NULL){
        headend = terminado;
    }else{
        node_t *temp = headend;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = terminado;
    }
}

void imprimirTerminada(){//Imprime la lista de terminados al final de la ejecución
    int cantProcesos = 0;
    float totalTAT = 0.0;
    float totalWT = 0.0;
    while(headend != NULL){
        cantProcesos++;
        int tat = (headend->salida) - (headend->llegada);
        totalTAT+=tat;
        int wt = tat - headend->tiempo_ejecutado;
        totalWT += wt;
        printf("PID:%d TurnAround Time: %d Waiting Time: %d\n",headend->pid,tat,wt);
        fflush(stdout);
        headend= headend->next;
    }
    printf("Promedios:\nTurnAround Time: %f\nWaiting Time: %f\n",totalTAT/cantProcesos,totalWT/cantProcesos);//Imprime los promedios calculados
    fflush(stdout);
}

void *roundrobin(void *vargp){//algoritmo para la selección de round robin
	float oscioso, tiempo = 0;
    int ejecucion = 0;
    time_t startOcioso, endOcioso;
    startOcioso = time(NULL);
    int quantum = *((int *) vargp);
    while (continuar) { 
        if(head!=NULL){
            while(bloquear_cola){};//Espera a su turno para accesar a la lista
            bloquear_cola = 1;
            node_t *terminado = head//Quita el proceso de la lista de ready
            head = head->next;
            bloquear_cola = 0;
        	printf("Ejecutando PID:%d Burst:%d Prioridad:%d\n", terminado->pid, terminado->burst, terminado->prioridad);
            fflush(stdout);
        	if(terminado->burst > quantum){//Si el cuantum es menor que el burst que le queda
        		endOcioso = time(NULL);
            		tiempo = (float)(endOcioso - startOcioso);//Aumenta el TAT
            		oscioso+=tiempo;//Calcula el tiempo oscioso
            		sleep(quantum);//Duerme por la duración del cuantum
            		printf("Proceso %d Ejecutado por %d segundos\n",terminado->pid, quantum);
            		fflush(stdout);
            		startOcioso = time(NULL);
            		terminado->burst -= quantum;
        		ejecucion += quantum;
            		terminado->tiempo_ejecutado += quantum;
            		while(bloquear_cola){}
        			bloquear_cola = 1;
        			node_t *temp = head;
                	if(temp!=NULL){//Mueve el proceso ejecutado al final de la lista
            	    	while(temp->next != NULL){
                	   	temp=temp->next;
            	    	}
            	    	temp->next = terminado;
            	    	temp->next->next = NULL;
                	}else{
                    		head = terminado;
                    	head->next=NULL;
                	}
            		bloquear_cola = 0;
        	}else{//Si el burst es menor o igual al quantum
        		endOcioso = time(NULL);
            	tiempo = (float)(endOcioso - startOcioso);
            	oscioso+=tiempo;
            	sleep(terminado->burst);
            	printf("Proceso %d Ejecutado\n",terminado->pid);
            	fflush(stdout);
            	startOcioso = time(NULL);
            	terminado->tiempo_ejecutado += terminado->burst;
        		ejecucion += terminado->burst;
            	terminado->salida = startOcioso;
            	anadirAterminado(terminado);//Lo agrega a la lista de terminados

        	}
        }
        if(continuar == 0) {endOcioso = time(NULL);//Se fija se debe continuar ejecutando procesos
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            break;
        }
    }
    printf("Tiempo Ejecutando:%d  Tiempo Oscioso:%f\n",ejecucion, oscioso);//Muestra el tiempo de ejecución total
    fflush(stdout);
    imprimirTerminada();

}

void *hpf(){
    float oscioso, tiempo = 0;
    int ejecucion = 0;
    time_t startOcioso, endOcioso;
    startOcioso = time(NULL);
    while (continuar) { 
        while(bloquear_cola){}//Espera a su turno de usar la lista de procesos
        bloquear_cola = 1;
        if(head!=NULL){
            node_t *temp = head;
            node_t *eliminar = head;
            node_t *anterior_eliminar = NULL;
            int prioridad = head->prioridad;
            while(temp->next != NULL){//Se fija a ver cuál es la menor prioridad de la lista
                if(temp->next->prioridad < prioridad){
                    prioridad = temp->next->prioridad;
                    eliminar = temp->next;
                    anterior_eliminar = temp;
                }
                temp=temp->next;
            }
            if(anterior_eliminar!=NULL){
                anterior_eliminar->next = eliminar->next;
            }else{
                head = eliminar->next;
            }
            bloquear_cola = 0;
            printf("Ejecutando PID:%d Burst:%d Prioridad:%d\n", eliminar->pid, eliminar->burst, eliminar->prioridad);
            fflush(stdout);
            endOcioso = time(NULL);//Ayuda a calcular el tiempo oscioso
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            fflush(stdout);
            sleep(eliminar->burst);//Ejecuta el burst total del proceso
            ejecucion += eliminar->burst;
            printf("Proceso %d Ejecutado\n",eliminar->pid);
            fflush(stdout);
            startOcioso = time(NULL);//Ayuda a calcular el tiempo ocioso y el TAT del proceso proceso
            eliminar->salida = startOcioso;
            eliminar->tiempo_ejecutado = eliminar->burst;
            anadirAterminado(eliminar);//Lo añade a la lista de procesos terminados
        }
        bloquear_cola=0;
        if(continuar == 0) {endOcioso = time(NULL);//Se fija a ver si debe terminar
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            break;
        }
    }
    printf("Tiempo Ejecutando:%d  Tiempo Oscioso:%f\n",ejecucion, oscioso);
    fflush(stdout);
    imprimirTerminada();//Imprime la lista de terminados
}  

void *sjf(){
    float oscioso, tiempo = 0;
    int ejecucion = 0;
    time_t startOcioso, endOcioso;
    startOcioso = time(NULL);
    while (continuar) { 
        while(bloquear_cola){}//Espera a su turno de usar la lista de procesos
        bloquear_cola = 1;
        if(head!=NULL){
            node_t *temp = head;
            node_t *eliminar = head;
            node_t *anterior_eliminar = NULL;
            int burst = head->burst;
            while(temp->next != NULL){//Se fija a ver cuál es la menor prioridad de la lista
                if(temp->next->burst < burst){
                    burst = temp->next->burst;
                    eliminar = temp->next;
                    anterior_eliminar = temp;
                }
                temp=temp->next;
            }
            if(anterior_eliminar!=NULL){
                anterior_eliminar->next = eliminar->next;
            }else{
                head = eliminar->next;
            }
            bloquear_cola = 0;
            printf("Ejecutando PID:%d Burst:%d Prioridad:%d\n", eliminar->pid, eliminar->burst, eliminar->prioridad);
            fflush(stdout);
            endOcioso = time(NULL);//Ayuda a calcular el tiempo oscioso
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            fflush(stdout);
            sleep(eliminar->burst);
            ejecucion += eliminar->burst;
            printf("Proceso %d Ejecutado\n",eliminar->pid);
            fflush(stdout);
            startOcioso = time(NULL);//Ayuda a calcular el tiempo ocioso y el de proceso
            eliminar->salida = startOcioso;
            eliminar->tiempo_ejecutado = eliminar->burst; 
            
            anadirAterminado(eliminar);//Lo añade a la lista de procesos terminados
        }
        bloquear_cola=0;
        if(continuar == 0) {endOcioso = time(NULL);//Se fija a ver si debe terminar
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            break;
        }
    }
    printf("Tiempo Ejecutando:%d  Tiempo Oscioso:%f\n",ejecucion, oscioso);
    fflush(stdout);
    imprimirTerminada();
}  

void *fifo(){
    float oscioso, tiempo = 0;
    int ejecucion = 0;
    time_t startOcioso, endOcioso;
    startOcioso = time(NULL);
    while (continuar) { 
        if(head!=NULL){
            while (bloquear_cola){};
            bloquear_cola = 1;
            node_t *terminado = head;//Agarra el primer proceso y lo ejecuta
            head = head->next;
            bloquear_cola = 0;
            printf("Ejecutando PID:%d Burst:%d Prioridad:%d\n", terminado->pid, terminado->burst, terminado->prioridad);
            fflush(stdout);
            endOcioso = time(NULL);//Ayuda a calcular el tiempo oscioso
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            sleep(terminado->burst);
            ejecucion += terminado->burst;
            printf("Proceso %d Ejecutado\n",terminado->pid);
            fflush(stdout);
            startOcioso = time(NULL);//Ayuda a calcular el tiempo oscioso y el TAT de los procesos
            terminado->salida = startOcioso;
            terminado->tiempo_ejecutado = terminado->burst;
            anadirAterminado(terminado);//Lo añade a la lista de procesos terminados
        }
        if(continuar == 0) {endOcioso = time(NULL);//Se fija a ver si debe terminar
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            break;
        }
    }
    printf("Tiempo Ejecutando:%d  Tiempo Oscioso:%f\n",ejecucion, oscioso);
    fflush(stdout);
    imprimirTerminada();
}  

// Driver function 
int main() 
{
	int *quantum; 
    printf("Seleccione el tipo de algoritmo:\n");
    printf("FIFO: 1\n");
    printf("SJF: 2\n");
    printf("HPF: 3\n");
    printf("Round Robin: 4\n");
    int algoritmo;
    scanf("%d", &algoritmo);
    if(algoritmo == 4){
    	printf("Ingrese el quantum para el Round Robin: \n");
    	quantum = malloc(sizeof(int));
    	scanf("%d", quantum); 
    }

    int sockfd, len; 
    struct sockaddr_in servaddr, cli;

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);//Crea el server 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 

    // Asigna IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 

    // Hace el bind del socket 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 

    // El socket está listo para escuchar 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 

    // Acepta y verifica el paquete de datos del cliente 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else{
        printf("server acccept the client...\n"); 
        pthread_t thread_id, thread2_id;
        continuar = 1; 
      	pthread_create(&thread_id, NULL, receptorProcesos, NULL); //Recibe los procesos
        switch(algoritmo){//Switch para ver cual algoritmo ejecuta
        	case 1:
        		pthread_create(&thread2_id, NULL, fifo, NULL);
        		break;
        	case 2:
        		pthread_create(&thread2_id, NULL, sjf, NULL);
        		break;
        	case 3:
        		pthread_create(&thread2_id, NULL, hpf, NULL);
        		break;
        	case 4:
        		pthread_create(&thread2_id, NULL, roundrobin, (void *)quantum);
        		break;

        }
        char comprobar = 'a';
        while(comprobar != 'q'){//Para poder hacer quit del algoritmo en cualquier momento
        	comprobar = getchar();
            if(comprobar == 'c'){
                pthread_t thread3_id; 
                pthread_create(&thread3_id, NULL, printList, NULL);
            }
        }
        continuar = 0; 
        pthread_join(thread2_id, NULL);//Espera a que el algoritmo termine para que todo se imprima
    }

    close(sockfd); 
} 


