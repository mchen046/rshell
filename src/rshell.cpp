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
#include <fstream>
#include <signal.h>

#define RESET "\033[0m" //reset
#define BOLDBLACK "\033[1m\033[30m"
#define BOLDGREEN "\033[1m\033[32m"
#define BOLDBLUE "\033[1m\033[34m"
#define GREEN "\033[32m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"

using namespace std;
using namespace boost;

bool exitSeen = false, ackInt = false;
const int RD = 0;
const int WR = 1;
int fdNULL[2];
int savestdin;
int savestdout;

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

int execCmd(char** argv, int (&fdCur)[2], int (&fdPrev)[2], bool dealloc); //process spawning

char* findDestFile(char **argv, unsigned int &anglePos, bool &seenRed){ 
	char* destFile = NULL;
	for(unsigned int i = 0; argv[i]!=NULL; i++){
		if(strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0){
			seenRed = true;
			anglePos = i;
		}
		else if(strcmp(argv[i], "<") == 0 && seenRed){
			destFile = argv[anglePos+1];
			seenRed = false;
		}
		else if(argv[i+1] == NULL && seenRed){
			destFile = argv[anglePos+1];
		}
	}
	return destFile;
}

int io(char** argv, int (&fdCur)[2], int(&fdPrev)[2]){
	//invalid >>>>>> <<<<<< >><<><><
	for(unsigned int i = 0; argv[i]!=NULL; i++){
		if((hasText(argv[i], "<") || hasText(argv[i], ">")) && !(strcmp(argv[i], ">")==0 || strcmp(argv[i], ">>")==0 || strcmp(argv[i], "<")==0)){
			cerr << "rshell: syntax error near unexpected token '" << argv[i] << "'" << endl;
			exit(1);
		}
	}

	vector<char*> argvChild; //single executable command
	unsigned int pos = 0;
	for(; argv[pos]!=NULL && strcmp(argv[pos], "<")!=0 && strcmp(argv[pos], ">")!=0 && strcmp(argv[pos], ">>")!=0; pos++){
		argvChild.push_back(argv[pos]);
	}
	unsigned int prevPos = pos;
	unsigned int firstAnglePos = pos;
	unsigned int anglePos = 0;

	//convert executable commands from vector<char*> to char**
	unsigned int argc = 0;
	char **argvExec = new char*[argvChild.size()+2];
	for(unsigned  int i = 0; i<argvChild.size(); i++){
		argvExec[i] = argvChild[i];
		if((i+1)==argvChild.size()){
			argvExec[i+1] = NULL;
			argvExec[i+2] = NULL;
		}
		argc++;
	}

	//loop iterations of < > and/or >>
	unsigned int cnt = 0, numL = 0, numG = 0;
	vector<string> files;

	//only enable pipe if there is such a file that is preceded by > && succeeded by either < || NULL
	//find destination file
	bool seenRed = false;
	char *destFile = findDestFile(argv, anglePos, seenRed);

	if(destFile==NULL){
		//cerr << __LINE__ << endl;
		if(fdCur==fdNULL && fdPrev==fdNULL){
			//cerr << __LINE__ << endl;
			if(-1 == dup2(savestdout, 1)){//restore stdout
				perror("dup2");
			}
		}
	}

	//count number of < and > in command
	unsigned cntG = 0;
	unsigned cntL = 0;
	for(unsigned int i = 0; argv[i]!=NULL; i++){
		if(strcmp(argv[i], "<")==0){
			cntL++;
		}
		else if(strcmp(argv[i], ">")==0){
			cntG++;
		}
	}

	bool nxtPipeSeg = false;
	for(unsigned int i = pos + 1; argv[i]!=NULL && !nxtPipeSeg; i++){
		int fdGL;
		if(strcmp(argv[i], "<")!=0 && strcmp(argv[i], ">")!=0 && strcmp(argv[i], ">>")!=0){ //creating list of files
			//check if file is an existing file
			bool fileExists = true;
			
			if(-1 == (fdGL = open(argv[i], O_RDONLY))){
				perror("open");
				fileExists = false;
				//status = -1;
			}

			if(fileExists){ //if is an existing file
				if(-1 == close(fdGL)){
					perror("close");
					exit(1);
				}
				files.push_back(argv[i]);
			
				if(strcmp(argv[pos], ">") == 0 && files.size()==1){ //if prev >, trunc file
					if(-1 == (fdGL = open(argv[i], O_WRONLY | O_TRUNC))){
						perror("open");
					}
					if(-1 == close(fdGL)){
						perror("close");
					}
				}
			}
			else if(!fileExists){ //file doesn't exit
				/*if(strcmp(argv[pos], "<")==0 || ((strcmp(argv[pos], ">")==0 || strcmp(argv[pos], ">>")==0) && files.size()!=0)){
					cerr << argv[0] << ": " << argv[i] << ": No such file or directory" << endl;
					//exit(1);
				}*/
				if((strcmp(argv[pos], ">")==0 || strcmp(argv[pos], ">>")==0) && files.size()==0){ //add condition of parent pipe not enabled 
					//create new file
					if(-1 == (fdGL = open(argv[i], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR))){
						perror("open");
					}
					else if(-1 == close(fdGL)){
						perror("close");
					}
				}
				files.push_back(argv[i]);
			}
		}
		if(!(strcmp(argv[i], "<")!=0 && strcmp(argv[i], ">")!=0 && strcmp(argv[i], ">>")!=0) || argv[i+1]==NULL){
			prevPos = pos;
			pos = i;
			if(strcmp(argv[prevPos], "<") == 0){
				numL++;
				if(strcmp(argv[0], "cat")==0 && files.size()==1 && (((numL==1 && numG==0 && argc-firstAnglePos==3) || argv[i+1]==NULL) || (argvExec[1]!=NULL))){
					if(argvExec[1]==NULL && cnt == 0){
						cnt++;
						char* token = new char[strlen(const_cast<char*>(files[0].c_str()))];
						strcpy(token, files[0].c_str());
						argvExec[argvChild.size()] = token;
						execCmd(argvExec, fdCur, fdPrev, false);
					}
					else if(argvExec[1]!=NULL){
						cnt++;
						if(-1 == execCmd(argvExec, fdCur, fdPrev, false)){
							return -1;
						}
					}

					while(argvChild.size()!=1 && argvChild[argvChild.size()-1][0]!='-'){
						argvChild.pop_back();
					}
					//clear argv
					for(unsigned int k = 1; argvExec[k]!=NULL; k++){
						if(argvExec[k][0]!='-'){
							delete[] argvExec[k];
							argvExec[k]=NULL;
						}
					}
				}
				else if(files.size()==1 && cnt==0 && strcmp(argv[0], "cat")==0 && numL==cntL){
					cnt++;
					char* token = new char[strlen(const_cast<char*>(files[0].c_str()))];
					strcpy(token, files[0].c_str());
					argvExec[argvChild.size()] = token;
					if(-1 == execCmd(argvExec, fdCur, fdPrev, false)){
						return -1;
					}
				}
				/*else if(files.size()==1 && cnt==0 && strcmp(argv[argc-2], "<")!=0 && strcmp(argv[argc-2], ">")!=0 && strcmp(argv[argc-2], ">>")!=0){
					if(-1 == execCmd(argvExec, fdCur, fdPrev, false)){
						return -1;
					}
				}*/
				else if(files.size()>1){
					for(unsigned int j = 1; j < files.size(); j++){
						cnt++;
						char* token = new char[strlen(const_cast<char*>(files[j].c_str()))];
						strcpy(token, files[j].c_str());
						argvExec[argvChild.size()]=token;	
						//cerr << "calling execCmd on " << files[j] << endl;
						if(execCmd(argvExec, fdCur, fdPrev, false)){
							return -1;
						}
						while(argvChild.size()!=1 && argvChild[argvChild.size()-1][0]!='-'){
							argvChild.pop_back();
						}
						//clear argv
						for(unsigned int k = 1; argvExec[k]!=NULL; k++){
							if(argvExec[k][0]!='-'){
								delete[] argvExec[k];
								argvExec[k]=NULL;
							}
						}
					}
				}
			}
			if(strcmp(argv[prevPos], ">") == 0 || strcmp(argv[prevPos], ">>") == 0){
				numG++;
				if(files.size()==1 && argv[i+1]==NULL && strcmp(argv[i], destFile)!=0){ //last file in pipe seg, and not destFile
					cnt++;
					if(-1 == execCmd(argvExec, fdCur, fdPrev, false)){
						return -1;
					}
				}

				else if(files.size()==1 && seenRed && strcmp(argv[i],destFile)==0 && numL==0 && argvExec[1]==NULL && cnt==0 && strcmp(argv[0], "cat")==0){ //first and last element of pipe seg
					string userInput;
					if(-1 == dup2(savestdin, 0)){ //restore stdin
						perror("dup2");
					}
					if(-1 == dup2(savestdout, 1)){//restore stdout
						perror("dup2");
					}

					//deallocate any memory here
					
					while(!ackInt){	
						cin.clear();
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
									
				else if(argvExec[1]!=NULL && files.size()==1 && cnt==0){
					cnt++;
					if(-1 == execCmd(argvExec, fdCur, fdPrev, false)){
						return -1;
					}
				}
				
				for(unsigned int j = 1; j < files.size(); j++){ //write to fdChild pipe
					if(strcmp(files[j].c_str(), destFile)!=0){
						cnt++;
						char* token = new char[strlen(const_cast<char*>(files[j].c_str()))];
						strcpy(token, files[j].c_str());
						argvExec[argvChild.size()]=token;	
						//cerr << "calling execCmd on " << files[j] << endl;
						if(execCmd(argvExec, fdCur, fdPrev, false)==-1 && (j+1)==files.size()){
							return -1;
						}
						while(argvChild.size()!=1 && argvChild[argvChild.size()-1][0]!='-'){
							argvChild.pop_back();
						}
						//clear argv
						for(unsigned int k = 1; argvExec[k]!=NULL; k++){
							if(argvExec[k][0]!='-'){
								delete[] argvExec[k];
								argvExec[k]=NULL;
							}
						}
					}
				}
			}
			files.clear();
		}
	
		if(argv[i+1]==NULL){ //last phrase
			//look for destination file to be written to
			if(destFile!=NULL && cnt!=0 && fdCur!=fdNULL){
				if((strcmp(argv[prevPos], ">")==0 || strcmp(argv[prevPos], "<")==0) && (-1 == (fdGL = open(destFile, O_WRONLY | O_TRUNC)))){	
					perror("open");
					exit(1);
				}
				else if (strcmp(argv[prevPos], ">>")==0 && (-1 == (fdGL = open(destFile, O_WRONLY | O_APPEND)))){
					perror("open");
					exit(1);
				}

				//source file
				int size = 0;
				char c[BUFSIZ];
				if(-1 == (size = read(fdCur[RD], &c, sizeof(c)))){
					//cerr << __LINE__ << endl;
					perror("read");
					exit(1);
				}
				if(-1 == write(fdGL, &c, size)){
					perror("write");
					exit(1);
				}
			}
		}
	}
	//deallocate char** argvExec
	for(unsigned int i = 0; argvExec[i]!=NULL; i++){
		delete[] argvExec[i];
	}
	delete[] argvExec;
		
	return 1;
	//deallocate vector<char**> cmdExec
}

void redirExec(vector<char**> argvMaster){
	vector<int> pids;

	unsigned int nPS = argvMaster.size();
	struct pStruct{ int fd[2]; };
	vector<pStruct> pipes;
	for(unsigned int i = 0; i<nPS; i++){
		int fd[2];
		pStruct fdObj;
		if(-1 == pipe(fd)){
			perror("pipe");
		}
		for(unsigned int i = 0; i<2; i++){
			fdObj.fd[i] = fd[i];
		}
		pipes.push_back(fdObj);
	}
	
	if(pipes.size()==0){
		int fd[2];
		pStruct fdObj;
		if(-1 == pipe(fd)){
			perror("pipe");
		}
		for(unsigned int i = 0; i<2; i++){
			fdObj.fd[i] = fd[i];
		}
		pipes.push_back(fdObj);
	}
		
	bool noRD = false;
	if(argvMaster.size()==1){
		unsigned int a;
		bool cond;
		if(NULL == findDestFile(argvMaster[0], a, cond)){
			pipes[0].fd[0] = 0;
			pipes[0].fd[1] = 0;
		}
		io(argvMaster[0], pipes[0].fd, fdNULL);
	}
	else for(unsigned int i = 0; i<nPS; i++){

		unsigned int a;
		
		if(i > 0 && NULL != findDestFile(argvMaster[i-1], a, noRD)){ //if destFile is not NULL && not last element
			//cerr << __LINE__ << endl;
			pipes[i-1].fd[RD] = fdNULL[RD];
			pipes[i-1].fd[WR] = fdNULL[WR];
		}

		int status = 0;
		int pid = fork();
		pids.push_back(pid);
		bool next = false;
		if(-1 == pid){
			perror("fork");
			exit(1);
		}
		else if(pid == 0){ //child
			//check if i/o redirection exists
			bool redir = false;
			for(unsigned int j = 0; argvMaster[i][j]!=NULL && !redir; j++){
				if(hasText(argvMaster[i][j], "<") || hasText(argvMaster[i][j], ">")){
					redir = true;
				}
			}

			if(i==0 && argvMaster.size()>1){
				//cerr << __LINE__ << endl;
				if(-1 == dup2(pipes[i].fd[WR], 1)){
					perror("dup2");
					//cerr << __LINE__ << endl;
				}
				if(-1 == close(pipes[i].fd[RD])){
					perror("close");
					//cerr << __LINE__ << endl;
				}
			}
			else if(i>0 && (i+1)<nPS){
				//cerr << __LINE__ << endl;
				if(-1 == dup2(pipes[i].fd[WR], 1)){
					perror("dup2");
					//cerr << __LINE__ << endl;
				}
				if(-1 == close(pipes[i].fd[RD])){
					perror("close");
					//cerr << __LINE__ << endl;
				}
				if(-1 == dup2(pipes[i-1].fd[RD], 0)){
					perror("dup2");
					//cerr << __LINE__ << endl;
				}
				if(-1 == close(pipes[i-1].fd[WR])){
					perror("close");
					//cerr << __LINE__ << endl;
				}
			}
			else if(!redir && i>0 && (i+1)>=nPS){
				//restore stdout
				//cerr << __LINE__ << endl;
				if(-1 == dup2(savestdout, 1)){//restore stdout
					perror("dup2");
					//cerr << __LINE__ << endl;
				}
				if(-1 == dup2(pipes[i-1].fd[RD], 0)){
					perror("dup2");
					//cerr << __LINE__ << endl;
				}
				if(-1 == close(pipes[i-1].fd[WR])){
					perror("close");
					//cerr << __LINE__ << endl;
				}
			}


			if(redir){
				if(i==0){
					//cerr << __LINE__ << endl;
					io(argvMaster[i], pipes[i].fd, fdNULL);
				}
				else if(i>0 && (i+1)<nPS){
					//cerr << __LINE__ << endl;
					io(argvMaster[i], pipes[i].fd, pipes[i-1].fd);
				}
				else if((i+1)>=nPS){
					io(argvMaster[i], pipes[i].fd, pipes[i-1].fd); 
				}
			}
				
			else if(!redir && -1 == execvp(argvMaster[i][0], argvMaster[i])){
				perror("execvp");
				_exit(2);
			}
			_exit(2);
		}
		else if(pid > 0){ //parent
			if(i!=0){
				if(-1 == close(pipes[i-1].fd[RD])){
					perror("close");
				}
				if(-1 == close(pipes[i-1].fd[WR])){
					perror("close");
				}
			}
			if((i+1)<nPS){ //continue forking
				next = true;
			}
			else if((i+1)>=nPS && !next){ //last pipe segment
				for(unsigned int j = 0; j<pids.size(); j++){
					if(-1 == waitpid(pids[j], &status, WUNTRACED)){
						perror("wait");
					}
				}
			}
		}
		//}
	}
}

vector<char**> parsePipes(char* cmdB){
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
	/*cerr << endl << "cmdPiped: " << endl;
	for(unsigned int i = 0; i<cmdPiped.size(); i++){
		cerr << '[' << i << "]: " << cmdPiped[i] << endl;
	}
	cerr << endl;*/

	vector<char**> argvMaster;
	for(unsigned int i = 0; i<cmdPiped.size(); i++){
		argvMaster.push_back(parseSpace(cmdPiped[i].c_str()));
	}

	return argvMaster;
}

int redir(char*cmdB){
	vector<char**> argvMaster = parsePipes(cmdB);
	
	redirExec(argvMaster);

	/*for(unsigned int i = 0; i<argvMaster.size(); i++){
		for(unsigned int j = 0; argvMaster[i][j]!=NULL; j++){
			delete[] argvMaster[i][j];
		}
		delete argvMaster[i];
	}
	argvMaster.clear();*/

	if(-1 == dup2(savestdin, 0)){ //restore stdin
		perror("dup2");
	}
	if(-1 == dup2(savestdout, 1)){ //restore stdout
		perror("dup2");
	}

	return 1;
}

string findPWD(){
	int fd[2];
	if(-1 == pipe(fd)){
		perror("fd");
	}
	string pwd = "pwd";
	char** argv = new char*[2];
	argv[0] = const_cast<char*>(pwd.c_str());
	argv[1] = NULL;
	if(-1 == execCmd(argv, fd, fdNULL, false)){
		cerr << "findPWD failed!" << endl;
	}

	//create and read from random file using basic i/o
	int fdfile;
	char file[7] = {'X', 'X', 'X', 'X', 'X', 'X', 0};
	if(-1 == (fdfile = mkstemp(file))){
		perror("mkstemp");
	}
	int size = 0;
	char c[BUFSIZ];
	if(-1 == (size = read(fd[RD], &c, sizeof(c)))){
		perror("read");
	}
	if(-1 == write(fdfile, &c, size)){
		perror("write");
	}

	ifstream inFile(file);
	if(!inFile){
		cerr << file << " could not be opened!" << endl;
		exit(1);
	}

	string workDir;
	inFile >> workDir;

	//deallocate argv
	delete[] argv;

	//remove unique file
	char** argv2 = new char*[3];	
	string rm = "rm";
	argv2[0] = const_cast<char*>(rm.c_str()); 
	argv2[1] = file;
	argv2[2] = NULL;
	if(-1 == execCmd(argv2, fdNULL, fdNULL, false)){
		cerr << file << " could not be removed!" << endl;
		exit(1);
	}
	
	//deallocate argv
	delete[] argv2;

	if(-1 == dup2(savestdout, 1)){ //restore stdout
		perror("dup2");
	}

	return workDir;
}
void execCD(char** argv){
	string pwd = findPWD();

	char* home = getenv("HOME");
	bool alt = false;
	bool failed = false;
	
	if(argv[1]==NULL){
		alt = true;	
	}

	if(!alt && strcmp(argv[1], "-")==0 && -1 == chdir(getenv("OLDPWD"))){
		perror("chdir");
		failed = true;
	}

	if(!alt && strcmp(argv[1], "-")!=0 && -1 == chdir(argv[1])){
		perror("chdir");
		failed = true;
	}

	if(alt && -1 == chdir(home)){
		perror("chdir");
		failed = true;
	}

	//set OLDPWD
	if(!failed && -1 == setenv("OLDPWD", const_cast<char*>(pwd.c_str()), 1)){
		perror("setenv");
	}

	//print OLDPWD
	//cerr << "OLDPWD: " << getenv("OLDPWD") << endl;

	if(!failed && -1 == setenv("PWD", const_cast<char*>(findPWD().c_str()), 1)){
		perror("setenv");
	}
	
	//print PWD
	//cerr << "PWD: " << getenv("PWD") << endl;
}


	
int execCmd(char** argv, int (&fdCur)[2], int (&fdPrev)[2], bool dealloc){ //process spawning
	if(fdCur!=fdNULL && fdPrev==fdNULL){ //first element
		//cerr << __LINE__ << endl;	
		if(-1 == dup2(fdCur[WR], 1)){
			perror("dup2");
			//cerr << __LINE__ << endl;
		}
	}
	else if(fdCur!=fdNULL && fdPrev!=fdNULL){ //element between first and last
		if(-1 == dup2(fdCur[WR], 1)){
			perror("dup2");
			//cerr << __LINE__ << endl;
		}
		if(-1 == dup2(fdPrev[RD], 0)){
			perror("dup2");
			//cerr << __LINE__ << endl;
		}
		if(-1 == close(fdPrev[WR])){
			perror("close");
			cerr << __LINE__ << endl;
		}
	}
	else if(fdCur==fdNULL && fdPrev!=fdNULL){ //last element
		cerr << __LINE__ << endl;
		if(-1 == dup2(fdPrev[RD], 0)){
			perror("dup2");
			//cerr << __LINE__ << endl;
		}
		if(-1 == close(fdPrev[WR])){
			perror("close");
			//cerr << __LINE__ << endl;
		}
	}

	int status = 0;
	if(exitSeen){
		//deallocate cmdD
		for(int i = 0; argv[i]!=NULL; i++){
			delete[] argv[i];
		}
		delete[] argv;		
		return -1;
	}
	exitSeen = false;
	
	//checking if command is cd
	if(strcmp(argv[0], "cd")==0){
		execCD(argv);
	}	
	
	else{
		int pid = fork();
		if(pid == -1){ //error
			perror("fork() error");
			_exit(2);
		}
		else if(pid == 0){ //child process
			//cerr << "in child" << endl;
			if(execvp(argv[0], argv) == -1){
				perror("error in execvp"); //status becomes 256/512
				_exit(2);
			}
			_exit(2);
		}
		else if(pid > 0){ //parent process
			//cerr << "in parent" << endl;
			if(wait(&status)==-1){		
				perror("wait");
				_exit(2);
			}
		}
		//cout << "status: " << status << endl;
		if(status!=0){ //command fails 
			//cout << "command fails!\n";
			return -1;
		}
	}

	//deallocate argv
	if(dealloc){
		for(int i = 0; argv[i]!=NULL; i++){
			delete[] argv[i];
		}
		delete[] argv;
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
		if(execCmd(parseSpace(cmdC[i]), fdNULL, fdNULL, false)==-1){ //command fails
			success = -1;
		}
	}
	return success;
}

int cOR(vector<char*> &cmdC, char *cmdB, char **ptr){ //parse and execute || command
	parseDelim(cmdC, cmdB, "||", &*ptr);
	int success = -1;
	for(unsigned int i = 0; i<cmdC.size() && success==-1; i++){
		if(!execCmd(parseSpace(cmdC[i]), fdNULL, fdNULL, false)){ //command succeeds
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
		execCmd(parseSpace(cmdB), fdNULL, fdNULL, false);
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
				else if(!execCmd(parseSpace(cmdC[i]), fdNULL, fdNULL, false)){
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
				else if(execCmd(parseSpace(cmdC[i]), fdNULL, fdNULL, true)==-1){
					succeed = false;
				}
				cmdD.clear();
			}
		}
		delete[] cmdBX;
	}
	cmdC.clear();
}

void prompt(){
	while(1){
		ackInt = false;
		cin.clear();
		string cmd, cmd2, stringToken;

		char host[64];	//prompt
		if(-1 == gethostname(host,64)){
			perror("gethostname");
		}
		char *login;
		if(NULL == (login = getlogin())){
			perror("getlogin");
		}

		//displays working directory
		string pwd = findPWD();
		string home = static_cast<string>(getenv("HOME"));
		if(hasText(const_cast<char*>(pwd.c_str()), home)){
			pwd.replace(0, home.size(), "~");
		}

		cout << GREEN << login << CYAN << '@' << GREEN << host << CYAN << ':'  << GREEN << pwd << CYAN << " $ " << RESET;
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
}

void sig(int signum){
	if(signum==SIGINT){
		cout << endl;
		ackInt = true;
		//prompt();
	}
}



int main(){
	//close(2);
	
	//saving stdin
	if(-1 == (savestdin = dup(0))){ //saving stdin
		perror("dup");
	}

	//save stdout
	if(-1 == (savestdout = dup(1))){
		perror("dup");
	}

	//set up sigaction here to call prompt()
	struct sigaction ctrlC;
	memset(&ctrlC, 0, sizeof(ctrlC));
	ctrlC.sa_handler = sig;
	sigaction(SIGINT, &ctrlC, NULL);
	prompt();
	return 0;
}
