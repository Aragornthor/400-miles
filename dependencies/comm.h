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
    CLOSE_CONNECTION
} comm_type;

typedef struct {
    char msg[256];
    comm_type type;
    int src;
    int dest;
} t_comm;

