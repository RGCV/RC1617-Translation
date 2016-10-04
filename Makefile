all: user tcs

user: src/user.c src/rctr.h src/rctr.c
	cd src && \
	gcc -Wall -g -o ../user rctr.c user.c

tcs: src/tcs.c src/rctr.h src/rctr.c
	cd src && \
	gcc -Wall -g -o ../tcs rctr.c tcs.c
	
clean:
	rm -rf user tcs

