#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"
#include "mylib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int myceil(float x) {
    if (x == (int) x)
        return (int) x;
    else
        return (int) x + 1;
}

// functie ce returneaza indexul primului pachet neconfirmat din vector
int get_first_missing(int *confirmed, int size, int last_confirmed) {
    int i = 0;
    for (i = last_confirmed + 1; i < size; i++)
        if (!confirmed[i])
            return i;
    return -1;
}

int main(int argc, char** argv){
    
    msg *v; // vector in care voi stoca pachetele primite
    int v_size = 1000; // initial ii dau dimensiunea maxima a ferestrei (1000) 
    v = malloc(v_size * sizeof(msg));
    int *confirmed; // confirmed[i] are valoarea 0 daca pachetul i nu a fost primit in regula si 1 altfel
    confirmed = calloc(v_size, sizeof(int)); 
    int last_confirmed = -1, got_filesize = 0, last_nack = -1;

    init(HOST,PORT);

    msg r, t; 
    pkt p, conf;
// pregatesc pachetul folosit pentru trimiterea confirmarilor
    memset(&conf, 0, sizeof(pkt)); 
    memcpy(conf.payload, "CONFIRMATION MSG", strlen("CONFIRMATION MSG"));
    memset(&t, 0, sizeof(msg));
    t.len = 2 * sizeof(int) + strlen(conf.payload) + 1;
    memcpy(t.payload, &conf, sizeof(conf));

    do {
        if (recv_message_timeout(&r, 2) > 0) {
            p = *((pkt *) r.payload);
            if (p.check == xor((char *)&p)) { 
                if (!got_filesize && p.nr == 1) { // a venit pachetul cu dimensiunea fisierului
                    v_size = myceil(atoi(p.payload) / sizeof(p.payload)) + 3; // deci pot afla cate pachete am de primit
                    v = (msg *)realloc(v, v_size * sizeof(msg));
                    confirmed = (int *)realloc(confirmed, v_size * sizeof(int));
                    got_filesize = 1;
                }
                if (confirmed[p.nr]) 
                    continue;
                if (p.nr == last_confirmed + 1) { // a venit primul pachet care lipsea
                    v[p.nr] = r;
                    confirmed[p.nr] = 1;
                    int f = get_first_missing(confirmed, v_size, last_confirmed);
                    if (f == -1) // actualizez last_confirmed
                        last_confirmed = v_size - 1; 
                    else
                        last_confirmed = f - 1;
                    conf.check = ACK; 
                    conf.nr = last_confirmed; 
                    memcpy(t.payload, &conf, 2 * sizeof(int));
                    send_message(&t); // trimit ACK
                } 
                else { // a venit altul neconfirmat
                    v[p.nr] = r;
                    confirmed[p.nr] = 1; 
                    if (last_nack < last_confirmed + 1) { // n-am trimis NACK pentru el
                        conf.check = NACK;
                        conf.nr = last_confirmed + 1; 
                        memcpy(t.payload, &conf, 2 * sizeof(int));
                        send_message(&t); // trimit NACK
                        last_nack = conf.nr;
                    } else {
                        conf.check = ACK;
                        conf.nr = last_confirmed; 
                        memcpy(t.payload, &conf, 2 * sizeof(int));
                        send_message(&t); // trimit ACK cu ultimul pachet primit in ordine
                    }
                }
            }
        } else { // a expirat timeout-ul
            conf.check = ACK;
            conf.nr = last_confirmed; 
            memcpy(t.payload, &conf, 2 * sizeof(int));
            send_message(&t); // trimit ACK cu ultimul pachet primit in ordine
        }
    } while (last_confirmed < v_size - 1);
    free(confirmed);

// construiesc numele fisierului nou
    p = *((pkt *) v[0].payload);
    char prefix[] = "recv_";
    char *filename = NULL;
    int x = strlen(prefix) + strlen(p.payload) + 1;
    filename = malloc(x);
    strcpy(filename, prefix);
    int i;
    for (i = 5; i <= x - 2; i++) {
        filename[i] = p.payload[i - 5];
    }
    filename[x - 1] = '\0';

// scriere in fisier
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 777); 
    free(filename);
    int dim = atoi(v[1].payload + 2 * sizeof(int)), scris = 0, nr = 2;
    while (scris < dim && nr < v_size) {
        memset(&p, 0, sizeof(pkt));
        p = *((pkt *) v[nr].payload);
        scris += write(fd, p.payload, v[nr].len - 2 * sizeof(int));
        nr++;
    }
    close(fd);

// anunt sender-ul ca am terminat
    conf.check = DONE; 
    memcpy(t.payload, &conf, 2 * sizeof(int));
    send_message(&t);
    return 0;
}
