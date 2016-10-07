all: user tcs trs

user: src/user.c src/rctr.h src/rctr.c
	mkdir -p bin
	cd src && \
	gcc -Wall -g -o ../bin/user rctr.c user.c

tcs: src/tcs.c src/trs_list.h src/trs_list.c src/rctr.h src/rctr.c
	mkdir -p bin
	cd src && \
	gcc -Wall -g -o ../bin/tcs trs_list.c rctr.c tcs.c

trs: src/trs.c src/rctr.h src/rctr.c
	mkdir -p bin
	cd src && \
	gcc -Wall -g -o ../bin/trs rctr.c trs.c

clean:
	rm -rf bin

