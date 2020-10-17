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


typedef struct prueba {
    int burst;
    int prioridad;
} prueba;

typedef struct node { 
    int pid;
    int burst;
    int prioridad;
    time_t llegada;
    time_t salida;
    struct node * next; 
} node_t;

static node_t *head = NULL;
static node_t *headend = NULL;

int connfd;
int continuar;
int bloquear_cola = 0;

void *printList(){

    node_t *temp = head;
    while(temp!= NULL){
        printf("%d\n", temp->pid);
        printf("%d\n", temp->burst);
        printf("%d\n", temp->prioridad);
        printf("\n");
        fflush(stdout);
        temp = temp->next;
    }

} 

void printList2(){

    node_t *temp = headend;
    while(temp!= NULL){
        printf("%d\n", temp->pid);
        printf("%d\n", temp->burst);
        printf("%d\n", temp->prioridad);
        printf("\n");
        fflush(stdout);
        temp = temp->next;
    }

} 

void agregarALista(int pid, int burst, int prioridad) {
    if(head == NULL){
        head = (node_t *)malloc(sizeof(node_t));
        head->next = NULL;
        head->pid = pid;
        head->burst = burst;
        head->prioridad = prioridad;
        head->llegada = time(NULL);
        
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
        temp->next = nuevo_nodo;
    }
} 

// Function designed for chat between client and server. 
void *receptorProcesos() 
{ 
    char buff[MAX]; 
    int n; 
    int counter_pid = 1;
    prueba ptr = {2,3};
    char* buffer[sizeof(prueba)]; 
    char hello[sizeof(prueba)];
    // infinite loop for chat 
    for (;;) { 
        bzero(buff, MAX); 
        memset(buffer, 0, sizeof(prueba));
        // read the message from client and copy it in buffer 

        int bytesrecv = read(connfd, buffer, sizeof(prueba)); 
        //recv(sockfd, buffer, sizeof(prueba), 0); 
        if (bytesrecv == 0) {   
            break; 
        }else{
            memcpy(&ptr, buffer, sizeof(prueba));
            printf("%d\n%d\n",ptr.burst,ptr.prioridad);
            fflush(stdout);

           while(bloquear_cola){};
            bloquear_cola = 1;
            agregarALista(counter_pid, ptr.burst, ptr.prioridad);
            bloquear_cola = 0;
            counter_pid++;
        }
        // print buffer which contains the client contents 
        /*printf("To client : "); 
        bzero(buff, MAX); 
        n = 0; 
        // copy server message in the buffer 
        while ((buff[n++] = getchar()) != '\n') 
            ; 

        // and send that buffer to client 
        write(sockfd, buff, sizeof(buff));
        //send(sockfd, buff, sizeof(buff), 0); 

        // if msg contains "Exit" then server exit and chat ended. 
        if (strncmp("exit", buff, 4) == 0) { 
            printf("Server Exit...\n"); 
            break; 
        } */
    }
} 

void anadirAterminado(node_t *terminado){
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

void imprimirTerminada(){
    int cantProcesos = 0;
    float totalTAT, totalWT;
    while(headend != NULL){
        cantProcesos++;
        int tat = (headend->salida - headend->llegada);
        totalTAT+=tat;
        int wt = tat - headend->burst;
        totalWT += wt;
        printf("PID:%d TurnAround Time: %d Waiting Time: %d\n",headend->pid,tat,wt);
        fflush(stdout);
        headend= headend->next;
    }
    printf("Promedios:\nTurnAround Time: %f\nWaiting Time: %f\n",totalTAT/cantProcesos,totalWT/cantProcesos);
    fflush(stdout);
}

void *hpf(){
    float oscioso, tiempo = 0;
    int ejecucion = 0;
    time_t startOcioso, endOcioso;
    startOcioso = time(NULL);
    while (continuar) { 
        while(bloquear_cola){}
        bloquear_cola = 1;
        if(head!=NULL){
            fflush(stdout);
            node_t *temp = head;
            node_t *eliminar = head;
            node_t *anterior_eliminar = NULL;
            int prioridad = head->prioridad;
            while(temp->next != NULL){
                if(temp->next->prioridad < prioridad){
                    prioridad = temp->next->prioridad;
                    eliminar = temp->next;
                    anterior_eliminar = temp;
                }
                temp=temp->next;
            }
            bloquear_cola = 0;
            printf("Ejecutando PID:%d Burst:%d prioridad:%d\n", eliminar->pid, eliminar->burst, eliminar->prioridad);
            fflush(stdout);
            endOcioso = time(NULL);
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            fflush(stdout);
            sleep(eliminar->burst);
            ejecucion += eliminar->burst;
            printf("Proceso %d Ejecutado\n",eliminar->pid);
            fflush(stdout);
            startOcioso = time(NULL);
            eliminar->salida = startOcioso;
            node_t *terminado = eliminar;
            if(anterior_eliminar!=NULL){
                anterior_eliminar->next = eliminar->next;
            }else{
                head = eliminar->next;
            }
            anadirAterminado(terminado);
        }
        bloquear_cola=0;
        if(continuar == 0) {endOcioso = time(NULL);
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            break;
        }
    }
    printf("Tiempo Ejecutando:%d  Tiempo Oscioso:%f\n",ejecucion, oscioso);
    fflush(stdout);
    imprimirTerminada();
}  

void *sjf(){
    float oscioso, tiempo = 0;
    int ejecucion = 0;
    time_t startOcioso, endOcioso;
    startOcioso = time(NULL);
    while (continuar) { 
        while(bloquear_cola){}
        bloquear_cola = 1;
        if(head!=NULL){
            fflush(stdout);
            node_t *temp = head;
            node_t *eliminar = head;
            node_t *anterior_eliminar = NULL;
            int burst = head->burst;
            while(temp->next != NULL){
                if(temp->next->burst < burst){
                    burst = temp->next->burst;
                    eliminar = temp->next;
                    anterior_eliminar = temp;
                }
                temp=temp->next;
            }
            bloquear_cola = 0;
            printf("Ejecutando PID:%d Burst:%d prioridad:%d\n", eliminar->pid, eliminar->burst, eliminar->prioridad);
            fflush(stdout);
            endOcioso = time(NULL);
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            fflush(stdout);
            sleep(eliminar->burst);
            ejecucion += eliminar->burst;
            printf("Proceso %d Ejecutado\n",eliminar->pid);
            fflush(stdout);
            startOcioso = time(NULL);
            eliminar->salida = startOcioso;
            node_t *terminado = eliminar;
            if(anterior_eliminar!=NULL){
                anterior_eliminar->next = eliminar->next;
            }else{
                head = eliminar->next;
            }
            anadirAterminado(terminado);
        }
        bloquear_cola=0;
        if(continuar == 0) {endOcioso = time(NULL);
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
            printf("Ejecutando PID:%d Burst:%d prioridad:%d\n", head->pid, head->burst, head->prioridad);
            fflush(stdout);
            endOcioso = time(NULL);
            tiempo = (float)(endOcioso - startOcioso);
            oscioso+=tiempo;
            sleep(head->burst);
            ejecucion += head->burst;
            printf("Proceso %d Ejecutado\n",head->pid);
            fflush(stdout);
            startOcioso = time(NULL);
            head->salida = startOcioso;
            node_t *terminado = head;
            head = head->next;
            anadirAterminado(terminado);
        }
        if(continuar == 0) {endOcioso = time(NULL);
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
    printf("Seleccione el tipo de algoritmo:\n");
    printf("FIFO: 1\n");
    printf("SJF: 2\n");
    printf("HPF: 3\n");
    printf("Round Robin: 4\n");
    int algoritmo;
    scanf("%d", &algoritmo);
    int sockfd, len; 
    struct sockaddr_in servaddr, cli; 

    // socket create and verification 
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 

    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 

    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 

    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else{
        printf("server acccept the client...\n"); 
        pthread_t thread_id, thread2_id; 
        pthread_create(&thread_id, NULL, receptorProcesos, NULL);
        continuar = 1; 
        pthread_create(&thread2_id, NULL, hpf, NULL);
        char comprobar;
        while(comprobar = getchar()!='q'){
            if(comprobar == 'c'){
                pthread_t thread3_id; 
                pthread_create(&thread3_id, NULL, printList, NULL);
            }
        }
        continuar = 0; 
        pthread_join(thread2_id, NULL);
    }
    // Function for chatting between client and server 
    

    // After chatting close the socket 
    close(sockfd); 
} 


