all: send recv

link_emulator/lib.o:
	$(MAKE) -C link_emulator

myfunc.o: myfunc.c
	gcc -Wall -g -c myfunc.c

send: send.o link_emulator/lib.o myfunc.o
	gcc -g send.o myfunc.o link_emulator/lib.o -o send

recv: recv.o link_emulator/lib.o myfunc.o
	gcc -g recv.o myfunc.o link_emulator/lib.o -o recv

.c.o:
	gcc -Wall -g -c $?

clean:
	$(MAKE) -C link_emulator clean
	-rm -f *.o send recv
