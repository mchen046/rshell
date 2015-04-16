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

fix boolean operators? partially working
	compact and optimize
execCmd is not able to tell if a command fails or not
	success variable assignment is lost as soon as child exits
	cannot tell if success or not while in parent
compact and optimize!!!
fix memory leaks
*/
/*
Fixed:
problem when running ls -a multiple times
	ls -a breaks on the second execution "error in execvp: Bad address
*/
/*
README:
ex command?
parentheses and quotes
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

int sizeOfPart(tokenizer<char_separator<char> > mytok){
	int size= 0;
	for(tokenizer<char_separator<char> >::iterator it = mytok.begin(); it!=mytok.end(); it++){
		size++;
	}
	return size;
}

char** parseSpace(char *cmdLine){
	//parsing " "
	string str = static_cast<string>(cmdLine);
	char_separator<char> delim(" ");
	tokenizer< char_separator<char> > mytok(str, delim);

	int cmdLinePartSize = sizeOfPart(mytok);
			
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
	return cmdLinePart;
	//end creating cmdLinePart
}

void connectorAnd();

void connectorOr();

bool isText(char* cmdText, string text){
	if(!(cmdText[text.size()-1]==text[text.size()-1] && cmdText[text.size()]=='\0' && text[text.size()]=='\0')){
		return false;
	}
	for(unsigned int i = 0; cmdText[i]!='\0' && i<text.size(); i++){
		if(cmdText[i]!=text[i])
			return false;
	}
	return true;
}

bool containsText(char* cmdText, string text){
	unsigned int count = 0;
	int j = 0;
	for(unsigned int i = 0; cmdText[i]!='\0'; i++){
		if(cmdText[i]!=text[j]){
			count = 0;
			j = 0;
		}
		if(cmdText[i]==text[j]){
			count++;
			j++;
		}
		if(count == text.size()){
			return true;
		}
	}
	return false;
}

void printCmdLinePart(char**cmdLinePart, int cmdLinePartSize){ //buggy when printing
	for(int i = 0; i<=cmdLinePartSize+1; i++){
		cout << cmdLinePart[i] << endl;
	}
}

int execCmd(char** cmdLinePart){ //process spawning
	int pid = fork();
	int success = -1;
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
		else{
			success = 1;
		}
	}
	cout << "success: " << success << endl;
	return success;
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

			//parsing && or ||
			char *parentPtr, *token;
			char **cmdConnector;
			bool nextCmd = true;
			bool nextConnector = false;
			bool finalCmd = true;


		

			//setting initial token
			/*if(containsText(cmdLine,"&&") && !containsText(cmdLine, "||")){
				token = strtok_r(cmdLine, "&&", &parentPtr);
			}
			else if(containsText(cmdLine, "||") && !containsText(cmdLine, "&&")){
				token = strtok_r(cmdLine, "||", &parentPtr);
			}
			else if(containsText(cmdLine, "||") && containsText(cmdLine, "&&")){
				token = strtok_r(cmdLine, "&&", &parentPtr);
			}*/
			



			if(!containsText(cmdLine, "||") && !containsText(cmdLine, "&&")){
				//cout << "nextCmd set to false\n";
				nextCmd = false;
			}
			else if(containsText(cmdLine, "||") && !containsText(cmdLine, "&&")){
				//cout << "initial parse ||\n";
				token = strtok_r(cmdLine, "||", &parentPtr);
				if(!containsText(parentPtr, "&&") && !containsText(parentPtr, "||")){
					cmdConnector = parseSpace(token);
					if(execCmd(cmdConnector)!=-1){ //succeeds
						finalCmd = false;
						nextCmd = false;
					}
					else{
						token = strtok_r(NULL, "||", &parentPtr);
						cmdLine = token;
						nextCmd = false;
					}
				}
			}
			else if(containsText(cmdLine, "&&") && !containsText(cmdLine, "||")){
				//cout << "initial parse &&\n";
				token = strtok_r(cmdLine, "&&", &parentPtr);
				if(!containsText(parentPtr, "&&") && !containsText(parentPtr, "||")){
					cmdConnector = parseSpace(token);
					if(execCmd(cmdConnector)==-1){ //fails
						cout << "command failed!\n";
						finalCmd = false;
						nextCmd = false;
					}
					else{
						token = strtok_r(NULL, "&&", &parentPtr);
						cmdLine = token;
						nextCmd = false;
					}
				}
			}
			else{
				token = strtok_r(cmdLine, "&&", &parentPtr);
			}
			



			while(nextCmd){
				while(containsText(token, "||") && nextCmd && !nextConnector){
					cout << "in || connector\n";
					cout << "token: " << token << endl;
					token = strtok_r(NULL, "||", &parentPtr); //parse ||
					cout << "token after parsing ||: " << token << endl;
					cout << "parentPtr: " << parentPtr << endl;
					cmdConnector = parseSpace(token); //parse space
					if(execCmd(cmdConnector)==-1){ //if command fails
						
						if(containsText(parentPtr,"&&") && !containsText(parentPtr, "||")){
							nextConnector = true;
						}
						else if(containsText(parentPtr, "||") && !containsText(parentPtr, "&&")){
							token = strtok_r(NULL, "||", &parentPtr);
						}
						else if(containsText(parentPtr, "||") && containsText(parentPtr, "&&")){
							token = strtok_r(NULL, "&&", &parentPtr);
						}	
						nextCmd = true;
						if(!containsText(parentPtr, "||") && !containsText(parentPtr, "&&")){
							token = strtok_r(NULL, "||", &parentPtr);
							cmdLine = token;
							nextCmd = false;
						}

					}
					else{
						nextCmd = false;
					}//command succeeds, move on to next one after semicolon
				}
				nextConnector = false;
				while(containsText(token, "&&") && nextCmd && !nextConnector){
					cout << "in && connector\n";
					cout << "token: " << token << endl;
					token = strtok_r(NULL, "&&", &parentPtr); //parse &&
					cout << "token after parsing &&: " << token << endl;
					cout << "parentPtr: " << parentPtr << endl;
					cmdConnector = parseSpace(token); //parse space
					if(execCmd(cmdConnector)==-1){ //if command fails
						nextCmd = false;
					}
					else{ //command succeeds

						if(containsText(parentPtr,"&&") && !containsText(parentPtr, "||")){
							token = strtok_r(NULL, "&&", &parentPtr);	
						}
						else if(containsText(parentPtr, "||") && !containsText(parentPtr, "&&")){
							nextConnector = true;
						}
						else if(containsText(parentPtr, "||") && containsText(parentPtr, "&&")){
							token = strtok_r(NULL, "&&", &parentPtr);
						}	
						nextCmd = true;
						if(!containsText(parentPtr, "||") && !containsText(parentPtr, "&&")){
							nextCmd = false;
							token = strtok_r(NULL, "&&", &parentPtr);
							cmdLine = token;
						}
					}
				}
				nextConnector = false;
			}
			
			if(finalCmd){
				//cout << "final token: " << token << endl;

				char**cmdLinePart = parseSpace(cmdLine);

				cmdLine = strtok(NULL, ";"); //parse ";"

				if(cmdLine==NULL){
					cmdLineDone = true;
				}

				if(isText(cmdLinePart[0], "exit")){
					return 0;
				}

				//checking for quotes and parentheses
				bool valid = true;
				for(int i = 0; cmdLinePart[i]!='\0' && valid; i++){
					if(containsText(cmdLinePart[i], "\"") || containsText(cmdLinePart[i], "(") || containsText(cmdLinePart[i], ")")){
						cout << "Syntax error! Command contains parentheses and/or quotes.\n";
						valid = false;
					}
				}

				//process spawning
				if(valid){
					execCmd(cmdLinePart);
				}
			}
		}
		//delete[] cmdLine;
		//delete[] cmdArray;
	}
	cout << "reached end of program\n";
	return 0;
}
