all:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -o bin/rshell
	g++ -std=c++11 -Wall -Werror -pedantic ./src/ls.cpp -o bin/ls
rshell:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -o bin/rshell
ls:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/ls.cpp -o bin/ls
run:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic ./src/ls.cpp -o bin/ls
	bin/ls
gdb:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic -g ./src/ls.cpp -o bin/ls
	gdb bin/ls
Rl: ls
	bin/ls -Rl
valgrind:
	if [ ! -d bin ]; then mkdir bin; fi
	g++ -std=c++11 -Wall -Werror -pedantic -g ./src/ls.cpp -o bin/ls
	valgrind --tool=memcheck --leak-check=full --track-origins=yes bin/ls


