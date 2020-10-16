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
    clock_t llegada;
    clock_t salida;
    struct node * next; 
} node_t;

static node_t *head = NULL;
static node_t *headend = NULL;

int connfd;
int *continuar;

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

void agregarALista(int pid, int burst, int prioridad) 
{    
    if(head == NULL){
        head = (node_t *)malloc(sizeof(node_t)); 
        head->pid = pid;
        head->burst = burst;
        head->prioridad = prioridad;
        head->llegada = clock();
        head->next = NULL;
    }else{
        node_t *temp = head;
        while(temp->next != NULL){
            temp = temp->next;
        }
        node_t *nuevo_nodo = (node_t *)malloc(sizeof(node_t)); 
        nuevo_nodo->pid = pid;
        nuevo_nodo->burst = burst;
        nuevo_nodo->prioridad = prioridad;
        nuevo_nodo->llegada = clock();
        nuevo_nodo->next = NULL;
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

            agregarALista(counter_pid, ptr.burst, ptr.prioridad);
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

void anadirAterminado(node_t * terminado){
    if(headend==NULL){
        headend = terminado;
    }else{
        node_t *temp = headend;
        while(temp->next!=NULL){
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
        float tat = (float)(headend->salida - headend->llegada) / CLOCKS_PER_SEC;
        totalTAT+=tat;
        float wt = tat - headend->burst;
        totalWT += wt;
        printf("PID:%d TurnAround Time: %f Waiting Time: %f",headend->pid,tat,wt);
        fflush(stdout);
    }
    printf("Promedios:\nTurnAround Time: %f\nWaiting Time: %f\n",totalTAT/cantProcesos,totalWT/cantProcesos);
    fflush(stdout);
}

void *fifo(){
    float oscioso, tiempo = 0;
    int ejecucion = 0;
    clock_t startOcioso, endOcioso;
    sleep(2);
    startOcioso = clock();
    while (*continuar) { 
        if(head!=NULL){
            printf("Ejecutando PID:%d Burst:%d prioridad:%d\n", head->pid, head->burst, head->prioridad);
            fflush(stdout);
            endOcioso = clock();
            tiempo = (float)(endOcioso - startOcioso) / CLOCKS_PER_SEC;
            oscioso+=tiempo;
            sleep(head->burst);
            ejecucion += head->burst;
            printf("Proceso %d Ejecutado\n",head->pid);
            fflush(stdout);
            startOcioso = clock();
            head->salida = startOcioso;
            //anadirAterminado(head);
            head = head->next;
        }
        if(*continuar == 0) break;
    }
    printf("Tiempo Ejecutando:%d  Tiempo Oscioso:%f\n",ejecucion, oscioso);
    fflush(stdout);
    //printList2();
}  

// Driver function 
int main() 
{ 
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
        printf("Seleccione el tipo de algoritmo");
        int algoritmo;
        scanf("%d", &algoritmo);
        pthread_t thread_id, thread2_id; 
        pthread_create(&thread_id, NULL, receptorProcesos, NULL);
        continuar = malloc(sizeof(int));
        *continuar = 1; 
        pthread_create(&thread2_id, NULL, fifo, NULL);
        char comprobar;
        while(comprobar = getchar()!='q'){
            if(comprobar == 'c'){
                pthread_t thread3_id; 
                pthread_create(&thread3_id, NULL, printList, NULL);
            }
        }
        *continuar = 0; 
        pthread_join(thread2_id, NULL);
    }
    // Function for chatting between client and server 
    

    // After chatting close the socket 
    close(sockfd); 
} 


