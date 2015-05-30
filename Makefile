all:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -o bin/rshell
	g++ -std=c++11 -Wall -Werror -pedantic ./src/cp.cpp -o bin/cp
	g++ -std=c++11 -Wall -Werror -pedantic ./src/ls.cpp -o bin/ls
	g++ -std=c++11 -Wall -Werror -pedantic ./src/mv.cpp -o bin/mv
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rm.cpp -o bin/rm
rshell:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -o bin/rshell
cp:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/cp.cpp -o bin/cp
ls:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/ls.cpp -o bin/ls
run:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -o bin/rshell
	bin/rshell
gdb:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic -g ./src/rshell.cpp -o bin/rshell
	gdb bin/rshell
valgrind:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic -g ./src/rshell.cpp -o bin/rshell
	valgrind --tool=memcheck --leak-check=full --track-origins=yes bin/rshell
.PHONY: eif p test
test: p eif
eif:
	rm eif*; echo one > eif; echo two > eif2; echo three > eif3; echo four > eif4; echo five > eif5; echo six > eif6; echo seven > eif7; echo eight > eif8; echo nine > eif9	
p:
	echo 1; cat eif; echo ""; echo 2; cat eif2; echo ""; echo 3; cat eif3; echo ""; echo 4; cat eif4; echo ""; echo 5; cat eif5; echo ""; echo 6; cat eif6; echo ""; echo 7; cat eif7; echo ""; echo 8; cat eif8; echo ""; echo 9; cat eif9; echo "";

