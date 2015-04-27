#include <iostream>
#include <fstream>
#include <stdio.h>
#include "Timer.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

void streamcp(char* infile, char* outfile); 

void rdwr(char* infile, char* outfile);

void rdwrbuf(char* infile, char* outfile);

int main(int argc, char** argv) {
	if(argc < 3) {
		cout << "Error: Enter at least 2 files. " << endl;
	}
	else if(argc == 3) {
		rdwrbuf(argv[1], argv[2]);
	}
	else if(argc == 4) {
		double wt = 0, ut =0, st = 0;
		Timer wall, user, sys;
		
		
		sys.start();
		rdwrbuf(argv[1], argv[2]);
		if(-1 == sys.elapsedTime(wt,ut,st)) {
			cout << "Error with elapsedTime(). " << endl;
		}
		else {
			cout << "rdwrbuf: wt = " << wt << " ut = " << ut << " st = " << st << endl;
		}
		
		wall.start();
		streamcp(argv[1],argv[2]);
		if(-1 == wall.elapsedTime(wt,ut,st) ) {
			cout << "Error with elapsedTime(). " << endl;
		}
		else {
			cout << "stream: wt = " << wt << " ut = " << ut << " st = " << st << endl;
		}

		user.start();
		rdwr(argv[1], argv[2]);
		if(-1 == user.elapsedTime(wt,ut,st) ) { 
			cout << "Error with elapsedTime(). " << endl;
		}
		else {
			cout << "rdwr: wt = " << wt << " ut = " << ut << " st = " << st << endl;
		}


	}
	else {
		cout << "Error: too many arguements" << endl;
	}

	return 0;
}


void streamcp(char* infile, char* outfile) {
	ifstream ifs;
	ofstream ofs;
	ifs.open(infile);
	ofs.open(outfile);
	char c;
	while(ifs.good() && ifs.get(c)) {
		ofs.put(c);
	}
	ifs.close();
	ofs.close();
}

void rdwrbuf(char* infile, char* outfile) {
	int fdin;
	int fdout;
	if(-1 == (fdout = open(outfile, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR))) {
		perror("There was an error with open(). ");
		exit(1);
	}
	if(-1 == (fdin = open(infile, O_RDONLY))) {
		perror("There was an error with open(). ");
		exit(1);
	}
	char c[BUFSIZ];
	int size;
	if(-1 == (size = read(fdin, c, sizeof(c)))) {
		perror("There was an error with read().");
		exit(1);
	}
	while(size > 0) {
		if(-1 == write(fdout, c, size)) {
			perror("There was an error with write(). ");
			exit(1);
		}
		if(-1 == (size = read(fdin, c, sizeof(c)))) {
			perror("There was an error with read().");
			exit(1);
		}
	}
	if(-1 == close(fdout)) {
		perror("There was an error with close(). ");
		exit(1);
	}
	if(-1 == close(fdin)) {
		perror("There was an error with close(). ");
		exit(1);
	}
}

void rdwr(char* infile, char* outfile) {
	int fdin;
	int fdout;
	if(-1 == (fdout = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR))) {
		perror("There was an error with open(). ");
		exit(1);
	}
	if(-1 == (fdin = open(infile, O_RDONLY))) {
		perror("There was an error with open(). ");
		exit(1);
	}
	char c[1];
	int size;
	if(-1 == (size = read(fdin, c, sizeof(c)))) {
		perror("There was an error with read().");
		exit(1);
	}
	while(size > 0) {
		if(-1 == write(fdout, c, size)) {
			perror("There was an error with write(). ");
			exit(1);
		}
		if(-1 == (size = read(fdin, c, sizeof(c)))) {
			perror("There was an error with read().");
			exit(1);
		}
	}
	if(-1 == close(fdout)) {
		perror("There was an error with close(). ");
		exit(1);
	}
	if(-1 == close(fdin)) {
		perror("There was an error with close(). ");
		exit(1);
	}
}
