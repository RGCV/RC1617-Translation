all: user tcs trs

deploy: user tcs trs
	mkdir -p proj_50/{trs,user}
	mv bin/user proj_50/user/
	mv bin/trs proj_50/trs/
	mv bin/tcs proj_50/
	cp src/trs\ resources/* proj_50/trs/
	cp src/user\ resources/* proj_50/user/

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
	cp src/trs\ resources/* bin/
	cd src && \
	gcc -Wall -g -o ../bin/trs rctr.c trs.c

clean:
	rm -rf bin proj_50
