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
#include <fcntl.h>

using namespace std;
using namespace boost;

bool exitSeen = false;
const int RD = 0;
const int WR = 1;
int fdNULL[2];

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

int execCmd(char** cmdD, bool dealloc, int (&fdChild)[2], bool pipe); //process spawning

int redir(char**cmdD){
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
	cerr << endl << "cmdPiped: " << endl;
	for(unsigned int i = 0; i<cmdPiped.size(); i++){
		cerr << '[' << i << "]: " << cmdPiped[i] << endl;
	}
	cerr << endl;

	vector<char**> cmdExec;
	for(unsigned int i = 0; i<cmdPiped.size(); i++){
		cmdExec.push_back(parseSpace(cmdPiped[i].c_str()));
	}

	//printing cmdExec;
	for(unsigned int i = 0; i<cmdExec.size(); i++){
		for(unsigned int j = 0; cmdExec[i][j]!=NULL; j++){
			cerr << cmdExec[i][j] << endl;
		}
	}

	//saving stdin
	int savestdin = 0;
	if(-1 == (savestdin==dup(0))){ //saving stdin
		perror("dup");
		_exit(2);
	}

	//save stdout
	int savestdout;
	if(-1 == (savestdout = dup(1))){
		perror("dup");
	}

	//executing by pipe segment
	for(unsigned int i = 0; i<cmdExec.size(); i+=2){

		//bool hasL = false, hasG = false;
		//bool has2L = false, has2G = false;
		for(unsigned int j = 0; cmdExec[i][j]!=NULL; j++){
			if(strcmp(cmdExec[i][j], "<") == 0){
				//hasL = true;
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

		vector<char*> argvChild; //executable commands
		unsigned int pos = 0;
		unsigned int prevPos = pos;
		for(; cmdExec[i][pos]!=NULL && strcmp(cmdExec[i][pos], "<")!=0 && strcmp(cmdExec[i][pos], ">")!=0; pos++){
			argvChild.push_back(cmdExec[i][pos]);
		}

		//convert executable commands from vector<char*> to char**
		cerr << endl << "argv: " << endl;
		char **argv = new char*[argvChild.size()+2];
		for(unsigned int j = 0; j<argvChild.size(); j++){
			argv[j] = argvChild[j];
			cerr << argv[j] << endl;
			if((j+1)==argvChild.size()){
				argv[j+1] = NULL;
				argv[j+2] = NULL;
			}
		}
		
		//loop iterations of < and/or >
		unsigned cnt = 0;
		vector<string> files;

		int fdChild[2];
		if(-1 == pipe(fdChild)){
			perror("pipe");
		}
		

		if(-1 == dup2(fdChild[WR], 1)){ //make stdout the write end of pipe fdChild
			perror("dup2");
		}

		if(-1 == dup2(fdChild[RD], 0)){ //make stdin the read end of the pipe fdChild
			perror("dup2");
		}
		
	
			
		for(unsigned int j = pos + 1; cmdExec[i][j]!=NULL; j++){
			int fdGL;
			if(strcmp(cmdExec[i][j], "<")!=0 && strcmp(cmdExec[i][j], ">")!=0 && cmdExec[i][j]!=NULL){
				//check if file is an existing file
				bool fileExists = true;
				
				if(-1 == (fdGL = open(cmdExec[i][j], O_RDONLY))){
					perror("open");
					fileExists = false;
					//status = -1;
				}

				if(fileExists){ //if is an existing file
					if(-1 == close(fdGL)){
						perror("close");
						exit(1);
					}
					files.push_back(cmdExec[i][j]);
					cerr << cmdExec[i][j]<< endl;
				}
				else if(!fileExists){ //file doesn't exit
					if(strcmp(cmdExec[i][pos], "<")==0 || (strcmp(cmdExec[i][pos], ">")==0 && files.size()!=0)){
						cerr << argv[0] << ": " << cmdExec[i][j] << ": No such file or directory" << endl;
						exit(1);
					}
					else if(strcmp(cmdExec[i][pos], ">")==0 && files.size()==0){
						//create new file
						if(-1 == (fdGL = open(cmdExec[i][j], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR))){
							perror("open");
						}
						else if(-1 == close(fdGL)){
							perror("close");
						}
						files.push_back(cmdExec[i][j]);
					}
				}
			}
			if(!(strcmp(cmdExec[i][j], "<")!=0 && strcmp(cmdExec[i][j], ">")!=0 && cmdExec[i][j+1]!=NULL)){

				cnt++;
				prevPos = pos;
				pos = j;

				if(strcmp(cmdExec[i][prevPos], "<") == 0){
					if(files.size()==1 && strcmp(argv[0], "cat")==0){
						if(argv[1]==NULL && cnt == 1){
							char* token = new char[strlen(const_cast<char*>(files[0].c_str()))];
							strcpy(token, files[0].c_str());
							argv[argvChild.size()] = token;
						}
						execCmd(argv, false, fdChild, true); //add file descriptor parameter and pass it in by reference
						while(argvChild.size()!=1){
							argvChild.pop_back();
						}
						//clear argv
						for(unsigned int a = 1; argv[a]!=NULL; a++){
							argv[a]=NULL;
						}
					}
					else if(files.size()>1){
						for(unsigned int k = 1; k < files.size(); k++){
							char* token = new char[strlen(const_cast<char*>(files[k].c_str()))];
							strcpy(token, files[k].c_str());
							argv[argvChild.size()]=token;	
							cerr << "calling execCmd on " << files[k] << endl;
							if(execCmd(argv, false, fdChild, true)==-1){
								return -1;
							}
							while(argvChild.size()!=1){
								argvChild.pop_back();
							}
							//clear argv
							for(unsigned int a = 1; argv[a]!=NULL; a++){
								argv[a]=NULL;
							}
						}
					}
				}

				if(strcmp(cmdExec[i][prevPos], ">") == 0){
					
					for(unsigned int k = 0; k < files.size() && files.size()!=1; k++){ //write to fdChild pipe
						if(-1 == (fdGL = open(files[k].c_str(), O_RDONLY))){
							perror("open");
							exit(1);
						}
						int size;
						char c[BUFSIZ];
						if(-1 == (size = read(fdGL, c, sizeof(c)))){
							perror("read");
							exit(1);
						}
						if(-1 == write(fdChild[WR], c, size)){
							perror("write");
							exit(1);
						}
					}

					if(cmdExec[i][j+1]==NULL){ //last phrase
						if(-1 == (fdGL = open(files[0].c_str(), O_WRONLY | O_TRUNC))){ //look for destination file to be written to
							perror("open");
							exit(1);
						}
						//source file = fdChild[RD]
						int size = 0;
						char c[BUFSIZ];
						if(-1 == (size = read(fdChild[RD], c, sizeof(c)))){
							perror("read");
							exit(1);
						}
						if(-1 == write(fdGL, c, size)){
							perror("write");
							exit(1);
						}
					}
				}
				files.clear();
			}
		}
	}
	return 1;
}
									
/*
		int fd[2];
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

	//deallocating cmdExec
	/*for(unsigned int i = 0; i<cmdExec.size(); i++){
		for(unsigned int j = 0; cmdExec[i][j]!=NULL; j++){
			delete[] cmdExec[i][j];
		}
		delete[] cmdExec[i];
	}*/

int execCmd(char** cmdD, bool dealloc, int (&fdChild)[2], bool pipe){ //process spawning
	if(pipe && (-1 == dup2(fdChild[WR], 1))){ //make stdout the write end of pipe fdChild
		perror("dup");
	}

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
			//checking for nonexistent file; failing loud and clear
			bool nonEx = false;
			for(unsigned int i = 1; cmdD[i]!=NULL && !nonEx; i++){
				if(-1 == open(cmdD[i], O_RDONLY)){
					perror("open");
					cerr << cmdD[0] << ": " << cmdD[i] << ": No such file or directory" << endl;
					nonEx = true;
				}
			}

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
	if(dealloc){
		for(int i = 0; cmdD[i]!=NULL; i++){
			delete[] cmdD[i];
		}
		delete[] cmdD;
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
		if(execCmd(parseSpace(cmdC[i]), true, fdNULL, false)==-1){ //command fails
			success = -1;
		}
	}
	return success;
}

int cOR(vector<char*> &cmdC, char *cmdB, char **ptr){ //parse and execute || command
	parseDelim(cmdC, cmdB, "||", &*ptr);
	int success = -1;
	for(unsigned int i = 0; i<cmdC.size() && success==-1; i++){
		if(!execCmd(parseSpace(cmdC[i]), true, fdNULL, false)){ //command succeeds
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
			execCmd(parseSpace(cmdB), true, fdNULL, false);
		}
		else{
			//invalid command
			cout << "rshell: " << cmdB  <<": command not found\n";
		}
	}
	else if(!hasText(cmdB, "&&") && !hasText(cmdB, "||")){
		//execute
		execCmd(parseSpace(cmdB), true, fdNULL, false);
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
				else if(!execCmd(parseSpace(cmdC[i]), true, fdNULL, false)){
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
				else if(execCmd(parseSpace(cmdC[i]), true, fdNULL, false)==-1){
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
	close(2);
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
