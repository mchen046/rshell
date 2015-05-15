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

void findDestFile();

int execCmd(char** cmdD, bool dealloc, int (&fdChild)[2], bool pipe); //process spawning

int redir(char*cmdB){

	//parsing by pipes
	string cmd = static_cast<string>(cmdB);
	vector<string> cmdPiped;
	string seg;
	for(unsigned int i = 0; i<cmd.size(); i++){
		if(cmd[i]!='|'){
			seg+=cmd[i];
		}
		if(i>0 && ( ((cmd[i]!='<' && cmd[i]!='>') && (cmd[i+1]=='<' || cmd[i+1]=='>')) || ((cmd[i]=='<' || cmd[i]=='>') && (cmd[i+1]!='<' && cmd[i+1]!='>')))){
			seg+=' ';
		}
		if(cmd[i]=='|' || (i+1)==cmd.size()){
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
	exit(1);
	//saving stdin
	int savestdin = 0;
	if(-1 == (savestdin = dup(0))){ //saving stdin
		perror("dup");
		_exit(2);
	}

	//save stdout
	int savestdout;
	if(-1 == (savestdout = dup(1))){
		perror("dup");
	}

	//bool parentPipe = false;
	if(cmdExec.size()>1){ //if there are segments of pipe, create fdParent[2]
		int fdParent[2];
		if(-1 == pipe(fdParent)){
			perror("pipe");
		}
		if(-1 == dup2(fdParent[RD], 0)){
			perror("dup2");
		}
		if(-1 == dup2(fdParent[WR], 1)){
			perror("dup2");
		}
		//parentPipe = true;
	}

	//cerr << parentPipe;

	//executing by pipe segment
	for(unsigned int i = 0; i<cmdExec.size(); i++){
		//invalid >>>>>> <<<<<< >><<><><
		for(unsigned int j = 0; cmdExec[i][j]!=NULL; j++){
			if((hasText(cmdExec[i][j], "<") || hasText(cmdExec[i][j], ">")) && !(strcmp(cmdExec[i][j], ">")==0 || strcmp(cmdExec[i][j], ">>")==0 || strcmp(cmdExec[i][j], "<")==0)){
				cerr << "rshell: syntax error near unexpected token '" << cmdExec[i][j] << "'" << endl;
				exit(1);
			}
		}

		vector<char*> argvChild; //executable commands
		unsigned int pos = 0;
		for(; cmdExec[i][pos]!=NULL && strcmp(cmdExec[i][pos], "<")!=0 && strcmp(cmdExec[i][pos], ">")!=0 && strcmp(cmdExec[i][pos], ">>")!=0; pos++){
			argvChild.push_back(cmdExec[i][pos]);
		}
		unsigned int prevPos = pos;
		unsigned int firstAnglePos = pos;

		//convert executable commands from vector<char*> to char**
		char **argv = new char*[argvChild.size()+2];
		for(unsigned int j = 0; j<argvChild.size(); j++){
			argv[j] = argvChild[j];
			if((j+1)==argvChild.size()){
				argv[j+1] = NULL;
				argv[j+2] = NULL;
			}
		}
		
		//loop iterations of < > and/or >>
		unsigned int cnt = 0, numL = 0, numG = 0;
		vector<string> files;

		int fdChild[2];
		if(-1 == pipe(fdChild)){
			perror("pipe");
		}
		
		//only enable pipe if there is such a file that is preceded by > && succeeded by either < || NULL
		//find destination file
		bool childPipe = false, seenRed = false;;
		char* destFile;
		unsigned int anglePos;
		for(unsigned int j = 0; cmdExec[i][j]!=NULL; j++){
			if(strcmp(cmdExec[i][j], ">") == 0 || strcmp(cmdExec[i][j], ">>") == 0){
				childPipe = true;
				seenRed = true;
				anglePos = j;
			}
			else if(strcmp(cmdExec[i][j], "<") == 0 && seenRed){
				destFile = cmdExec[i][anglePos+1];
				seenRed = false;
			}
			else if(cmdExec[i][j+1] == NULL && seenRed){
				destFile = cmdExec[i][anglePos+1];
			}
		}
		
		if(childPipe){ //if childPipe true, enable the redirection pipe (childPipe)
			if(-1 == dup2(fdChild[WR], 1)){ //make stdout the write end of pipe fdChild
				perror("dup2");
			}
			if(-1 == dup2(fdChild[RD], 0)){ //make stdin the read end of the pipe fdChild
				perror("dup2");
			}
		}	

		//change stdout to fdParent if childPipe is disabled WORKLIST // no pipes, only i/o redirection
		if(!childPipe || cmdExec[i+1] == NULL); //or if last element of the pipe segment
		//switch to fdParent using dup2

		
		//finding size of pipe segment and storing in argc
		unsigned int argc = 0;
		for(; cmdExec[i][argc+1]!=NULL; argc++){}
		
		for(unsigned int j = pos + 1; cmdExec[i][j]!=NULL; j++){

			int fdGL;
			if(strcmp(cmdExec[i][j], "<")!=0 && strcmp(cmdExec[i][j], ">")!=0 && strcmp(cmdExec[i][j], ">>")!=0){
				//check if file is an existing file
				bool fileExists = true;
				
				if(-1 == (fdGL = open(cmdExec[i][j], O_RDONLY))){
					//perror("open");
					fileExists = false;
					//status = -1;
				}

				if(fileExists){ //if is an existing file
					if(-1 == close(fdGL)){
						perror("close");
						exit(1);
					}
					files.push_back(cmdExec[i][j]);
				
					if(strcmp(cmdExec[i][pos], ">") == 0 && files.size()==1){ //if prev >, trunc file
						if(-1 == (fdGL = open(cmdExec[i][j], O_WRONLY | O_TRUNC))){
							perror("open");
						}
						if(-1 == close(fdGL)){
							perror("close");
						}
					}
				}
				else if(!fileExists){ //file doesn't exit
					if(strcmp(cmdExec[i][pos], "<")==0 || ((strcmp(cmdExec[i][pos], ">")==0 || strcmp(cmdExec[i][pos], ">>")==0) && files.size()!=0)){
						cerr << argv[0] << ": " << cmdExec[i][j] << ": No such file or directory" << endl;
						//exit(1);
					}
					else if((strcmp(cmdExec[i][pos], ">")==0 || strcmp(cmdExec[i][pos], ">>")==0) && files.size()==0){ //add condition of parent pipe not enabled 
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
			if(!(strcmp(cmdExec[i][j], "<")!=0 && strcmp(cmdExec[i][j], ">")!=0 && strcmp(cmdExec[i][j], ">>")!=0) || cmdExec[i][j+1]==NULL){

				prevPos = pos;
				pos = j;
				if(strcmp(cmdExec[i][prevPos], "<") == 0){
					numL++;
					if(strcmp(argv[0], "cat")==0 && files.size()==1 && (((numL==1 && numG==0 && argc-firstAnglePos==3) || cmdExec[i][j+1]==NULL) || (argv[1]!=NULL))){
						if(argv[1]==NULL && cnt == 0){
							cnt++;
							char* token = new char[strlen(const_cast<char*>(files[0].c_str()))];
							strcpy(token, files[0].c_str());
							argv[argvChild.size()] = token;
							execCmd(argv, false, fdChild, childPipe);
						}
						else if(argv[1]!=NULL){
							if(-1 == execCmd(argv, false, fdChild, childPipe)){
								return -1;
							}
						}

						while(argvChild.size()!=1 && argvChild[argvChild.size()-1][0]!='-'){
							argvChild.pop_back();
						}
						//clear argv
						for(unsigned int a = 1; argv[a]!=NULL && argv[a][0]!='-'; a++){
							argv[a]=NULL;
						}
					}
					else if(files.size()==1 && cnt==0 && strcmp(cmdExec[i][argc-2], "<")!=0 && strcmp(cmdExec[i][argc-2], ">")!=0 && strcmp(cmdExec[i][argc-2], ">>")!=0){
						if(-1 == execCmd(argv, false, fdChild, childPipe)){
							return -1;
						}
					}
					else if(files.size()>1){
						for(unsigned int k = 1; k < files.size(); k++){
							cnt++;
							char* token = new char[strlen(const_cast<char*>(files[k].c_str()))];
							strcpy(token, files[k].c_str());
							argv[argvChild.size()]=token;	
							//cerr << "calling execCmd on " << files[k] << endl;
							if(execCmd(argv, false, fdChild, childPipe)==-1){
								return -1;
							}
							while(argvChild.size()!=1 && argvChild[argvChild.size()-1][0]!='-'){
								argvChild.pop_back();
							}
							//clear argv
							for(unsigned int a = 1; argv[a]!=NULL && argv[a][0]!='-'; a++){
								argv[a]=NULL;
							}
						}
					}
				}
				if(strcmp(cmdExec[i][prevPos], ">") == 0 || strcmp(cmdExec[i][prevPos], ">>") == 0){
					numG++;
					if(files.size()==1 && cmdExec[i][j+1]==NULL && strcmp(cmdExec[i][j], destFile)!=0){ //last file in pipe seg, and not destFile
						cnt++;
						if(-1 == execCmd(argv, false, fdChild, childPipe)){
							return -1;
						}
					}

					else if(files.size()==1 && seenRed && strcmp(cmdExec[i][j],destFile)==0 && numL==0 && argv[1]==NULL && cnt==0 && strcmp(argv[0], "cat")==0){ //first and last element of pipe seg
						string userInput;
						if(-1 == dup2(savestdin, 0)){ //restore stdin
							perror("dup2");
						}
						if(-1 == dup2(savestdout, 1)){//restore stdout
							perror("dup2");
						}

						//deallocate any memory here
						
						while(1){	
							getline(cin, userInput);
							userInput += '\n';
							//write to the file here
							if(-1 == (fdGL = open(destFile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR))){
								perror("open");
							}
							if(-1 == write(fdGL, const_cast<char*>(userInput.c_str()), userInput.size())){
								perror("write");
							}
						}
						//mimicking a ctrl c exit similar to bash's behavior
					}

					else if(argv[1]!=NULL){
						if(-1 == execCmd(argv, false, fdChild, childPipe)){
							return -1;
						}
					}
					else if(files.size()==1 && cnt==0 && strcmp(cmdExec[i][argc-2], "<")!=0 && strcmp(cmdExec[i][argc-2], ">")!=0 && strcmp(cmdExec[i][argc-2], ">>")!=0){
						if(-1 == execCmd(argv, false, fdChild, childPipe)){
							return -1;
						}
					}
					
					for(unsigned int k = 1; k < files.size(); k++){ //write to fdChild pipe
						if(strcmp(files[k].c_str(), destFile)!=0){
							cnt++;
							char* token = new char[strlen(const_cast<char*>(files[k].c_str()))];
							strcpy(token, files[k].c_str());
							argv[argvChild.size()]=token;	
							//cerr << "calling execCmd on " << files[k] << endl;
							if(execCmd(argv, false, fdChild, childPipe)==-1){
								return -1;
							}
							while(argvChild.size()!=1 && argvChild[argvChild.size()-1][0]!='-'){
								argvChild.pop_back();
							}
							//clear argv
							for(unsigned int a = 1; argv[a]!=NULL && argv[a][0]!='-'; a++){
								argv[a]=NULL;
							}
						}
					}
				}
				files.clear();
			}
			if(cmdExec[i][j+1]==NULL && childPipe){ //last phrase
				//look for destination file to be written to
				if((strcmp(cmdExec[i][prevPos], ">")==0 || strcmp(cmdExec[i][prevPos], "<")==0) && (-1 == (fdGL = open(destFile, O_WRONLY | O_TRUNC)))){	
					perror("open");
					exit(1);
				}
				else if (strcmp(cmdExec[i][prevPos], ">>")==0 && (-1 == (fdGL = open(destFile, O_WRONLY | O_APPEND)))){
					perror("open");
					exit(1);
				}

				//source file = fdChild[RD]
				int size = 0;
				char c[BUFSIZ];
				if(-1 == (size = read(fdChild[RD], &c, sizeof(c)))){
					perror("read");
					exit(1);
				}
				if(-1 == write(fdGL, &c, size)){
					perror("write");
					exit(1);
				}
			}
		}
		if(-1 == dup2(savestdin, 0)){ //restore stdin
			perror("dup2");
		}
		if(-1 == dup2(savestdout, 1)){//restore stdout
			perror("dup2");
		}
		//deallocate char** argv
		for(unsigned int j = 0; argv[j]!=NULL; j++){
			delete[] argv[j];
		}
		delete[] argv;
	}
	//deallocate vector<char**> cmdExec
	
	return 1;
}
									
int execCmd(char** cmdD, bool dealloc, int (&fdChild)[2], bool pipe){ //process spawning
	if(pipe && (-1 == dup2(fdChild[WR], 1))){ //make stdout the write end of pipe fdChild
		perror("dup2");
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
	
	
	int pid = fork();
	if(pid == -1){ //error
		perror("fork() error");
		_exit(2);
	}
	else if(pid == 0){ //child process
		//cerr << "in child" << endl;
		//checking for nonexistent file; failing loud and clear
		bool nonEx = false;
		for(unsigned int i = 1; cmdD[i]!=NULL && !nonEx && cmdD[i][0]!='-'; i++){
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
		//cerr << "in parent" << endl;
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
	if((hasText(cmdB, "<") || hasText(cmdB, ">") || hasText(cmdB, "|")) || ((hasText(cmdB, "\"") || hasText(cmdB, "(") || hasText(cmdB, ")")) && !hasText(cmdB, "&&") && !hasText(cmdB, "||"))){
		if(hasText(cmdB, "&") || hasText(cmdB, "|") || hasText(cmdB, "<") || hasText(cmdB, ">")){
			redir(cmdB);
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
	//close(2);
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
