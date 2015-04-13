/*
Michael Chen
mchen046
SID: 861108671
CS100 Spring 2015: hw0-rshell
https://www.github.com/mchen046/rshell/src/main.cpp
*/
/*
Worklist:
bug when running: ls -a; echo hello; mkdir test;
	the command exits when mkdir test finishes.
	concatenates an extra " e" after original command
fix: have to have a NULL at the end of the char* command
currently trying to implement connectors || &&
*/
/*
Fixed:
problem when running ls -a multiple times
	ls -a breaks on the second execution "error in execvp: Bad address
*/
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdio.h>
#include <boost/tokenizer.hpp>
#include <boost/token_iterator.hpp>

using namespace std;
using namespace boost;

char** parseString(string cmd);

void connectorAnd();

void bitOr();

void printCmdLinePart(char**cmdLinePart, int cmdLinePartSize){
	for(int i = 0; i<=cmdLinePartSize+1; i++){
		cout << cmdLinePart[i] << endl;
	}
}

void checkExitCmd(char* cmdText){ //exit command
	char exitText[] = {'e','x', 'i', 't'};
	bool same = true;
	for(int i = 0; cmdText[i]!='\0' && same; i++){
		if(cmdText[i]!=exitText[i])
			same = false;
	}
	if(same){
		cout << "exit command called\n";
		exit(1);
	}
}

int main(){
	while(1){
		string cmd;
		cout << "$ ";
		getline(cin, cmd);

		//convert string to char[]
		char *cmdArray = new char[cmd.size()];
		for(int i=0; i<static_cast<int>(cmd.size()); i++){
			cmdArray[i] = cmd.at(i);
		}
		cout << "cmdArray: " << cmdArray << endl;
		
		bool cmdLineDone = false;
		char *cmdLine = strtok(cmdArray, ";"); //parsing ";"
		while(!cmdLineDone){
			cout << "cmdLine: " << cmdLine << endl;
			//parsing " "
			string str = static_cast<string>(cmdLine);
			char_separator<char> delim(" ");
			tokenizer< char_separator<char> > mytok(str, delim);

			//finding size of cmdLinePart
			int cmdLinePartSize = 0;
			for(tokenizer<char_separator<char> >::iterator it = mytok.begin(); it!=mytok.end(); it++){
				cmdLinePartSize++;
			}
			
			//creating cmdLinePart
			char **cmdLinePart = new char*[cmdLinePartSize+1];

			//cout << "cmdLinePartSize: " << cmdLinePartSize << endl;
			int i = 0;
			for(tokenizer<char_separator<char> >::iterator it = mytok.begin(); it!=mytok.end(); it++){
				string cmdString = static_cast<string>(*it);
				char *token = new char[cmdString.size()];
				for(unsigned int j = 0; j!=cmdString.size(); j++){
					token[j] = cmdString[j];
				}

				cmdLinePart[i] = token;
				i++;

				tokenizer<char_separator<char> >::iterator itA = it;
				if(++itA==mytok.end()){
					//cout << "adding NULL to end of cmdLinePart\n";
					cmdLinePart[i+1] = NULL;
				}
			}
			//end creating cmdLinePart


			//parsing && or ||
			//str = static_cast<string>(


			//printCmdLinePart(cmdLinePart, cmdLinePartSize);
			cmdLine = strtok(NULL, ";"); //parse ";"
			if(cmdLine==NULL){
				cmdLineDone = true;
			}
			checkExitCmd(cmdLinePart[0]); //check for exit command

			//process spawning
			int pid = fork();
			//cout << "pid: " << pid << endl;
			if(pid == -1){ //error
			perror("fork() error");
			cout << "exiting pid error msg\n";
				exit(1);
			}
			else if(pid == 0){ //child process
				//cout << "in child process\n";
				if(execvp(cmdLinePart[0], cmdLinePart) == -1){
					perror("error in execvp");
				}
				cout << "exiting child process\n";
				exit(1);
			}
			else if(pid > 0){ //parent process
				//cout << "in parent process\n";
				if(wait(0) == -1){
					perror("error in wait()");
				}
			}
			//delete[] *cmdLinePart;
		}
		//delete[] cmdLine;
		//delete[] cmdArray;
	}
	cout << "reached end of program\n";
	return 0;
}
