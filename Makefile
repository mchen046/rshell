all:
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp

rshell:
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp
run:
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp
	./a.out
gdb: 
	g++ -std=c++11 -Wall -Werror -pedantic ./src/rshell.cpp -g
	gdb a.out
	
