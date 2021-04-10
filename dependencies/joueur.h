#include "carte.h"

struct joueur{
    int ordre;
    char * pseudo;

    struct carte main[7];

    bool isAs;
    bool isCitern;
    bool isIncrevable;
    bool isPrioritaire;

    bool isStun;
    struct carte * stunningCarte;
};

