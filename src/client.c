#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "../dependencies/map.h"
#include "../dependencies/joueur.h"
#include "../dependencies/carte.h"
#include "../dependencies/message.h"

#include "../dependencies/comm.h"

#define CHECK(sts, msg) if ((sts)==-1) {perror(msg); exit(-1);}
#define SRV_KEY 18




int writer_fifo, id_file, shared_memory;
key_t rx_key, shm_key;
struct msqid_ds buf;
int client_id;
pthread_t thread_reader;

void init_writer(void) {
    mkfifo("game.fifo", 0666);
    writer_fifo = open("game.fifo", O_WRONLY);
}

void init_reader(void) {
   rx_key = ftok("/tmp",SRV_KEY);
   CHECK(id_file = msgget(rx_key, 0666),"PB msgget");

}

void init_shm(void) {
    CHECK(shm_key = ftok("/tmp",SRV_KEY),"ftok()");
    CHECK(shared_memory = shmget(shm_key, sizeof(t_comm), 0666 | IPC_CREAT),"shmget()");    
}



// Communication via mémoire partagée
int shmNo = -1;
int creationMemoire(void) {
    key_t key = ftok("/tmp", 123456);
    shmNo = shmget(key, 256, 0666);
    if(shmNo < 0) {
        perror("Erreur lors de la création de la mémoire partagée");
        exit(-1);
    }
}

void sendMessage(comm_type type, char * message) {
    t_comm* msg;
    msg = malloc(sizeof(t_comm));
    msg->type = type;
    msg->src = getpid();
    strncpy(msg->msg, message,256);
    write(writer_fifo, msg, sizeof(t_comm));
}

void *reader() {
    init_reader();
    //displayInformation(id_file);
    char txt[256];
    message_t msg;
    printf("Prêt à recevoir des messages !\n");
    strcpy(msg.msg, "message");
    while(strcmp(msg.msg, "STOP") != 0) {
        CHECK(msgrcv(id_file,&msg, sizeof(char)*256, 0,MSG_NOERROR),"msgrcv");
        printf("RX : %s\n", msg.msg);
    }
    printf("Fin de la lecture\n");

}


void handleData(t_comm data) {
    if (data.dest == -1 || data.dest == getpid()) {
        switch (data.type) {
        case ERROR:
            printf("Une erreur est survenue : \n%s", data.msg);
            exit(EXIT_FAILURE);
            break;
        
        case CLOSE_CONNECTION:
            printf("Fermeture de la connexion au serveur !\n%s\n", data.msg);
            sleep(2);
            exit(EXIT_SUCCESS);
            break;
        default:
            printf("<SERVEUR>: %s\n", data.msg);
            break;
        }
    }   
}


void *reader_shm() {
    init_shm();
    t_comm *comm;
    comm = malloc(sizeof(t_comm));
    strncpy(comm->msg, "vide", 256);
    
    t_comm tmp;
    
    t_comm *empty;
    empty = malloc(sizeof(t_comm));
    //char empty[256];
    while(strcmp(tmp.msg, "STOP") != 0) {
        comm = shmat(shared_memory,NULL, 0);
        if (memcmp(comm, empty, sizeof(t_comm)) != 0) {
            memcpy(&tmp, comm, sizeof(t_comm));
            handleData(*comm);
            sendMessage(ACK,"");
        }
        CHECK(shmdt(comm), "shmdt()");
        sleep(1);
    }
}






int main(void) {
    printf("Bienvenue dans le client du 400 miles !\n");

    // On récupère la taille du terminal
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    printf("Dimension du terminal = %dx%d\n", ws.ws_col, ws.ws_row); // Contrôle durant le dev, TODO retirer cette ligne
    
    pthread_create(&thread_reader, NULL, reader_shm, NULL);

    init_writer();
    //login
    sendMessage(LOGIN, "");
    printf("<>\n");

    char msg[256];
    bool isFirstMessage = true;
    do {
        if(isFirstMessage) {
            printf("Comment vous appelez vous ? ");
            isFirstMessage = false;
            fgets(msg, 256, stdin);
            sendMessage(PSEUDO, msg);
        } else {
            fgets(msg, 256, stdin);
            sendMessage(DEFAULT, msg);
        }
    } while (strcmp(msg, "STOP\n") != 0);
    close(writer_fifo);
    pthread_kill(thread_reader, 9);

    //pthread_join(thread_reader, NULL);

    return 0;
}