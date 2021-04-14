#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>


#include "../dependencies/map.h"
#include "../dependencies/joueur.h"
#include "../dependencies/carte.h"
#include "../dependencies/message.h"
#include "../dependencies/comm.h"


#define true 1
#define false 0
#define NB_PIOCHE 120
#define SHM_SIZE 256
#define SRV_KEY 19

#define CHECK(sts,msg) if ((sts) == -1 )  { perror(msg);exit(-1);}

t_connexion connexions[5];
int nbconnexions = 0;
int ack_remain = 0;
int nbJoueur;
bool shutdown = false;
// Communication via messages et tubes
int reader_fifo, id_file;
pthread_t thread_listener;
key_t tx_key, shm_key;
int nr=0;
// Communication via mémoire partagée
int shared_memory;
struct joueur joueurs[0];

void * findByPid(int pid);

/*
    Initialise la mémoire partagée
*/
void initShm(void) {
    CHECK(shm_key = ftok("/tmp",SRV_KEY),"ftok()");
    CHECK(shared_memory = shmget(shm_key,  sizeof(t_comm), 0666 | IPC_CREAT),"shmget()");
    t_comm *empty;
    empty = malloc(sizeof(t_comm));
    t_comm *snd = shmat(shared_memory, NULL, 0);
    memcpy(snd, empty, sizeof(t_comm));
    CHECK(shmdt(snd), "shmdt()");
}

/*
    Initialise les messages
*/
void initMessage(void) {
   tx_key = ftok("/tmp", SRV_KEY);
   id_file = msgget(tx_key, 0666 | IPC_CREAT);
    CHECK(id_file, "Impossible de créer le flux de message");
}


/*
    Envoie au client (message)
*/
void sendMessageToClient_msg(char message[256]) {
    int res;
    message_t msg;
    msg.type = 1;
    strncpy(msg.msg, message, 256);
    if(nr == 0) {nr++; return;}
    res = msgsnd(id_file, (void *) &msg, sizeof(char)*256, 0);
    CHECK(res, "Envoi du message impossible\n");
}

/*
    Envoie au client (shared memory)
*/
void sendMessageToClient(char message[256]) {
    char* write;
    write = shmat(shared_memory,NULL, 0);
    strncpy(write, message,256);
    CHECK(shmdt(write), "shmdt()");
}


/*
    Envoie au client (shared memory) | Comm
*/
void sendMessageToClient_comm(t_comm comm) { 
    if (comm.dest == -1) {
        ack_remain = nbconnexions;
    } else {
        ack_remain = 1;
    }

    t_comm *snd = shmat(shared_memory,NULL, 0);
    memcpy(snd, &comm,sizeof(t_comm));
    CHECK(shmdt(snd), "shmdt()");
}


struct carte pioche[NB_PIOCHE];


struct carte creerCarte(char * nom, int type, int move, char * desc, int ident) {
    struct carte tmp;
    strncpy(tmp.nom, nom, 256);
    //tmp.nom = nom;
    tmp.type = type;
    tmp.movement = move;
    tmp.ident = ident;
    //tmp.description = desc;
    strncpy(tmp.description, desc, 256);
    return tmp;
}

void shufflePioche(int nbShuffle) {
    for(int j = 0; j < nbShuffle; ++j) {
        for(int i = 0; i < (NB_PIOCHE / 2); ++i) {
            int ran = rand() % NB_PIOCHE + 1;
            
            struct carte tmp = pioche[i];
            pioche[i] = pioche[ran];
            pioche[ran] = tmp;
        }
    }
}

void genererCartes(void) {
    for(int i = 0; i < NB_PIOCHE; ++i) {
        if(i < 30) {
            pioche[i] = creerCarte("25 miles", MOVEMENT, 25, "Carte pour avancer de 1 case",0);
        } else if(i < 50) {
            pioche[i] = creerCarte("50 miles", MOVEMENT, 50, "Carte pour avancer de 2 cases",0);
        } else if(i < 60) {
            pioche[i] = creerCarte("75 miles", MOVEMENT, 75, "Carte pour avancer de 3 cases",0);
        } else if(i < 65) {
            pioche[i] = creerCarte("100 miles", MOVEMENT, 100, "Carte pour avancer de 4 cases",0);
        } else if(i < 66) {
            pioche[i] = creerCarte("As du volant", UNIQUE, 0, "Carte empêchant les accidents",1);
        } else if(i < 67) {
            pioche[i] = creerCarte("Increvable", UNIQUE, 0, "Carte empêchant les crevaisons",2);
        } else if(i < 68) {
            pioche[i] = creerCarte("Citerne", UNIQUE, 0, "Carte empêchant les pannes d'essence",3);
        } else if(i < 69) {
            pioche[i] = creerCarte("Véhicule prioritaire", UNIQUE, 0, "Carte empêchant les ralentissements et feux rouges",4);
        } else if(i < 77) {
            pioche[i] = creerCarte("Feu vert", AVANTAGE, 0, "Utile au 1er tour et annulant les feux rouges",5);
        } else if(i < 83) {
            pioche[i] = creerCarte("Réparation", AVANTAGE, 0, "Annule les accidents",6);
        } else if(i < 85) {
            pioche[i] = creerCarte("Roue de secours", AVANTAGE, 0, "Annule les crevaisons",7);
        } else if(i < 87) {
            pioche[i] = creerCarte("Pompe à essence", AVANTAGE, 0, "Annule les pannes d'essence",8);
        } else if(i < 95) {
            pioche[i] = creerCarte("Feu rouge", PIEGE, 0, "Impose un arrêt au joueur ciblé",9);
        } else if(i < 99) {
            pioche[i] = creerCarte("Crevaison", PIEGE, 0, "Impose un arrêt au joueur ciblé",10);
        } else if(i < 103) {
            pioche[i] = creerCarte("Panne d'essence", PIEGE, 0, "Impose un arrêt au joueur ciblé",11);
        } else if(i < 109) {
            pioche[i] = creerCarte("Limitation de vitesse", PIEGE, 0, "Impose un ralentissement au joueur ciblé",12);
        } else if(i < 117) {
            pioche[i] = creerCarte("Accident", PIEGE, 0, "Impose un arrêt au joueur ciblé",13);
        } else {
            pioche[i] = creerCarte("Fin de limitation", AVANTAGE, 0, "Annule les ralentissements",14);
        }
    }
}

void createListener(void) {
    mkfifo("game.fifo",0666);
    reader_fifo = open("game.fifo", O_RDONLY);
}

char questionNbJoueur(void) {
    char nbJoueur; 
    printf("Combien de joueurs vont jouer ? [2 - 4] ");
    scanf("%c", &nbJoueur);

    // On vérifie si il s'agit d'un nombre valide
    if(nbJoueur < '2' || nbJoueur > '4') {
        printf("\nLa valeur saisit n'est pas correcte !\n");
    } else {
        printf("La partie va être configurée avec %c joueurs\n", nbJoueur);
    }

    return nbJoueur;
}

int isCaseValide(int x, int y) {
    if(map[y][x] == ' ' && map[y][x - 1] != ' ' && map[y][x + 1] != ' ') {
        return true;
    }

    if(map[y][x] == ' ' && map[y - 1][x] != ' ' && map[y + 1][x] != ' ') {
        return true;
    }

    return false;
}

int verifierNbCases(void) {
    int compteur = 0;
    for(int hauteur = 0; hauteur < MAP_HEIGHT; ++hauteur) {
        for(int largeur = 0; largeur < MAP_WIDTH; ++largeur) {
            if(isCaseValide(largeur, hauteur) == true) {
                ++compteur;
            }
        }
    }

    return compteur;
}

int lancerDe(int max) {
    return rand() % max + 1;
}

/*
    Suppression de la mémoire partagée
*/
void del_shm(void) {
    CHECK( shmctl(shared_memory, IPC_RMID, NULL),"shmctl()");
}

void force_close() {
    printf("Arrêt forcé du programme : plus aucun client connecté. Tube cassé.\n");
    exit(EXIT_FAILURE);
}

void addPlayerToConnexions(int pid, char pseudo[256]) {
    t_connexion c;
    c.pid = pid;
    struct joueur tmp;
    tmp.connected = true;
    tmp.connexion = c;
    strncpy(tmp.pseudo, pseudo,256);
    connexions[nbconnexions] = c;
    joueurs[nbconnexions] = tmp;
    nbconnexions++;
}

void handleACK() {
    ack_remain--;
    if (ack_remain == 0) {
        printf("Tous les clients ont accusé la réception du paquet\n");
        t_comm *empty;
        empty = malloc(sizeof(t_comm));
        t_comm *snd = shmat(shared_memory,NULL, 0);
        memcpy(snd, empty,sizeof(t_comm));
        CHECK(shmdt(snd), "shmdt()");
    }
    if (shutdown && ack_remain == 0) {
        del_shm();
        exit(EXIT_SUCCESS);
    }
}

void handleLogin(t_comm msg) {
    if (msg.src == 0) {force_close();}

    if (nbconnexions < nbJoueur) {
        printf("Connexion d'un client > pid : %d | nom d'utilisateur : %s\n", msg.src, msg.msg);
        addPlayerToConnexions(msg.src, msg.msg);
    } else {
        t_comm rep;
        rep.type = ERROR;
        rep.dest = msg.src;
        strncpy(rep.msg, "Impossible de vous connecter : le nombre de clients autorisés est dépassé !\n", 256);
        sendMessageToClient_comm(rep);
    }
}

void *listener() {
    char msg[256];
    createListener();   
    do {
        t_comm *comm;
        comm = malloc(sizeof(t_comm));
        read(reader_fifo,comm, sizeof(t_comm));
        switch (comm->type)
        {
        case LOGIN:
            handleLogin(*comm);            
            break;
        
        case DEFAULT:
            printf("RX (%d) : %s\n", comm->src, comm->msg);
            break;

        case ACK:
            handleACK();
        break;

        default:
            printf("Message non reconnu\n");
            break;
        }
        //printf("RX : %s\n",msg);
    } while (strcmp(msg, "STOP\n") != 0);
    close(reader_fifo);
}

void emptybuff() {
    int c = 0;
    while (c!='\n' && c!=EOF) {
        c = getchar();
    }
}

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
    Déroulement de la partie
*/
void gameHandle(void) {
    // Attente des joueurs
    int last = nbconnexions;
    while (nbconnexions < nbJoueur) {
        if (last != nbconnexions) {
            last = nbconnexions;
            printf("Un joueur s'est connecté, (%d / %d) (%d joueurs restants)\n", nbconnexions, nbJoueur, nbJoueur-nbconnexions);
        }
        sleep(1);
    }
    printf("Un joueur s'est connecté, (%d / %d) (%d joueurs restants)\n", nbconnexions, nbJoueur, nbJoueur-nbconnexions);
    // Quand on est bon ...
    printf("Distribution des cartes...\n");

    // On va envoyer les cartes aux clients
    int debut = 120 - (nbJoueur*6);
    
    for (int i = 0; i<nbJoueur; i++) {
        printf("Envoi de cartes au joueur %s (%d)\n", joueurs[i].pseudo, joueurs[i].connexion.pid);
        for (int j = 0; j<6; j++) {
            t_comm snd;
            snd.carte = pioche[j+debut];
            snd.dest = joueurs[i].connexion.pid;
            snd.type = CARD;
            strncpy(snd.msg, "Carte !",256);
            snd.src = 0;
            sendMessageToClient_comm(snd);
            printf("Envoi de la carte %s\n", pioche[j+debut].nom);
            while(ack_remain != 0) {sleep(1);}
        }
        debut+=6;
        
    }


}

void * findByPid(int p) {
    for(int i = 0; i < nbJoueur; ++i) {
        if(joueurs[i].pid == p) {
            return &joueurs[i];
        }
    }
    return NULL;
} 

/*
    Fermeture de la connexion pour tout le monde
*/
void disconnectAll() {
    printf("Déconnexion des joueurs\n");
    t_comm disconnect;
    disconnect.type = CLOSE_CONNECTION;
    disconnect.dest = -1;
    strncpy(disconnect.msg, "Déconnexion des clients",256);
    sendMessageToClient_comm(disconnect);
} 


int main(int argc, char *argv[]) {
    printf("Bienvenue sur l'interface du serveur\n");

    // On récupère le nombre de joueurs
    nbJoueur = (questionNbJoueur() - '0'); // conversion des char en int
    while(nbJoueur < 2 || nbJoueur > 4) {
        nbJoueur = questionNbJoueur();
    }
    
    // On récupère la taille du terminal
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    printf("Dimension du terminal = %dx%d\n", ws.ws_col, ws.ws_row); // Contrôle durant le dev, TODO retirer cette ligne

    // On affiche le nombre de cases sur la carte
    printf("La carte contient %d cases\n", verifierNbCases());

    
    for(int i = 0; i < nbJoueur; ++i) {
        struct joueur tmp;
        joueurs[i] = tmp;
    }

    bool ordre[nbJoueur];
    for (int i = 0; i<nbJoueur; i++) {
        ordre[i] = false;
    }

    for (int i = 0; i<nbJoueur; i++) {
        bool used = true;
        int rdm;
        while(used) {
            rdm = lancerDe(nbJoueur);
            if (ordre[rdm-1] == false) {
                used = false;
                ordre[rdm-1] = true;
                joueurs[i].ordre = rdm;
                joueurs[i].connected = false;
            }
        }
    }

    for (int i = 0; i<nbJoueur; i++) {
        printf("Joueur %d : Place %d\n", i, joueurs[i].ordre);
    }

    genererCartes();
    printf("Il y a %ld cartes dans la pioche\n", sizeof(pioche) / sizeof(pioche[0]));

    shufflePioche(10);
    printf("Affichage des 10ères cartes de la pioche (tmp pour le dév) :\n");
    for(int i = 0; i < 10; ++i) {
        printf("\t%s | %s\n", pioche[i].nom, pioche[i].description);
    }


    pthread_create(&thread_listener, NULL, listener, NULL);

    printf("En attente de la connexion des joueurs ...\n");
    // Procédure ici :
    // Quand le client envoie un message comme quoi il est arrivé, le serveur lui envoie 
    // comme quoi il attend encore x joueur(s)

    // La gueule des messages se trouve dans le md


    initShm();
    /*
    printf("Que voulez-vous envoyez au client ? ");
    char buffer[256];
    char empty[256];
    t_comm comm;
    while (strcmp(buffer, "STOP")) {
        printf("> ");
        readline(buffer, 256);
        if (strcmp(buffer, empty) != 0) {
            strncpy(comm.msg, buffer, 256);
            comm.type = DEFAULT;
            comm.dest = -1;
            //sendMessageToClient_msg(buffer);
            sendMessageToClient_comm(comm);
        }
    }
    */
    gameHandle();
    pause();

    disconnectAll();
    
    return 0;
}