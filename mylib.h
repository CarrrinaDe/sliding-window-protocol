#define ACK 1
#define NACK 2
#define DONE 3

typedef struct {
	int check; // rezultatul xor daca pachetul contine date din fisier
	      // tip daca pachetul este o confirmare (ack, nack sau done)
	int nr; // index pachet in vector
	char payload[1392]; // camp de date
} pkt;

int xor(char *p);
