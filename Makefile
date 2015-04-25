all:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -o bin/rshell
rshell:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -o bin/rshell
run:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -o bin/rshell
	bin/rshell
gdb:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic -g ./src/rshell.cpp -o bin/rshell
	gdb bin/rshell

