typedef enum {
    LOGIN,
    DISCONNECT,
    DEFAULT,
    CHOICE,
    ENDGAME,
    SCORE,
    ACK,
    PSEUDO,
    ERROR,
    OK,
    FAULT,
    CLOSE_CONNECTION,
    CARD,
    TURN
} comm_type;

typedef struct {
    char msg[256];
    comm_type type;
    int src;
    int dest;
    carte carte;
} t_comm;

