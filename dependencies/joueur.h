#include "connexion.h"

struct joueur {
    int pid;
    int ordre;
    char pseudo[256];
    bool connected;
    int id_client;

    void * main;

    t_connexion connexion;

    bool isAs;
    bool isCitern;
    bool isIncrevable;
    bool isPrioritaire;

    bool stopped, accident, slowed, fuel, tire;

    bool isStun;
    void * stunningCarte;

    int traveled;
};

