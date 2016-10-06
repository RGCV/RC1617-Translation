all: user tcs

user: src/user.c src/rctr.h src/rctr.c
	mkdir -p bin
	cd src && \
	gcc -Wall -g -o ../bin/user rctr.c user.c

tcs: src/tcs.c src/rctr.h src/rctr.c
	mkdir -p bin
	cd src && \
	gcc -Wall -g -o ../bin/tcs rctr.c tcs.c
	
clean:
	rm -rf bin

