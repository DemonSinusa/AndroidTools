/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   main.cpp
 * Author: aveman
 *
 * Created on 11 марта 2016 г., 20:14
 */

#include <cstdlib>

using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <string.h>

#include <bootimg.h>

#include "BootUtils.h"
#include "LowLvlPck.h"

char *maindir=NULL;
char *anyprefix=(char *)"WRORK_DIR_";

int main(int argc, char** argv)
{
	char WDR = 0,*targs=NULL;
	char *bootimgname = (char *) "boot.img",
		*recimgname = (char *) "recovery.img";

    char *curfileimg=NULL,*work_prefix=NULL,*w_cut=NULL;
    short mlen=strlen(argv[0]);
    FILE *fh=NULL;
	maindir = new char[mlen];
	strcpy(maindir, argv[0]);
	strrchr(maindir, TRUE_SLASH)[1]='\0';
	//--->
	work_prefix=new char[mlen+15];
	sprintf(work_prefix,"%s%s%c",maindir,"WORK_DIR_BOOT",TRUE_SLASH);
	curfileimg=new char[mlen+strlen(bootimgname)+1];
	sprintf(curfileimg,"%s%s",maindir,bootimgname);
	if(((fh=fopen(curfileimg,"rb"))!=NULL)||(chdir(work_prefix)==0)){
	if(fh)fclose(fh);
	WDR += ReMakeANDCatalogs(work_prefix, curfileimg);
	}
	delete curfileimg;
	delete work_prefix;


	work_prefix=new char[mlen+19];
	sprintf(work_prefix,"%s%s%c",maindir,"WORK_DIR_RECOVERY",TRUE_SLASH);
	curfileimg=new char[mlen+strlen(recimgname)+1];
	sprintf(curfileimg,"%s%s",maindir,recimgname);
	if(((fh=fopen(curfileimg,"rb"))!=NULL)||(chdir(work_prefix)==0)){
	if(fh)fclose(fh);
	WDR += ReMakeANDCatalogs(work_prefix, curfileimg);
	}
	delete curfileimg;
	delete work_prefix;

	while(argc>1){

	targs=new char[strlen(argv[argc-1])+1];
	strcpy(targs,argv[argc-1]);
	strrchr(targs, TRUE_SLASH)[1]='\0';
	w_cut=(strrchr(argv[argc-1], TRUE_SLASH)+1);

	work_prefix=new char[strlen(targs)+strlen(anyprefix)+strlen(w_cut)+2];
	sprintf(work_prefix,"%s%s%s%c",targs,anyprefix,w_cut,TRUE_SLASH);

	if(((fh=fopen(argv[argc-1],"rb"))!=NULL)||(chdir(work_prefix)==0)){
	if(fh)fclose(fh);
	WDR += ReMakeANDCatalogs(work_prefix, argv[argc-1]);
	}
	delete work_prefix;
	delete targs;
	argc--;
	}

	//---<
	delete maindir;
	return 0;
}

