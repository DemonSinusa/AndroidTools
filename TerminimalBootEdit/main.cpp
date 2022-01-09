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
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <string.h>

#include <bootimg.h>
#include "FRulezConfigs.h"

#include "BootUtils.h"
#include "LowLvlPck.h"

#define MAX_BCKP_DEEP	24

char *maindir=NULL;
char *bckph=(char *)".BckPH";
char *anyprefix=(char *)"WRORK_DIR_";
char *bootimgname_def[] = {(char *) "boot.img",(char *) "recovery.img"};


AUI *ulist=NULL;
AFPC *config=NULL;
HRGA *huirdrsch=NULL;

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

//Проверяет наличие каталога и создает при отсутствии возвращает 0 при успехе
int CheckADmbig(char *dir){
	int st=0;
	if(!IsDir(dir)){
		if((st=mkdir(dir, 0775))){
			fprintf(stdout, "Шалаш по \"%s\"-не строится,расследование привело к:\r\n", dir);
			fprintf(stdout, "Невероятной ошибчке №%d(%s)\r\n",errno,strerror(errno));
			return errno;
		}
		}
	return 0;
}

void CreateBackUP(char *b_dir,HRGA *b_file){
	FILE *fh=NULL,*frfh=NULL;
	int blocklen=1024*32,rblen=0;
	char block[blocklen];
	int bdir_len=strlen(b_dir),b_len=0,R_fix_lengthty=1;
	char *def_px=(char *)"def_";
	char *b_name=NULL,*rb_name=NULL,*R_fix=NULL,*rwalos=NULL,*trueb_dir=b_dir;

		if(CheckADmbig(b_dir))return;

		b_name=&strrchr(b_file->str, TRUE_SLASH)[1];
		b_len=strlen(b_name);

		if(b_file->_isdef){
			trueb_dir=new char[bdir_len+strlen(def_px)+b_len+2];
			sprintf(trueb_dir,"%s%s%s%c",b_dir,def_px,b_name,TRUE_SLASH);
			bdir_len=strlen(trueb_dir);

			if(CheckADmbig(trueb_dir)){
				delete trueb_dir;
				return;
			}
		}else{
			trueb_dir=new char[bdir_len+1];
			strcpy(trueb_dir,b_dir);
		}

			if((frfh=fopen(b_file->str,"rb"))!=NULL){
				//Можно бэкап..

			do{
			R_fix=new char[R_fix_lengthty+1];

			for(int hy=0;hy<R_fix_lengthty;hy++)R_fix[hy]='_';
			R_fix[R_fix_lengthty]='\0';

			rb_name=new char[b_len+R_fix_lengthty+1];
			sprintf(rb_name,"%s%s",R_fix,b_name);
			rwalos=new char[bdir_len+b_len+R_fix_lengthty+1];
			sprintf(rwalos,"%s%s",trueb_dir,rb_name);

			if((fh=fopen(rwalos,"rb"))!=NULL){
					fclose(fh);
					R_fix_lengthty+=1;
					delete R_fix;
					delete rb_name;
					delete rwalos;
			}else{
				if((fh=fopen(rwalos,"wb"))!=NULL){
					while((rblen=fread(&block,1,blocklen,frfh))==blocklen){
						if(fwrite(block,blocklen,1,fh)!=1){
							rblen=0;
							break;
						}
					}
					if(rblen>0)fwrite(block,rblen,1,fh);
					fclose(fh);
					break;
				}else{
					//Проверить доступность на rw.
					fprintf(stdout, "0шибчкa №%d(%s)\r\n",errno,strerror(errno));
				}
			}

			}while(R_fix_lengthty<MAX_BCKP_DEEP);

				fclose(frfh);
				if(R_fix_lengthty!=MAX_BCKP_DEEP){
				delete R_fix;
				delete rb_name;
				delete rwalos;
				}
			}

			delete trueb_dir;
	}

int main(int argc, char** argv)
{
	char WDR = 0,*targs=NULL;
    char *work_prefix=NULL,*build_h_prefix=NULL,*w_cut=NULL;
    short mlen=0,deflen=0,irgcur=0;

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


	for(deflen=0;bootimgname_def[deflen];deflen++);
   	huirdrsch = new HRGA[deflen+(argc-1)];

	for(int p=0;bootimgname_def[p];p++){
		huirdrsch[irgcur]._isdef=1;
		huirdrsch[irgcur].len=mlen+strlen(bootimgname_def[p]);
		huirdrsch[irgcur].str=new char[huirdrsch[irgcur].len+1];
		sprintf(huirdrsch[irgcur].str,"%s%s",maindir,bootimgname_def[p]);
		irgcur++;
	}

	while(argc>1){
		huirdrsch[irgcur]._isdef=0;
		huirdrsch[irgcur].len=strlen(argv[argc-1]);
		huirdrsch[irgcur].str=new char[huirdrsch[irgcur].len+1];
		strcpy(huirdrsch[irgcur].str,argv[argc-1]);
		irgcur++;
		argc--;
	}


	long getrf=sizeof(*huirdrsch)/sizeof(huirdrsch);
	for(int check=0;check<getrf;check++){
		targs=new char[huirdrsch[check].len];
		strcpy(targs,huirdrsch[check].str);
		strrchr(targs, TRUE_SLASH)[1]='\0';
		build_h_prefix=new char[strlen(targs)+strlen(bckph)+2];
		sprintf(build_h_prefix,"%s%s%c",targs,bckph,TRUE_SLASH);
		CreateBackUP(build_h_prefix,&huirdrsch[check]);
		delete build_h_prefix;
		deflen=strlen((strrchr(huirdrsch[check].str, TRUE_SLASH)+1));
		w_cut=new char[deflen+1];
		strcpy(w_cut,(strrchr(huirdrsch[check].str, TRUE_SLASH)+1));

		work_prefix=new char[strlen(targs)+strlen(anyprefix)+strlen(w_cut)+2];
		UpperCase(w_cut);
		sprintf(work_prefix,"%s%s%s%c",targs,anyprefix,w_cut,TRUE_SLASH);

		if(((fh=fopen(huirdrsch[check].str,"rb"))!=NULL)||IsDir(work_prefix)){
			if(fh)fclose(fh);
			WDR += ReMakeANDCatalogs(work_prefix, huirdrsch[check].str);
		}
	delete work_prefix;
	delete w_cut;
	delete targs;
	delete huirdrsch[check].str;
	}
	delete []huirdrsch;


	FreeConfigs();
	//---<
	delete maindir;
	return 0;
}

