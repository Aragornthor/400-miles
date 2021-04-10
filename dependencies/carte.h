#include "carteType.h"

struct carte {
    char * nom; // Human-readable 
    struct carteType type;
    int movement; // Valeur du déplacement
    char * description;
};