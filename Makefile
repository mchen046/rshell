all:
	g++ -std=c++11 -Wall -Werror -pedantic ./src/main.cpp

rshell:
	g++ -std=c++11 -Wall -Werror -pedantic ./src/main.cpp
run:
	g++ -std=c++11 -Wall -Werror -pedantic ./src/main.cpp
	./a.out
gdb: 
	g++ -std=c++11 -Wall -Werror -pedantic ./src/main.cpp -g
	gdb a.out
	
