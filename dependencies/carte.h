typedef enum {
    MOVEMENT = 0, UNIQUE = 1, AVANTAGE = 2, PIEGE = 3
} carteType;
struct carte {
    char nom[256]; // Human-readable 
    carteType type;
    int movement; // Valeur du déplacement
    char description[256];
    int ident;
};