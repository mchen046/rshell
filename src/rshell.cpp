/*
Michael Chen mchen046@ucr.edu
SID: 861108671
CS100 Spring 2015: hw0-rshell
https://www.github.com/mchen046/rshell/src/rshell.cpp
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

bool exitSeen = false;

void printcmd(vector<char*> cmd){ //debugging cmdC
	cout << "---------------\ncmd\n";
	for(unsigned int i = 0; i<cmd.size(); i++){
		cout << "cmd[" << i << "]: " << cmd[i] << endl;
	}
	cout << "---------------\n";
}

int sizeOfPart(char *cmdLine){ //finds size to allocate for execution
	string str = static_cast<string>(cmdLine);
	char_separator<char> delim(" ");
	tokenizer< char_separator<char> > mytok(str, delim);
	int size= 0;
	for(tokenizer<char_separator<char> >::iterator it = mytok.begin(); it!=mytok.end(); it++){
		size++;
	}
	return size;
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

bool isNumericOnly(const string &str){
	return all_of(str.begin(), str.end(), ::isdigit);
}

char** parseSpace(char *cmd){
	string str = static_cast<string>(cmd); //parsing " "	
	char_separator<char> delim(" ");
	tokenizer< char_separator<char> > mytok(str, delim);

	int size = 0;
	for(tokenizer<char_separator<char> >::iterator it = mytok.begin(); it!=mytok.end(); it++){
		size++;
	}
	
	//creating cmdExec
	char **cmdExec = new char*[size+1];

	int i = 0;
	for(tokenizer<char_separator<char> >::iterator it = mytok.begin(); it!=mytok.end(); it++){
		string cmdString = static_cast<string>(*it);
		char *token = new char[cmdString.size()+1];
		for(unsigned int j = 0; j<cmdString.size(); j++){
			token[j] = cmdString[j];
			if(j+1==cmdString.size()){
				token[j+1] = '\0';
			}
		}
		cmdExec[i] = token;
		tokenizer<char_separator<char> >::iterator itA = it;
		if(++itA==mytok.end()){
			cmdExec[i+1] = NULL;
		}
		i++;	
		//deallocate token?
	}
	if(isText(cmdExec[0], "exit") || isText(cmdExec[0], "exit;")){ //exit executable
		exitSeen = true;
		if(cmdExec[1]!=NULL && !isNumericOnly(static_cast<string>(cmdExec[1]))){
			cout << "rshell: exit: " << cmdExec[1] << ": numeric argument required" << endl;
			exit(0); //exit
		}
		else if(cmdExec[1]!=NULL && cmdExec[2]!=NULL){
			cout << "rshell: exit: too many arguments" << endl;
		}
		else{
			exit(0); //exit
		}
	}
	return cmdExec;
	//end creating cmdExec
	//deallocate cmdExec?
}

int execCmd(char** cmdD){ //process spawning
	if(exitSeen){
		return -1;
	}
	exitSeen = false;
	int pid = fork();
	int status = 0;
	//cout << "cmdD[0]: " << cmdD[0] << endl;
	if(pid == -1){ //error
		perror("fork() error");
		_exit(2);
	}
	else if(pid == 0){ //child process
		//cout << "cmdD[0]: " << cmdD[0] << endl;
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


void parseDelim(vector<char*> &cmdC, char *cmdB, char const * delim, char **ptr){
	char *token = strtok_r(cmdB, delim, &*ptr);
	cmdC.push_back(token);
	while(hasText(*ptr, delim)){
		token = strtok_r(NULL, delim, &*ptr);
		cmdC.push_back(token);
	}
	token = strtok_r(NULL, delim, *&ptr);
	cmdC.push_back(token);
	//delete[] ptr;
	//delete[] token;
}

int cAND(vector<char*> &cmdC, char *cmdB, char **ptr){ //parse and execute && command
	parseDelim(cmdC, cmdB, "&&", &*ptr);
	int success = 1;
	for(unsigned int i = 0; i<cmdC.size() && success!=-1; i++){
		if(execCmd(parseSpace(cmdC[i]))==-1){ //command fails
			success = -1;
		}
	}
	return success;
}

int cOR(vector<char*> &cmdC, char *cmdB, char **ptr){ //parse and execute || command
	parseDelim(cmdC, cmdB, "||", &*ptr);
	int success = -1;
	for(unsigned int i = 0; i<cmdC.size() && success==-1; i++){
		if(!execCmd(parseSpace(cmdC[i]))){ //command succeeds
			success = 1;
		}
	}
	return success;
}

bool onlySpace(string str){
	for(unsigned int i = 0; i<str.size(); i++){
		if(str.at(i)!=' '){
			return false;
		}
	}
	return true;
}


bool checkConnector(string cmd, string &stringToken){
	bool valid = true;
	bool space = false;
	for(unsigned int i = 0; i<cmd.size() && valid; i++){
		if(cmd.at(i)=='&'){
			for(unsigned int j = i+1; j<cmd.size() && valid; j++){
				if(cmd.at(j)=='|'){
					stringToken = "|";
					valid = false;
				}
				else if(cmd.at(j)=='&'){
					if(space){
						stringToken = "&";
						if(j+1<cmd.size() && cmd.at(j+1)=='&'){
							stringToken = "&&";
						}
						valid = false;
					}
					for(unsigned int k = j+1; k<cmd.size() && valid; k++){
						if(cmd.at(k)=='&'){
							stringToken = "&";
							if(space){
								stringToken = "&";
								if(k+1<cmd.size() && cmd.at(k+1)=='&'){
									stringToken = "&&";
								}
							}
							valid = false;
						}
						else if(cmd.at(k)=='|'){
							stringToken = "||";
							if(space){
								stringToken = "|";
							}
							valid = false;
						}
						else if(cmd.at(k)==' '){
							space = true;
						}
					}
				}
				else if(cmd.at(j)==' '){
					space = true;
				}
			}
		}
		else if(cmd.at(i)=='|'){
			for(unsigned int j = i+1; j<cmd.size() && valid; j++){
				if(cmd.at(j)=='&'){
					stringToken = "&";
					valid = false;
				}
				else if(cmd.at(j)=='|'){
					if(space){
						stringToken = "|";
						if(j+1<cmd.size() && cmd.at(j+1)=='|'){
							stringToken = "||";
						}
						valid = false;
					}
					for(unsigned int k = j+1; k<cmd.size() && valid; k++){
						if(cmd.at(k)=='|'){
							stringToken = "|";
							if(space){
								stringToken = "|";
								if(k+1<cmd.size() && cmd.at(k+1)=='|'){
									stringToken = "||";
								}
							}
							valid = false;
						}
						else if(cmd.at(k)=='&'){
							stringToken = "&&";
							if(space){
								stringToken = "&";
							}
							valid = false;
						}
						else if(cmd.at(k)==' '){
							space = true;
						}
					}
				}
				else if(cmd.at(j)==' '){
					space = true;
				}
			}
		}
	}
	return valid;
}

void parseMaster(char* cmdB){
	char *ptr;
	vector <char*> cmdC;
	if((hasText(cmdB, "\"") || hasText(cmdB, "(") || hasText(cmdB, ")")) && !hasText(cmdB, "&&") && !hasText(cmdB, "||")){
		if(hasText(cmdB, "&") || hasText(cmdB, "|")){
			execCmd(parseSpace(cmdB));
		}
		else{
			//invalid command
			cout << "rshell: " << cmdB  <<": command not found\n";
		}
	}
	else if(!hasText(cmdB, "&&") && !hasText(cmdB, "||")){
		//execute
		execCmd(parseSpace(cmdB));
	}
	else if(hasText(cmdB, "&&") && !hasText(cmdB, "||")){ //only has &&
		cAND(cmdC, cmdB, &ptr);
	}
	else if(!hasText(cmdB, "&&") && hasText(cmdB, "||")){ //only has ||
		cOR(cmdC, cmdB, &ptr);
	}
	else if(hasText(cmdB, "&&") && hasText(cmdB, "||")){ //has both && and ||
		char* cmdBX = new char[strlen(cmdB)]; //backup cmdB
		strcpy(cmdBX, cmdB);

		bool succeed;
		char *ptrA;
		vector<char*> cmdD;
		parseDelim(cmdC, cmdB, "||", &ptr);
		if(hasText(cmdC[0], "&&")){ //vector of && commands separated by ||
			succeed = false;
			//printcmd(cmdC);
			for(unsigned int i = 0; i<cmdC.size() && !succeed; i++){
				if(hasText(cmdC[i], "&&")){
					if(cAND(cmdD, cmdC[i], &ptrA)!=-1){ //connectorAnd succeeds
						succeed	= true;
					}
				}
				else if(!execCmd(parseSpace(cmdC[i]))){
					succeed = true;
				}
			}
			cmdD.clear();
		}
		else{ //vector of || commands separated by &&
			succeed = true;
			cmdC.clear();
			cmdB = cmdBX; //restore cmdB	
			parseDelim(cmdC, cmdB, "&&", &ptr);
			//printcmd(cmdC);
			for(unsigned int i = 0; i<cmdC.size() && succeed; i++){
				if(hasText(cmdC[i], "||")){
					if(cOR(cmdD, cmdC[i], &ptrA)==-1){
						succeed = false;
					}
				}
				else if(execCmd(parseSpace(cmdC[i]))==-1){
					succeed = false;
				}
				cmdD.clear();
			}
			delete[] cmdBX;
		}
	}
}

int main(){
	while(1){
		string cmd, cmd2, stringToken;

		char host[64];	//prompt
		gethostname(host,64);
		cout << getlogin() << '@' << host << "$ ";
		getline(cin, cmd);
		
		//filter comments
		for(unsigned int i = 0; i<cmd.size(); i++){
			if(cmd.at(i)!='#'){
				cmd2.push_back(cmd.at(i));
			}
			else{
				i=cmd.size();
			}
		}
		/*
		//check for invalid instances of && and ||
		if(!checkConnector(cmd2, stringToken)){
			cout << "rshell: syntax error near unexpected token \'" << stringToken << "\'" << endl;
			cmd2 = "";
		}
		*/	
		if(cmd2!="" && !onlySpace(cmd2)){ //if command is not empty
			//convert string to char*
			char *cmdA = new char[cmd2.size()+1];
			for(int i = 0; i<static_cast<int>(cmd2.size()); i++){
				cmdA[i] = cmd2.at(i);
				if(i+1==static_cast<int>(cmd.size())){
					cmdA[i+1] = '\0'; //null terminating
				}
			}

			char *cmdB = strtok(cmdA, ";"); //parsing ";"

			bool cmdBDone = false;
			while(!cmdBDone){
				parseMaster(cmdB);	
				cmdB = strtok(NULL, ";");
				if(cmdB==NULL){
					cmdBDone = true;
				}
			}
			delete[] cmdA;
			delete[] cmdB;
		}
	}
	return 0;
}
