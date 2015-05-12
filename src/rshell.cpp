/*
Michael Chen mchen046@ucr.edu
SID: 861108671
CS100 Spring 2015: hw0-rshell
https://www.github.com/mchen046/rshell/blob/master/src/rshell.cpp
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

char** parseSpace(const char *cmd){
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

int redirExec(char** argv, string file);

int execCmd(char** cmdD); //process spawning


int redir(char**cmdD){
	//const int PIPE_READ = 0;
	//const int PIPE_WRITE = 1;
	//int status = 0, status2 = 0;

	//convert char** to vector<string>
	vector<string> cmd;
	for(unsigned int i = 0; cmdD[i]!=NULL; i++){
		cmd.push_back(string(cmdD[i]));
		cerr << cmd[i] << endl;
	}

	//parsing by pipes
	vector<string> cmdPiped;
	string seg;
	for(unsigned int i = 0; i<cmd.size(); i++){
		if(cmd[i]!="|"){
			seg+=cmd[i] + ' ';
		}
		if(cmd[i]=="|" || (i+1) == cmd.size()){
			cmdPiped.push_back(seg);
			seg = "";
		}
	}

	//printing cmdPiped
	cout << endl << "cmdPiped: " << endl;
	for(unsigned int i = 0; i<cmdPiped.size(); i++){
		cout << '[' << i << "]: " << cmdPiped[i] << endl;
	}
	cout << endl;

	vector<char**> cmdExec;
	for(unsigned int i = 0; i<cmdPiped.size(); i++){
		cmdExec.push_back(parseSpace(cmdPiped[i].c_str()));
	}

	//printing cmdExec;
	for(unsigned int i = 0; i<cmdExec.size(); i++){
		for(unsigned int j = 0; cmdExec[i][j]!=NULL; j++){
			cout << cmdExec[i][j] << endl;
		}
	}

	//executing by pipe segment
	for(unsigned int i = 0; i<cmdExec.size(); i+=2){
		bool hasL = false;
		//bool hasG = false, has2L = false, has2G = false;
		for(unsigned int j = 0; cmdExec[i][j]!=NULL; j++){
			if(strcmp(cmdExec[i][j], "<") == 0){
				hasL = true;
			}
			if(strcmp(cmdExec[i][j], ">") == 0){
				//hasG = true;
			}
			if(strcmp(cmdExec[i][j], "<<") == 1){
				//has2L = true;
			}
			if(strcmp(cmdExec[i][j], ">>") == 0){
				//has2G = true;
			}		
		}

		if(hasL){
			vector<char*> argvChild; //executable commands
			unsigned int posL = 0;
			for(; cmdExec[i][posL]!=NULL && strcmp(cmdExec[i][posL], "<")!=0; posL++){
				argvChild.push_back(cmdExec[i][posL]);
			}

			//convert executable commands from vector<char*> to char**
			cerr << endl << "argv: " << endl;
			char **argv = new char*[argvChild.size()+2];
			for(unsigned int j = 0; j<argvChild.size(); j++){
				argv[j] = argvChild[j];
				cerr << argv[j] << endl;
				if((j+1)==argvChild.size()){
					argv[j+2] = NULL;
				}
			}

			//loop iterations of <
			vector<string> files; //list of files to cast input redirection
			cerr << endl << "files: " << endl;
			for(unsigned int j = posL+1; cmdExec[i][j]!=NULL; j++){
				bool alt = false;
				if(strcmp(cmdExec[i][j], "<")!=0){
					//check if file is an existing file
						

					//if is an existing file
						files.push_back(cmdExec[i][j]);
						cerr << cmdExec[i][j]<< endl;
						if(cmdExec[i][j+1]!=NULL){
							alt = true;
						}
					
					//else not an existing file
						//output error message
						//exit
				}
				
				if(files.size()!=1 && !alt){
					//call dup pipe except on the first file
					for(unsigned int k = 1; k<files.size(); k++){
						//call dup and pipe
						argv[argvChild.size()+1]=const_cast<char*>(files[k].c_str());
						cerr << "calling execCmd on " << files[k] << endl;
						execCmd(argv);
					}
					files.clear();
					cerr << endl << "files: " << endl;
				}
				else if(files.size()==1 && !alt){
					if(cmdExec[i][j+1]==NULL){
						//call dup pipe on the first and only file
						argv[argvChild.size()+1]=const_cast<char*>(files[0].c_str());
						cerr << "calling pip and dup on " << files[0] << endl;
						execCmd(argv);
					}
					files.clear();
					cerr << endl << "files: " << endl;
				}
			}
		}

				



				



















		/*int fd[2];
		if(pipe(fd) == -1){
			perror("pipe");
			_exit(2);
		}

		int pid = fork();
		if(pid == -1){
			perror("fork");
			_exit(2);
		}
		else if(pid == 0){ //first child process
			//writing to the pipe
			cerr << "in first child" << endl;
			if(-1 == dup2(fd[PIPE_WRITE], 1)){ //move stdout to write end of pipe
				perror("dup2");
				_exit(2);
			}
			if(-1 == close(fd[PIPE_READ])){ //close read end of pipe (not using it)
				perror("close");
				_exit(2);
			}
			//cerr << "executing " << cmdExec[i][0] << endl;
			if(-1 == execvp(cmdExec[i][0], cmdExec[i])){
				perror("execvp");
				_exit(2);
			}
		}
		else if(pid>0){ //parent process
			cerr << "in first parent" << endl;
			
			if(-1 == wait(&status)){ 
				perror("wait");
				_exit(2);
			}
		}
		if(i+1<cmdExec.size()){
			int pid2 = fork();
			if(pid2 == -1){
				perror("fork");
				_exit(2);
			}
			else if(pid2 == 0){ //second child process
				cerr << "in second child" << endl;
				

				if(-1 == dup2(fd[PIPE_READ], 0)){ //make stdin the read end of pipe
					perror("dup2");
					_exit(2);
				}
				if(-1 == close(fd[PIPE_WRITE])){ //close write end of the pipe (not using it)
					perror("close");
					_exit(2);
				}
				//cerr << "executing " << cmdExec[i+1][0] << endl;
				if(-1 == execvp(cmdExec[i+1][0], cmdExec[i+1])){
					perror("execvp");
					_exit(2);
				}
			}
			else if(pid2>0){ //second parent process
				cerr << "in second parent" << endl;
				
				int savestdin = 0;
				if(-1 == (savestdin==dup(0))){ //saving stdin
					perror("dup");
					_exit(2);
				}
				if(-1 == wait(&status2)){
					perror("wait");
					_exit(2);
				}
				//restore stdin
				if(-1 == dup2(savestdin, 0)){
					perror("dup2");
					_exit(2);
				}
			}
		}*/
		//else{

	}
	return 1;
}

int execCmd(char** cmdD){ //process spawning
	int status = 0;
	if(exitSeen){
		//deallocate cmdD
		for(int i = 0; cmdD[i]!=NULL; i++){
			delete[] cmdD[i];
		}
		delete[] cmdD;		
		return -1;
	}
	exitSeen = false;
	
	//checking to see if | or > exists
	bool redirect = false;
	for(unsigned int i = 0; cmdD[i]!=NULL && !redirect; i++){
		for(unsigned int j = 0; cmdD[i][j]!='\0' && !redirect; j++){
			if(cmdD[i][j]=='>' || cmdD[i][j]=='|' || cmdD[i][j]=='<'){
				redirect = true;
			}
		}
	}
	if(redirect && redir(cmdD)!=1){ // call redirection (hw2)
		status = -1;
	}
	else if(!redirect){
		int pid = fork();
		if(pid == -1){ //error
			perror("fork() error");
			_exit(2);
		}
		else if(pid == 0){ //child process
			cerr << "in child" << endl;
			if(execvp(cmdD[0], cmdD) == -1){
				perror("error in execvp"); //status becomes 256/512
				_exit(2);
			}
		}
		else if(pid > 0){ //parent process
			cerr << "in parent" << endl;
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
	}

	//deallocate cmdD
	for(int i = 0; cmdD[i]!=NULL; i++){
		delete[] cmdD[i];
	}
	delete[] cmdD;
	
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
	unsigned int space = 0, a = 0, o = 0, x = 0;
	for(unsigned int i = 0; i<cmd.size(); i++){
		if(cmd[i]==' '){
			space++;
		}
		else if(cmd[i]=='&'){
			space=x=0;
			a++;
		}
		else if(cmd[i]=='|'){
			space=x=0;
			o++;
		}
		else{
			x++;
			a=o=space=0;
		}

		if((((a>2 || o>2) || ((a==2 && o==1) || (a==1 && o==2))) && x==0) || (a==1 && o==1) || (a==1 && space!=0)){
			stringToken = cmd[i];
			return false;
		}

	}
	return true;
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
		}
		delete[] cmdBX;
	}
	cmdC.clear();
}

int main(){
	while(1){
		string cmd, cmd2, stringToken;

		char host[64];	//prompt
		if(-1 == gethostname(host,64)){
			perror("gethostname");
		}
		char *login;
		if(NULL == (login = getlogin())){
			perror("getlogin");
		}
		
		cout << login << '@' << host << "$ ";
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
		
		//check for invalid instances of && and ||
		if(!checkConnector(cmd2, stringToken)){
			cout << "rshell: syntax error near unexpected token \'" << stringToken << "\'" << endl;
			cmd2 = "";
		}
		
		if(cmd2!="" && !onlySpace(cmd2)){ //if command is not empty
			//convert string to char*
			char *cmdA = new char[cmd2.size()+1];
			for(int i = 0; i<static_cast<int>(cmd2.size()); i++){
				cmdA[i] = cmd2.at(i);
				if(i+1==static_cast<int>(cmd2.size())){
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
