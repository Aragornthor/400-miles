#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>

#include "../dependencies/map.h"
#include "../dependencies/joueur.h"
#include "../dependencies/carte.h"

#define true 1
#define false 0
#define NB_PIOCHE 120

struct carte pioche[NB_PIOCHE];
enum carteType {
    MOVEMENT = 0, UNIQUE = 1, AVANTAGE = 2, PIEGE = 3
};

struct carte creerCarte(char * nom, int type, int move, char * desc) {
    struct carte tmp;
    tmp.nom = nom;
    tmp.type = type;
    tmp.movement = move;
    tmp.description = desc;
    
    return tmp;
}

void genererCartes(void) {
    for(int i = 0; i < NB_PIOCHE; ++i) {
        if(i < 30) {
            pioche[i] = creerCarte("25 miles", MOVEMENT, 1, "Carte pour avancer de 1 case");
        } else if(i < 50) {
            pioche[i] = creerCarte("50 miles", MOVEMENT, 2, "Carte pour avancer de 2 cases");
        } else if(i < 60) {
            pioche[i] = creerCarte("75 miles", MOVEMENT, 3, "Carte pour avancer de 3 cases");
        } else if(i < 65) {
            pioche[i] = creerCarte("100 miles", MOVEMENT, 4, "Carte pour avancer de 4 cases");
        } else if(i < 66) {
            pioche[i] = creerCarte("As du volant", UNIQUE, 0, "Carte empêchant les accidents");
        } else if(i < 67) {
            pioche[i] = creerCarte("Increvable", UNIQUE, 0, "Carte empêchant les crevaisons");
        } else if(i < 68) {
            pioche[i] = creerCarte("Citerne", UNIQUE, 0, "Carte empêchant les pannes d'essence");
        } else if(i < 69) {
            pioche[i] = creerCarte("Véhicule prioritaire", UNIQUE, 0, "Carte empêchant les ralentissements et feux rouges");
        } else if(i < 77) {
            pioche[i] = creerCarte("Feu vert", AVANTAGE, 0, "Utile au 1er tour et annulant les feux rouges");
        } else if(i < 83) {
            pioche[i] = creerCarte("Réparation", AVANTAGE, 0, "Annule les accidents");
        } else if(i < 85) {
            pioche[i] = creerCarte("Roue de secours", AVANTAGE, 0, "Annule les crevaisons");
        } else if(i < 87) {
            pioche[i] = creerCarte("Pompe à essence", AVANTAGE, 0, "Annule les pannes d'essence");
        } else if(i < 95) {
            pioche[i] = creerCarte("Feu rouge", PIEGE, 0, "Impose un arrêt au joueur ciblé");
        } else if(i < 99) {
            pioche[i] = creerCarte("Crevaison", PIEGE, 0, "Impose un arrêt au joueur ciblé");
        } else if(i < 103) {
            pioche[i] = creerCarte("Panne d'essence", PIEGE, 0, "Impose un arrêt au joueur ciblé");
        } else if(i < 109) {
            pioche[i] = creerCarte("Limitation de vitesse", PIEGE, 0, "Impose un ralentissement au joueur ciblé");
        } else if(i < 117) {
            pioche[i] = creerCarte("Accident", PIEGE, 0, "Impose un arrêt au joueur ciblé");
        } else {
            pioche[i] = creerCarte("Fin de limitation", AVANTAGE, 0, "Annule les ralentissements");
        }
    }
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

int main(int argc, char *argv[]) {
    printf("Bienvenue sur l'interface du serveur\n");

    // On récupère le nombre de joueurs
    int nbJoueur = (questionNbJoueur() - '0'); // conversion des char en int
    while(nbJoueur < 2 || nbJoueur > 4) {
        nbJoueur = questionNbJoueur();
    }
    
    // On récupère la taille du terminal
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    printf("Dimension du terminal = %dx%d\n", ws.ws_col, ws.ws_row); // Contrôle durant le dev, TODO retirer cette ligne

    // On affiche le nombre de cases sur la carte
    printf("La carte contient %d cases\n", verifierNbCases());

    
    struct joueur joueurs[nbJoueur];

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
            }
        }
    }

    for (int i = 0; i<nbJoueur; i++) {
        printf("Joueur %d : Place %d\n", i, joueurs[i].ordre);
    }

    genererCartes();
    printf("Il y a %ld cartes dans la pioche\n", sizeof(pioche)/24);

    return 0;
}