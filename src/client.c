#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "string.h"


#include "../dependencies/map.h"
#include "../dependencies/joueur.h"
#include "../dependencies/carte.h"

#define true 1
#define false 0

int writer_fifo;

void init_writer(void) {
    mkfifo("game.fifo", 0666);
    writer_fifo = open("game.fifo", O_WRONLY);
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

    return 0;
}