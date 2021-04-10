#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
<<<<<<< HEAD
#include <sys/ipc.h>
#include <sys/msg.h>
#include "string.h"
=======
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
>>>>>>> 139d0d74adf2f34cb6e24efd6892fac6c8e1025f


#include "../dependencies/map.h"
#include "../dependencies/joueur.h"
#include "../dependencies/carte.h"

#define CHECK(sts, msg) if ((sts)==-1) {perror(msg); exit(-1);}

#define true 1
#define false 0


int writer_fifo, id_file;
key_t rx_key=28487;
struct msqid_ds buf;
int client_id = rand();


void init_writer(void) {
    mkfifo("game.fifo", 0666);
    writer_fifo = open("game.fifo", O_WRONLY);
}

<<<<<<< HEAD
void init_reader(void) {
    id_file = msgget(rx_key, IPC_CREAT | IPC_EXCL);
    CHECK(id_file, "Échec lors de la création de la lecture.\n");
    CHECK(msgctl(id_file, IPC_STAT, &buf), "Échec lors de la récupération des informations");
}

=======
// Communication via mémoire partagée
int shmNo = -1;
int creationMemoire(void) {
    key_t key = ftok("/tmp", 12345);
    shmNo = shmget(key, 256, 0666);
    if(shmNo < 0) {
        perror("Erreur lors de la création de la mémoire partagée");
        exit(-1);
    }
}
>>>>>>> 139d0d74adf2f34cb6e24efd6892fac6c8e1025f

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

int main(void) {
    printf("Bienvenue dans le client du 400 miles !\n");

    // On récupère la taille du terminal
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    printf("Dimension du terminal = %dx%d\n", ws.ws_col, ws.ws_row); // Contrôle durant le dev, TODO retirer cette ligne

    init_writer();
    char msg[256];
    do {
        fgets(msg, 256, stdin);
        write(writer_fifo, msg, strlen(msg)+1);
    } while (strcmp(msg, "STOP\n") != 0);
    close(writer_fifo);

    bool isValid = false;
    while(isValid == false) {
        char * msg = readMessageFromServer();
        char * header = "";
        strncpy(header, msg, 4);
        if(strcmp(header, "400M") == 0) {
            isValid = true;
        }
    }

    return 0;
}