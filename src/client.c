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
#define SRV_KEY 19


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int writer_fifo, id_file, shared_memory;
key_t rx_key, shm_key;
struct msqid_ds buf;
int client_id;
pthread_t thread_reader;
carte deck[6];
carte place[120];
carte temp;
int nbCartes = 0, nbCartesPlacees = 0;
bool turn = false, endgame = false, ok = false, fault = false;
int traveled = 0;
char * message_reponse;
int nbj = 0;


// Valeurs du jeu
bool stopped = true, accident = false, slowed = false, fuel = false, tire = false;
bool good_driver = false, fuel_master = false, prior = false, good_tire = false;


/*
    Vidage du buffer pour ReadLine
*/
void emptybuff() {
    int c = 0;
    while (c!='\n' && c!=EOF) {
        c = getchar();
    }
}

/*
    Lis l'entrée clavier dans un char* chaine avec une taille définie (length)
*/
int readline(char *chaine, int length) {
    char *start = NULL;
    if (fgets(chaine, length, stdin) != NULL) {
        start = strchr(chaine, '\n');
        if (start != NULL) {
            *start = '\0';
        }else {
            emptybuff();
        }
        return 1;
    } else {
        emptybuff();
        return 0;
    }
}



/*
    Initialisation du fifo pour l'envoi de données client => serveur
*/
void init_writer(void) {
    mkfifo("game.fifo", 0666);
    writer_fifo = open("game.fifo", O_WRONLY);
}

/*
    Initialisation du lecteur de boîte aux lettres
    (unused)
*/
void init_reader(void) {
   rx_key = ftok("/tmp",SRV_KEY);
   CHECK(id_file = msgget(rx_key, 0666),"PB msgget");

}

/*
    Initialisation de la mémoire partagée pour l'envoi serveur => client
*/
void init_shm(void) {
    CHECK(shm_key = ftok("/tmp",SRV_KEY),"ftok()");
    CHECK(shared_memory = shmget(shm_key, sizeof(t_comm), 0666 | IPC_CREAT),"shmget()");    
}

/*
    Envoi d'un message textuel dans le tube
    type: type défini dans comm_type (attention, utiliser que default ou login ici)
    message: le message en char*
*/
void sendMessage(comm_type type, char * message) {
    t_comm* msg;
    msg = malloc(sizeof(t_comm));
    msg->type = type;
    msg->src = getpid();
    strncpy(msg->msg, message,256);
    write(writer_fifo, msg, sizeof(t_comm));
}

/*
    Envoi d'une communication dans le tube
    comm: communication (t_comm)
*/
void sendComm(t_comm comm) {
    comm.src = getpid();
    t_comm* snd;
    snd = malloc(sizeof(t_comm));
    memcpy(snd, &comm, sizeof(t_comm));
    write(writer_fifo, snd, sizeof(t_comm));
}

/*
    Méthode du thread pour la lecture de données via les boîtes aux lettres
    (unused)
*/
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


/*
    Ajoute la carte dans le deck du joueur (ou sur son jeu si ça vient d'un joueur)
*/
void addCardsToDeck(carte carte, int src) {
    if (src == 0 && nbCartes < 6) {
        // Remplissage des cartes
        deck[nbCartes] = carte;
        //printf("Vous avez reçu la carte %s (%s).\n", deck[nbCartes].nom, deck[nbCartes].description);
        nbCartes++;
    } else {
        printf("Vous avez reçu du joueur %d la carte %s (%s)\n", src, carte.nom, carte.description);
        switch (carte.ident){
        case 9:
            stopped = true;
            break;
        
        case 10:
            tire = true;
            break;

        case 11:
            fuel = true;
            break;
        
        case 12:
            slowed = true;
            break;
        
        case 13:
            accident = true;
            break;
        
        default:
            break;
        }
    }
}

/*
    Gestion des communications reçues, renvoi vers les fonctions
    dédiées et rejet des communication non addressées au client.
    data: données reçues en t_comm
*/
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
        
        case CARD:
            printf("Vous avez reçu : %s\n", data.carte.nom);
            addCardsToDeck(data.carte, data.src);
            break;
        
        case SCORE:
            printf("> Fin du tour <\nScore des joueurs :\n%s\n", data.msg);
            break;
        
        case ENDGAME:
            endgame = true;
            break;
        
        case PSEUDO:
            nbj = data.src;
            ok = true;
            break;
        
        case TURN:
            turn = true;
            temp = data.carte;
            break;

        case OK:
            ok = true;
            fault = false;
            break;
        
        case FAULT:
            ok = false;
            fault = true;
            break;

        default:
            printf("<SERVEUR>: %s\n", data.msg);
            break;
        }
        sendMessage(ACK,"");
    }   
}

/*
    Méthode du thread pour la lecture de la mémoire partagée
*/
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
            
        }
        CHECK(shmdt(comm), "shmdt()");
        usleep(250000);
    }
}

/*
*   Envoie la carte au serveur
*   carte: carte à envoyer
*   dest: destination de la carte (peut être l'identifiant du joueur / 0 pour le jeu du joueur / -1 pour la pioche)
*/
void sendCardToServer(carte carte, int dest) {
    t_comm send_back;
    send_back.dest = dest;
    send_back.carte = carte;
    send_back.type = CARD;
    sendComm(send_back);
}

/*
    Joue une carte donnée en paramètre
    carte: carte en type carte...
    renvoie : booléen pour dire si la carte a été acceptée ou refusée
*/
bool playcard(carte carte) {
    // on joue la carte
    int target = 0;
    int type = 1;
    if (carte.type == MOVEMENT) {
        if (fuel || stopped || accident || tire || (slowed && carte.movement > 50)) {
            printf("Vous ne pouvez pas joueur cette carte, vous êtes stoppé ou ralenti !\n");
            return false;
        }
        traveled+=carte.movement;
    } else if (carte.type == AVANTAGE) {
        if (carte.ident == 5 && stopped) {
            stopped = false;              
        } else if (carte.ident == 6 && accident) {
            accident = false;
        } else if (carte.ident == 7 && tire) {
            tire = false;
        } else if (carte.ident == 14 && slowed) {
            slowed = false;
        } else {
            printf("Vous ne pouvez pas poser cette carte, vous n'êtes pas bloqué...\n");
            return false;
        }
    } else if (carte.type == UNIQUE) {
        if (carte.ident == 1) {
            accident = false;
            good_driver = true;
        } else if (carte.ident == 2) {
            tire = false;
            good_tire = true;
        } else if (carte.ident == 3) {
            fuel = false;
            fuel_master = true;
        } else {
            stopped = false;
            slowed = false;
            prior = true;
        }
    } else {
        // carte mauvaise
        // on demande au serveur les joueurs...
        
        type = 0;
        t_comm msg;
        msg.type = PSEUDO;
        sendComm(msg);
        ok = false;
        while (!ok) {usleep(250000);}
        ok = false;
        printf("Joueurs :\n");
        for (int i = 1; i<nbj; i++) {
            printf("- Joueur %d\n", i);
        }

        printf("Entrez le numéro du joueur à qui envoyer la carte ?\n");
        bool success = false;
        while (!success) {
            success = false;
            printf("> ");
            char * tmp;    
            int nb = -1;
            while (nb <= -1 || nb > nbj-1) {
                tmp = malloc(sizeof(char)*256);
                readline(tmp, 256);
                nb = atoi(tmp);
            }
            if (nb != 0) {
                sendCardToServer(carte, nb);
                while (!ok && !fault) {usleep(250000);}
                bool localok = ok;
                if (ok) {ok = false;success = true;} else {printf("Le client séléctionné ne peux pas recevoir la carte, car il est déjà bloqué ou a une carte bloquante.\n"); fault = false;}
            } else {sendCardToServer(carte, -1);success = true;}
            
        }
        
        printf("La carte à bien été envoyée au joueur !\n");
       
    }
    if (type != 0) {
        sendCardToServer(carte,target);
        place[nbCartesPlacees] = carte;
        nbCartesPlacees++;
    }   
    return true;
}   

/*
    Jette une carte, donnée par entrée clavier dans la défausse
*/
void throw_card() {
    printf("Quel numéro de carte souhaitez-vous jeter ?\n");
    int num = 0;
    while (num <= 0 || num > 7) {
        printf(">");
        scanf(" %d", &num);
    }

    t_comm send_back;
    send_back.dest = -1;
    send_back.type = CARD;
    if (num == 7) {
        // On envoi temp au serveur
        send_back.carte = temp;
    } else {
        // On envoie la carte au serveur, en plaçant temp dans le tableau
        send_back.carte = deck[num-1];
        deck[num-1] = temp;
    }
    sendComm(send_back);
    printf("La carte a été retournée au serveur !\n");
}

/*
    Affiche les status du joueur
*/
void printStatus(void) {
    if (slowed) {
        printf("- Ralenti\n");
    }
    if (accident) {
        printf("- Accident\n");
    }
    if (stopped) {
        printf("- Stoppé\n");
    }
    if (tire) {
        printf("- Pneu Crevé\n");
    }
    if (fuel) {
        printf("- A court d'essence\n");
    }
}

/*
    Algorithmie du jeu...
    -> Attente des cartes de la part du serveur
    -> Attente du tour du joueur
    -> Gestion du tour (choix carte, si envoi vers défausse, si envoi vers le jeu ou un joueur...)
    -> Gestion d'une victoire
*/
void handleGame(void) {
    printf("En attente des cartes...\n");
    int actual = 0;
    while(nbCartes < 6) {
        if (actual != nbCartes) {
            actual = nbCartes;
            printf("Cartes : %d / %d\n", nbCartes, 6);
        }
        usleep(250000);
    }

    
    while (!endgame) {
        printf("En Attente de votre tour ...\n");
        while(!turn) {
            if (endgame) {break;}
            sleep(1);
        }
        if (endgame) {break;}
        printf("C'est à votre tour !\n");
        printf("Votre statut : \n");

        printStatus();

        printf("Votre distance parcourue :\n%d Miles\n", traveled);

        printf("Vos cartes :\n");
        for (int i = 0; i<nbCartes; i++) {
            printf("%d - %s (%s)\n", i+1, deck[i].nom, deck[i].description);
        }
        printf("%d - %s (%s)\n", 7, temp.nom, temp.description);
        printf("8 - Jeter une carte\n");

        int rep = 0; 
        bool status = false;
        while (!status) {
            rep = 0;
            while (rep <= 0 || rep > 8) {
            printf("> ");
            char * tmp;
            tmp = malloc(sizeof(char)*256);
            readline(tmp, 256);
            rep = atoi(tmp);
        }
            switch (rep)
            {
            case 8:
                throw_card();
                status = true;
                break;
            
            case 7:
                // on place la carte récupérée 
                status = playcard(temp);
                break;

            default:
                // on joue une carte normale
                status = playcard(deck[rep-1]);
                if (status)
                    deck[rep-1] = temp;
                break;
            }
        }
        



        turn = false;
        usleep(250000);
    }

    if (traveled >= 400) {
        printf("Félicitations ! Vous avez gagné la partie ! ");
    } else {
        printf("Un joueur à gagné la partie. ");
    }
    printf("Vous pouvez consulter votre score et celui des autres joueurs sur le serveur de Jeu\n");
    //printf("Un joueur a gagné la partie ...\n");
    
    
}




int main(void) {
    printf("Bienvenue dans le client du 400 miles !\n");
    message_reponse = malloc(sizeof(char)*1000);
    // On récupère la taille du terminal
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    //printf("Dimension du terminal = %dx%d\n", ws.ws_col, ws.ws_row); // Contrôle durant le dev, TODO retirer cette ligne
    
    pthread_create(&thread_reader, NULL, reader_shm, NULL);

    init_writer();
    // Avant d'envoyer notre connexion au serveur, on demande gentillement le pseudo
    char pseudo[256];
    printf("Pour vous connecter, vous devez entrer un pseudo !\nComment vous appelez vous ? ");
    readline(pseudo, 256);
    

    //login
    sendMessage(LOGIN, pseudo);
    printf("<>\n");

    char msg[256];
    bool isFirstMessage = true;
 
    // TODO : Attention, changer ici, ça ne va pas dans le handlegame !
    handleGame();
    close(writer_fifo);
    pthread_kill(thread_reader, 9);

    //pthread_join(thread_reader, NULL);

    return 0;
}