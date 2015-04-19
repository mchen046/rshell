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
	rmdir test: execvp succeeds, but command itself fails
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
#include <sys/wait.h>
#include <sys/types.h>
#include <wait.h>
#include <string.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <boost/tokenizer.hpp>
#include <boost/token_iterator.hpp>

using namespace std;
using namespace boost;

int sizeOfPart(char *cmdLine){
	string str = static_cast<string>(cmdLine);
	char_separator<char> delim(" ");
	tokenizer< char_separator<char> > mytok(str, delim);

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

	int size= 0;
	for(tokenizer<char_separator<char> >::iterator it = mytok.begin(); it!=mytok.end(); it++){
		size++;
	}

	//creating cmdLinePart
	char **cmdLinePart = new char*[size+1];

	//cout << "cmdLinePartSize: " << cmdLinePartSize << endl;
	int i = 0;
	for(tokenizer<char_separator<char> >::iterator it = mytok.begin(); it!=mytok.end(); it++){
		string cmdString = static_cast<string>(*it);
		char *token = new char[cmdString.size()];
		for(unsigned int j = 0; j<cmdString.size(); j++){
			token[j] = cmdString[j];
		}
		cmdLinePart[i] = token;
		tokenizer<char_separator<char> >::iterator itA = it;
		if(++itA==mytok.end()){
			//cout << "adding NULL to end of cmdLinePart\n";
			cmdLinePart[i+1] = NULL;
		}
		i++;
		//delete[] token;
	}
	return cmdLinePart;
	//end creating cmdLinePart
}

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

bool hasText(char* cmdText, string text){
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



int execCmd(char** cmdD){ //process spawning
	int pid = fork();
	int status = 0;
	//cout << "cmdD[0]: " << cmdD[0] << endl;
	if(pid == -1){ //error
		perror("fork() error");
		_exit(2);
	}
	else if(pid == 0){ //child process
		if(execvp(cmdD[0], cmdD) == -1){
			perror("error in execvp"); //status becomes 256/512
			_exit(2);
		}
	}
	else if(pid > 0){ //parent process
		if(wait(&status)==-1){		
			perror("error in wait()");
			_exit(2);
		}
	}
	//cout << "status: " << status << endl;
	if(status!=0){ //command fails 
		//cout << "command fails!\n";
		return -1;
	}
	return status; //status defaults to 0 if successful
}

void printcmd(vector<char*> cmd){
	cout << "---------------\ncmd\n";
	for(unsigned int i = 0; i<cmd.size(); i++){
		cout << "cmd[" << i << "]: " << cmd[i] << endl;
	}
	cout << "---------------\n";
}

void parseDelim(vector<char*> &cmdC, char *cmdB, char const * delim, char *&ptr){
	char *token;
	token = strtok_r(cmdB, delim, &ptr);
	cmdC.push_back(token);
	while(hasText(ptr, delim)){
		token = strtok_r(NULL, delim, &ptr);
		cmdC.push_back(token);
		cout << "ptr: " << ptr << endl;	
	}
	token = strtok_r(NULL, delim, &ptr);
	cmdC.push_back(token);
	//delete[] ptr;
	//delete[] token;
}

int parseMaster(char* cmdB){
	//cout << "cmdB: " << cmdB << endl;
	char *ptr;
	vector <char*> cmdC;
	if((hasText(cmdB, "|") || hasText(cmdB, "\"") || hasText(cmdB, "(") || hasText(cmdB, ")")) && !hasText(cmdB, "&&") && !hasText(cmdB, "||")){
		//invalid command
		cout << "rshell: " << cmdB  <<": command not found\n";
	}
	else if(!hasText(cmdB, "&&") && !hasText(cmdB, "||")){
		//execute
		execCmd(parseSpace(cmdB));
	}
	else if(hasText(cmdB, "&&") && !hasText(cmdB, "||")){
		//parse && loop	
		parseDelim(cmdC, cmdB, "&&", ptr);
		//exec
		printcmd(cmdC);
		bool boolDone = false;
		//cout << "cmdC.size(): " << cmdC.size() << endl;
		for(unsigned int i = 0; i<cmdC.size() && !boolDone; i++){
			if(execCmd(parseSpace(cmdC[i]))==-1){ //command fails
				boolDone = true;
			}
		}
	}
	else if(!hasText(cmdB, "&&") && hasText(cmdB, "||")){
		//parse || loop	
		parseDelim(cmdC, cmdB, "||", ptr);
		//exec
		printcmd(cmdC);
		bool boolDone = false;
		//cout << "cmdC.size(): " << cmdC.size() << endl;
		for(unsigned int i = 0; i<cmdC.size() && !boolDone; i++){
			if(!execCmd(parseSpace(cmdC[i]))){ //command succeeds
				boolDone = true;
			}
		}
	}
	else if(hasText(cmdB, "&&") && hasText(cmdB, "||")){
		//parse both && and || loop
		parseDelim(cmdC, cmdB, "||", ptr);
		//cout << "cmdC[0]: " << cmdC[0] << endl;
		if(hasText(cmdC[0], "&&")){
			cmdC.clear();
			char *ptrA;
			parseDelim(cmdC, cmdB, "&&", ptrA);
		}
		
	}
	return 1;
}
int main(){
	while(1){
		string cmd;
		cout << "$ ";
		getline(cin, cmd);

		//convert string to char[]
		char *cmdA = new char[cmd.size()];
		for(int i=0; i<static_cast<int>(cmd.size()); i++){
			cmdA[i] = cmd.at(i);
		}
		
		char *cmdB = strtok(cmdA, ";"); //parsing ";"
		bool cmdBDone = false;

		while(!cmdBDone){
			//cout << "cmdB: " << cmdB << endl;
			parseMaster(cmdB);	
			//cout << "parsed and executed!" << endl;
			cmdB = strtok(NULL, ";");
			if(cmdB==NULL){
				//cout << "no more semicolons\n";
				cmdBDone = true;
			}
		}
	}
}



























			//cout << "cmdLine: " << cmdLine << endl;

			//parsing && or ||
			/*char *parentPtr, *token;
			char **cmdConnector;
			bool nextCmd = true;
			bool nextConnector = false;
			bool finalCmd = true;*/
			//bool free = true;
		
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
			



			/*if(!containsText(cmdLine, "||") && !containsText(cmdLine, "&&")){
				//cout << "nextCmd set to false\n";
				nextCmd = false;
				//free = false;
			}
			else if(containsText(cmdLine, "||") && !containsText(cmdLine, "&&")){
				//cout << "initial parse ||\n";
				token = strtok_r(cmdLine, "||", &parentPtr);
				cmdConnector = parseSpace(token);
				if(execCmd(cmdConnector) != -1){ //succeeds
					//cout << "command fails!\n";
					finalCmd = false;
					nextCmd = false;
					cmdLineDone = true;
				}
				else if(!containsText(parentPtr, "&&") && !containsText(parentPtr, "||")){
					token = strtok_r(NULL, "||", &parentPtr);
					cmdLine = token;
					nextCmd = false;
				}
			}
			else if(containsText(cmdLine, "&&") && !containsText(cmdLine, "||")){
				//cout << "initial parse &&\n";
				token = strtok_r(cmdLine, "&&", &parentPtr);
				cmdConnector = parseSpace(token);
				if(execCmd(cmdConnector) == -1){ //fails
					//cout << "command fails!\n";
					finalCmd = false;
					nextCmd = false;
					cmdLineDone = true;
				}
				else if(!containsText(parentPtr, "&&") && !containsText(parentPtr, "||")){
					token = strtok_r(NULL, "&&", &parentPtr);
					cmdLine = token;
					nextCmd = false;

				}
			}
			else{
				token = strtok_r(cmdLine, "&&", &parentPtr);
			}

			
			while(nextCmd){
				while(containsText(parentPtr, "||") && nextCmd && !nextConnector){
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
						finalCmd = false;
					}//command succeeds, move on to next one after semicolon
				}
				nextConnector = false;
				while(containsText(parentPtr, "&&") && nextCmd && !nextConnector){
					cout << "in && connector\n";
					cout << "token: " << token << endl;
					token = strtok_r(NULL, "&&", &parentPtr); //parse &&
					cout << "token after parsing &&: " << token << endl;
					cout << "parentPtr: " << parentPtr << endl;
					cmdConnector = parseSpace(token); //parse space
					if(execCmd(cmdConnector)==-1){ //if command fails
						nextCmd = false;
						finalCmd = false;
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
			if(free){
				cout << "deleting cmdConnector\n";
				int size = sizeOfPart(token);
				cout << "size: " << size << endl;
				for(int i = 0; i < size; i++){
					cout << "i: " << i << endl;
					cout << "cmdConnector[i]: " << cmdConnector[i] << endl;
					delete[] cmdConnector[i];
				}
				delete[] cmdConnector;
			}

			if(finalCmd){
				char**cmdLinePart = parseSpace(cmdLine);
				//int size = sizeOfPart(cmdLine);

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
			
				cout << "deleting cmdLinePart\n";
				cout << "size: " << size << endl;
				for(int i = 0; i < size; i++){
					cout << "i: " << i << endl;
					cout << "cmdLinePart[i]: " << cmdLinePart[i] << endl;
					delete[] cmdLinePart[i];
				}
				cout << "helloooo" << endl;
				delete[] cmdLinePart;
			}
			//delete[] parentPtr;
			//delete[] token;
		}
		//delete[] cmdLine;
		//delete[] cmdArray;*/
	
	
