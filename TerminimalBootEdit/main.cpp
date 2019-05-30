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
#include "FRulezConfigs.h"

#include "BootUtils.h"
#include "LowLvlPck.h"

char *maindir=NULL;
char *anyprefix=(char *)"WRORK_DIR_";

AUI *ulist=NULL;
AFPC *config=NULL;

void LoadConfigs(char *usersf,char *pathsf){
	char *dump=NULL;
	int coutn=0,count2=0;
	FILE *ids=NULL,*fpaths=NULL;
	SSIM *usgr=NULL;

	if((ids=fopen(usersf,"rb"))){
		fseek(ids,0,SEEK_END);
		int nax=ftell(ids);
		dump=new char[nax+1];
		dump[nax]=0;
    	fseek(ids,0,SEEK_SET);
    	fread(dump,1,nax,ids);
    	usgr=DoParse(dump);
    	ulist=ParseIDs(usgr,&coutn);
    	ParseClear(usgr);
    	delete dump;
		fclose(ids);
	}

	if(ulist&&(fpaths=fopen(pathsf,"rb"))){
		fseek(fpaths,0,SEEK_END);
		int nax=ftell(fpaths);
		dump=new char[nax+1];
		dump[nax]=0;
    	fseek(fpaths,0,SEEK_SET);
    	fread(dump,1,nax,fpaths);
		usgr=DoParse(dump);
		config=ParseFS(usgr,ulist,&count2);
		ParseClear(usgr);
		delete dump;
		fclose(fpaths);
	}
}

void FreeConfigs(){
	if(config)FreeAFPCs(config);
	if(ulist)FreeAIDs(ulist);
}

int main(int argc, char** argv)
{
	char WDR = 0,*targs=NULL;
	char *bootimgname = (char *) "boot.img",
		*recimgname = (char *) "recovery.img",
		*wdb=(char *)"WORK_DIR_BOOT",
		*wdr=(char *)"WORK_DIR_RECOVERY";

    char *curfileimg=NULL,*work_prefix=NULL,*w_cut=NULL;
    short mlen=0;


    FILE *fh=NULL;
	maindir = new char[strlen(argv[0])+1];
	strcpy(maindir, argv[0]);
	strrchr(maindir, TRUE_SLASH)[1]='\0';
	mlen=strlen(maindir);

	//--->
	char *usersf=new char[mlen+16];
	char *pathsf=new char[mlen+10];
	sprintf(usersf,"%s%s",maindir,"usersgroups.cfg");
	sprintf(pathsf,"%s%s",maindir,"paths.cfg");
	LoadConfigs(usersf,pathsf);
	delete usersf;
	delete pathsf;

	work_prefix=new char[mlen+strlen(wdb)+2];
	sprintf(work_prefix,"%s%s%c",maindir,wdb,TRUE_SLASH);
	curfileimg=new char[mlen+strlen(bootimgname)+1];
	sprintf(curfileimg,"%s%s",maindir,bootimgname);
	if(((fh=fopen(curfileimg,"rb"))!=NULL)||IsDir(work_prefix)){
	if(fh)fclose(fh);
	WDR += ReMakeANDCatalogs(work_prefix, curfileimg);
	}
	delete curfileimg;
	delete work_prefix;


	work_prefix=new char[mlen+strlen(wdr)+2];
	sprintf(work_prefix,"%s%s%c",maindir,wdr,TRUE_SLASH);
	curfileimg=new char[mlen+strlen(recimgname)+1];
	sprintf(curfileimg,"%s%s",maindir,recimgname);
	if(((fh=fopen(curfileimg,"rb"))!=NULL)||IsDir(work_prefix)){
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

	if(((fh=fopen(argv[argc-1],"rb"))!=NULL)||IsDir(work_prefix)){
	if(fh)fclose(fh);
	WDR += ReMakeANDCatalogs(work_prefix, argv[argc-1]);
	}
	delete work_prefix;
	delete targs;
	argc--;
	}

	FreeConfigs();
	//---<
	delete maindir;
	return 0;
}

