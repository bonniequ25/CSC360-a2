/*
*	Name: Bonnie Qu
*	StudentID: V00875831
*	CSC360 Assignment 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>


typedef struct train { // typedef to contains the train information
    int id;
    int priority;
    int loadTime;
    int crossTime;
    char direction;
} train;

typedef struct node_t { // node-t to conains the train information as node
    train* train;
    struct node_t* next;
} node_t;

typedef struct Queue { // use for build a queue and conains all train nodes
    struct node_t* head;
    struct node_t* tail;
    int size;
} Queue;

node_t* constructNode(train* currTrain) { // use for default the node
    node_t* node = (node_t*) malloc(sizeof (node_t));
    if(node == NULL){
        perror("Error: failed on malloc node");
        return NULL;
    }
    node -> train = currTrain;
    node -> next = NULL;
    return node;
}

Queue* constructQueue() { // use for default the Queue
    Queue* queue = (Queue*) malloc (sizeof (Queue));
    if(queue == NULL){
        perror("Error: failed on molloc queue");
        return NULL;
    }
    queue -> head = NULL;
    queue -> tail = NULL;
    queue -> size = 0;
    return queue;
}

void enQueue(Queue* equeue, node_t* newNode) { //add node into the Queue FIFO
    if(equeue -> size == 0){
        equeue -> head = newNode;
        equeue -> tail = newNode;
    }else{
        equeue -> tail -> next = newNode;
        equeue -> tail = newNode;
    }
    equeue -> size++;
}

void deQueue(Queue* dqueue) { // delete the node from the Queue
    if(dqueue -> size == 0){
        exit(-1);
    }else if(dqueue -> size == 1){
        dqueue -> head = NULL;
        dqueue -> tail = NULL;
        dqueue -> size--;
    }else{
        dqueue -> head = (dqueue -> head) -> next;
        dqueue -> size--;
    }
}

struct timeval init_time; // use for log the time
train trainArray[100]; // use the array for store the train thread
Queue* westQueue = NULL; // create a Queue for west train
Queue* eastQueue = NULL; // create a Queue for East train

// mutex and convar for support and manage the main track
pthread_mutex_t mainTrackLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mainTrackLock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t westQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t eastQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condVar = PTHREAD_COND_INITIALIZER;

// thread array use for keep all train thread on track
pthread_t trainThreads[100];
pthread_t trackThread;

double get_simulation_time() { // use for calculate out the time interval between trains
    struct timeval curr_time;
    double curr_seconds;
    double init_seconds;

    init_seconds = (init_time.tv_sec + (double) init_time.tv_usec / 1000000);
    gettimeofday(&curr_time, NULL);
    curr_seconds = (curr_time.tv_sec + (double) curr_time.tv_usec / 1000000);

    return (curr_seconds - init_seconds);
}

int readFileIn(char* file, char fileInformation[100][100]) { // use for read file and get data from local file
    FILE *fp = fopen(file, "r");
    int count = 0;
    if(fp != NULL){
        while(1){
            fgets(fileInformation[count++], 100, fp);
            if(feof(fp)) break;
        }
    }else{
        perror("Error: failed on read file.\n");
    }
    fclose(fp);
    return count - 1;
}

void tokenInformation(char fileInformation[100][100], int totalTrains) { // use for token the usefull information and store them into array
    int i = 0;
    for(; i<totalTrains; i++){
        int p = 0;
        if(fileInformation[i][0] == 'w' || fileInformation[i][0] == 'e'){
            p = 1;
        }
        train t = {
            i,
            p,
            atoi(&fileInformation[i][2]),
            atoi(&fileInformation[i][4]),
            (fileInformation[i][0])
        };
        trainArray[i] = t;
        //printf("%d %d %d %d %c\n", t.id, t.priority, t.loadTime, t.crossTime, t.direction);
    }
}

void* trainEntry(void* currTrain){ // main Track for load train and train passing the main track simulation function
    train* t = (train*) currTrain;
    usleep(t->loadTime * 100000);

    if(t->direction == 'e' || t->direction == 'E'){
        printf("%.2f Train %2d is ready to go East\n", get_simulation_time(), t->id);
    }else{
        printf("%.2f Train %d is ready to go West\n", get_simulation_time(), t->id);
    }


    if(t->direction == 'e' || t->direction == 'E'){
        //pthread_mutex_lock(&eastQueueLock);
        enQueue(eastQueue, constructNode(t));
    }else{
        //pthread_mutex_lock(&westQueueLock);
        enQueue(westQueue, constructNode(t));
    }

    pthread_mutex_lock(&mainTrackLock);
    if(t->direction == 'e' || t->direction == 'E'){
        while(pthread_cond_wait(&condVar, &eastQueueLock) != 0);

        deQueue(eastQueue);

        printf("%.2f Train %2d is ON the main track going East\n", get_simulation_time(), t->id);

        usleep(t->crossTime * 100000);

        printf("%.2f Train %2d is OFF the main track after going East\n", get_simulation_time(), t->id);
    }else{
        while(pthread_cond_wait(&condVar, &westQueueLock) != 0);

        deQueue(westQueue);

        printf("%.2f Train %2d is ON the main track going West\n", get_simulation_time(), t->id);

        usleep(t->crossTime * 100000);

        printf("%.2f Train %2d is OFF the main track after going West\n", get_simulation_time(), t->id);
    }
    pthread_mutex_unlock(&mainTrackLock);

    pthread_exit(NULL);
    return NULL;
}

void* mainTrack() { // use for control trains in order get into the main track
    while (1) {
        pthread_mutex_lock(&mainTrackLock2);
        if(eastQueue->size != 0 && eastQueue->head->train->priority == 0){
            pthread_cond_signal(&condVar);
        }else if(westQueue->size != 0 && westQueue->head->train->priority == 0){
            pthread_cond_signal(&condVar);
        }else if(eastQueue->size != 0 && eastQueue->head->train->priority == 1){
            pthread_cond_signal(&condVar);
        }else if(westQueue->size != 0 && westQueue->head->train->priority == 0){
            pthread_cond_signal(&condVar);
        }

        pthread_mutex_unlock(&mainTrackLock2);
    }
}

int main(int argc, char* argv[]) { // main function use for run the program, to create thread or destrory them.
    int totalTrains;
    char fileInformation[100][100];
    totalTrains = readFileIn(argv[1], fileInformation);
    tokenInformation(fileInformation, totalTrains);
    eastQueue = constructQueue();
    westQueue = constructQueue();

    gettimeofday(&init_time, NULL);

    if(pthread_create(&trackThread, NULL, mainTrack, NULL)){
        perror("Error: failed on thread creation.\n");
        exit(1);
    }

    int i;
    for(i=0; i<totalTrains; i++){
        if(pthread_create(&trainThreads[i], NULL, trainEntry, &trainArray[i])){
            perror("Error: failed on thread creation.\n");
            exit(1);
        }
    }

    int j;
    for(j=0; j<totalTrains; j++){
        if(pthread_join(trainThreads[j], NULL)){
            perror("Error: failed on join thread.\n");
            exit(1);
        }
    }

    pthread_mutex_destroy(&mainTrackLock);
    pthread_mutex_destroy(&eastQueueLock);
    pthread_mutex_destroy(&westQueueLock);
    pthread_cond_destroy(&condVar);
    return 0;
}
