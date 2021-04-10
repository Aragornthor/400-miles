#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "../dependencies/map.h"
#include "../dependencies/joueur.h"


#define true 1
#define false 0

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
    for(int compteur = 0; compteur < nbJoueur; ++compteur) {
        joueurs[compteur].ordre = lancerDe(nbJoueur);
        printf("Joueur %d a tiré %d\n", compteur, joueurs[compteur].ordre);
    }


    return 0;
}