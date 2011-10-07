CFLAGS=-Wall -g -lsqlite3 -lyajl -lcurl -arch x86_64

clean:
	rm -f usher
	rm -rf usher.dSYM
