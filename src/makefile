xinp: main.o args.o nav.o unpack.o
	gcc -o xinp main.o args.o nav.o unpack.o
	make clean

main.o: main.c args.h nav.h unpack.h
	gcc -c main.c

args.o: args.c
	gcc -c args.c

nav.o: nav.c entry.h
	gcc -c nav.c

unpack.o: unpack.c entry.h
	gcc -c unpack.c

clean:
	rm *.o
	mv xinp ..
