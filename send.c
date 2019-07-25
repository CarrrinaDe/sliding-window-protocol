#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"
#include "mylib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int myceil(float x) {
    if (x == (int) x)
        return (int) x;
    else
        return (int) x + 1;
}

int main(int argc, char** argv) {

    init(HOST,PORT);
    msg r; 
    msg *v; // vector cu pachetele ce vor fi trimise, in ordine
    pkt p;

// calcul marime fereastra
    int SPEED = atoi(argv[2]), DELAY = atoi(argv[3]);
    int BDP = SPEED * DELAY * 1000;
    int WINDOW = BDP / (8 * sizeof(msg));

// extragere informatii despre fisier
    char *filename = strdup(argv[1]);
    int filename_len = strlen(filename);
    int fd = open(filename, O_RDONLY);
    struct stat f_stat; 
    fstat(fd, &f_stat);
    int filesize = f_stat.st_size;

// calcul numar total de pachete necesare
    int total_pkts = myceil(f_stat.st_size / sizeof(p.payload));
    int v_size = total_pkts + 3; // se adauga cele cu numele fisierului si dimensiunea
    v = malloc(v_size * sizeof(msg));

// pregatire pachet nume fisier
    memset(&p, 0, sizeof(pkt)); 
    p.nr = 0;
    memcpy(p.payload, filename, filename_len);
    p.check = xor((char *)&p);
    v[0].len = 2 * sizeof(int) + sizeof(p.payload);
    memcpy(v[0].payload, &p, v[0].len);

// pregatire pachet dimensiune fisier
    memset(&p, 0, sizeof(pkt));
    p.nr = 1;
    sprintf(p.payload, "%d", filesize);
    p.check = xor((char *)&p);
    v[1].len = 2 * sizeof(int) + sizeof(p.payload);
    memcpy(v[1].payload, &p, v[1].len);

// citire din fisier
    int citit, index = 2;
    char buf[1392];
    while ((citit = read(fd, buf, sizeof(p.payload)))) {
        memset(&p, 0, sizeof(pkt));
        p.nr = index;
        memcpy(p.payload, buf, citit);
        p.check = xor((char *)&p);
        v[index].len = 2 * sizeof(int) + citit;
        memcpy(v[index].payload, &p, v[index].len);
        index++;
        memset(buf, 0, sizeof(p.payload));
    }
    close(fd);

    int i, last_confirmed = -1, to_be_sent = 0;
    for (i = 1; i <= WINDOW; i++) { 
        send_message(&v[to_be_sent++]);
        if (to_be_sent >= v_size)
            break;
    }

    while (last_confirmed < v_size - 1) {
        if (recv_message(&r) > 0) {
            p = *((pkt *) r.payload);
            if (p.check == NACK) // am primit NACK
                send_message(&v[p.nr]); // retrimit pachetul
            else { // am primit ACK
                if (p.nr >= v_size - 1) // a fost confirmat ultimul mesaj
                        break;
                if (p.nr > last_confirmed) 
                    last_confirmed = p.nr; 
                if (to_be_sent - last_confirmed < WINDOW && to_be_sent < v_size) 
                    send_message(&v[to_be_sent++]);
                else 
                    send_message(&v[last_confirmed + 1]);
            }
        }
    }

    int done = 0;
    do {
        if (recv_message(&r) > 0) {
            p = *((pkt *) r.payload);
                if (p.check == DONE) // primit confirmarea ca receiver-ul a terminat
                    done = 1;
        }
    } while (!done);

    return 0;
}
