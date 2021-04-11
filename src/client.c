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
#include "../dependencies/map.h"
#include "../dependencies/joueur.h"
#include "../dependencies/carte.h"
#include "../dependencies/message.h"

#define CHECK(sts, msg) if ((sts)==-1) {perror(msg); exit(-1);}
#define SRV_KEY 12345

int writer_fifo, id_file;
key_t rx_key;
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
/*
char *  readMessageFromServer(void) {
    char * shm = shmat(shmNo, NULL, SHM_RDONLY);
    if(shm < 0) {
        perror("Erreur lors de l\'allocation de la mémoire partagée");
    }
    printf("Message reçu :\n\t%s", shm);
    int err = shmdt(shm);
    if(err < 0) {
        perror("Erreur lors du détachement de la mémoire partagée");
        exit(-1);
    }
}
*/
/*
void *reader() {
    bool isValid = false;
    while(isValid == false) {
        char * msg = readMessageFromServer();
        printf("MSG = %s\n", msg);
        char * header = "";
        strncpy(header, msg, 4);
        printf("HEADER = %s\n", header);
        if(strcmp(header, "400M") == 0) {
            isValid = true;
        }
    }

    shmctl(shmNo, IPC_RMID, NULL);
}
*/
void displayInformation(int id){
	struct msqid_ds information;

	CHECK(msgctl(id, IPC_STAT, &information),"msgctl");
	
	printf("Clé fournie à msgget : %d\n",id);
	printf("Nombre actuel d'octets dans la file (non standard) : %ld\n",information.__msg_cbytes);
	printf("Nombre actuel de messages dans la file : %ld\n",information.msg_qnum);
	printf("Nombre maximum d'octets autorisés dans la file : %ld\n",information.msg_qnum);
	printf("PID du dernier msgsnd : %d\n",information.msg_lspid);
	printf("PID du dernier msgrcv : %d\n",information.msg_lrpid);
	printf("UID effectif du propriétaire : %d\n",information.msg_perm.uid);
	printf("GID effectif du propriétaire : %d\n",information.msg_perm.gid);
	printf("UID effectif du créateur : %d\n",information.msg_perm.cuid);
	printf("ID effectif du créateur : %d\n",information.msg_perm.cgid);
	printf("Permissions : %d\n",information.msg_perm.mode);
}


void *reader() {
    init_reader();
    displayInformation(id_file);
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

int main(void) {
    printf("Bienvenue dans le client du 400 miles !\n");

    // On récupère la taille du terminal
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    printf("Dimension du terminal = %dx%d\n", ws.ws_col, ws.ws_row); // Contrôle durant le dev, TODO retirer cette ligne
    
    pthread_create(&thread_reader, NULL, reader, NULL);

    init_writer();
    char msg[256];
    do {
        fgets(msg, 256, stdin);
        write(writer_fifo, msg, strlen(msg)+1);
    } while (strcmp(msg, "STOP") != 0);
    close(writer_fifo);

    pthread_join(thread_reader, NULL);

    return 0;
}