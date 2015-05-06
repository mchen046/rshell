all:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -o bin/rshell
	g++ -std=c++11 -Wall -Werror -pedantic ./src/cp.cpp -o bin/cp
	g++ -std=c++11 -Wall -Werror -pedantic ./src/ls.cpp -o bin/ls
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
