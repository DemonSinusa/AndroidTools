#include <cstdlib>

using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>


#include "FRulezConfigs.h"

extern AFPC *config;

AFPC def_file= {00644,NULL,NULL,NULL,NULL};
AFPC def_dir= {00755,NULL,NULL,NULL,NULL};


bool IsDir(char *path) {
	struct stat tistica;
	if (!stat(path, &tistica)) {
		if (tistica.st_mode & (S_IFMT | S_IFDIR))
			return true;
		else
			return false;
	}
	return false;
}

int IsRuleSymb(char *str) {
	if(*str=='\r'||*str=='\n')
		return 1;
	if(*str=='/'&&*(str+1)=='*')
		return 2;
	if((*str=='\\'&&*(str+1)=='\\')||(*str=='/'&&*(str+1)=='/')||(*str==';'))
		return 3;
	if(*str=='*'&&*(str+1)=='/')
		return 4;
	//    if(*str==' '||*str=='\t')return 5;
	return 0;
}

int otod(int octanum) {
	int demn=0,i=0,rem=0;
	while(octanum!=0) {
		rem=octanum%10;
		octanum/=10;
		demn+=rem*pow(8,i);
		i++;
	}
	return demn;
}

int PassMLComment(char *data) {
	char *mass=data;
	int len=2;
	mass+=len;
	while(*mass) {
		if(*mass=='*'&&*(mass+1)=='/')
			break;
		len++;
		mass++;
	}
	return len;
}

int PassComment(char *data) {
	char *mass=data;
	int len=0;
	if(*mass==';') {
		len=1;
	} else {
		len=2;
	}
	mass+=len;
	while(*mass) {
		if(IsRuleSymb(mass)==1)
			break;
		mass++;
		len++;
	}
	return len;
}

int PassSpacesTab(char *data) {
	char *temp=data;
	int len=0;
	while(*temp&&(*temp==' '||*temp=='\t')) {
		len++;
		temp++;
	}
	return len;
}


SSIM *DoParse(char *mem) {              //Ôóíêöèþ ïåðåïèñàòü
	char *temp=mem;
	int slen=0,addstr=0,aplen=0,count=0;
	SSIM *root=NULL,*temps=NULL;

	temps=new SSIM;
	memset(temps,0,sizeof(SSIM));
	root=temps;

	while(*temp) {

		if((slen=PassSpacesTab(temp))) {
			//memset(temp,0,slen);
			temp+=slen;
			continue;
		}
		if(IsRuleSymb(temp)==1) {
			*temp=0;
			temp++;
			continue;
		}

		if(IsRuleSymb(temp)==2&&!addstr) {
			slen=PassMLComment(temp);
			memset(temp,0,2);
			temps->comtype=MULTILINE_COMMENT;
			temps->str.len=slen-4;
			temps->str.str=temp+2;
			memset(temp+slen,0,2);
			temp+=slen+2;
			addstr=1;
		}
		if(IsRuleSymb(temp)==3&&!addstr) {
			slen=PassComment(temp);
			temps->comtype=ONELINE_COMMENT;
			if(*temp==';') {
				*temp=0;
				temps->str.len=slen-1;
				temps->str.str=temp+1;
			} else {
				memset(temp,0,2);
				temps->str.len=slen-2;
				temps->str.str=temp+2;
			}
			temp+=slen;
			addstr=1;
		}
		if(*temp&&!addstr) {
			addstr=1;
			temps->comtype=NONE_COMMENT;
			temps->str.str=temp;
			slen=0;
			aplen=0;
			while(!IsRuleSymb(temp)) {
				if(*temp=='\"') {
					aplen=strchr(temp+1,'\"')-temp;
					slen+=aplen+1;
					temp+=aplen+1;
				} else {
					slen++;
					temp++;
				}
			}
			temps->str.len=slen;
		}
		//------------------------------
		if(addstr==1) {
			temps->next=new SSIM;
			memset(temps->next,0,sizeof(SSIM));
			temps->next->prev=temps;
			temps=temps->next;
			addstr=0;
			count++;
		}

	}

	if(temps==root)
		root=NULL;
	else
		temps->prev->next=NULL;
	delete temps;

	return root;
}


void ParseClear(SSIM *root) {
	SSIM *temp=root;
	while(root) {
		temp=root;
		root=root->next;
		delete temp;
	}
}

AUI *FindByName(AUI *root, char *name) {
	AUI *cur=root;
	if(cur&&name)
		do {
			if(strlen(name)==strlen(cur->name)) {
				if(strstr(cur->name,name))
					break;
			}
		} while((cur=cur->next));
	return cur;
}

AUI *ParseIDs(SSIM *ids,int *count) {
	SSIM *g=ids;
	AUI *curg=NULL,*root=NULL;
	int segmentlength=0;
	if(g)
		do {
			if(g->comtype==NONE_COMMENT) {
				if(!root) {
					root=curg=new AUI;
				} else {
					curg->next=new AUI;
					curg=curg->next;
				}
				memset(curg,0,sizeof(AUI));
				segmentlength=strchr(g->str.str,'=')-g->str.str;
				curg->name=new char[segmentlength+1];
				strncpy(curg->name,g->str.str,segmentlength);
				curg->aid=atoi(&g->str.str[segmentlength+1]);
				*count+=1;
			}
		} while((g=g->next));
	def_dir.u_gid=FindByName(root,(char *)"root");
	def_dir.u_uid=FindByName(root,(char *)"root");;
	def_file.u_gid=FindByName(root,(char *)"root");;
	def_file.u_uid=FindByName(root,(char *)"root");;
	return root;
}

int SetAFPC(char *str,int len,AUI *all,AFPC *a) {
	char *tstr=new char[len+1],*pref=NULL,*usr=NULL,*grp=NULL,*mode=NULL;
	strcpy(tstr,str);

	pref=strchr(tstr,'=');
	*pref='\0';
	pref+=1;
	pref+=PassSpacesTab(pref);
	if(*pref=='\"') {
		strchr(pref+1,'\"')[0]='\0';
		pref+=1;
	}
	a->prefix=new char[strlen(pref)+1];
	strcpy(a->prefix,pref);
	usr=strchr(tstr,'|');
	*usr='\0';
	usr+=1;
	mode=tstr;
	usr+=PassSpacesTab(usr);
	grp=strchr(usr,':');
	*grp='\0';
	grp+=1;
	grp+=PassSpacesTab(grp);
	a->mode=otod(atoi(mode));
	AUI *c=FindByName(all,usr);
	a->u_uid=c;
	c=FindByName(all,grp);
	a->u_gid=c;

	delete tstr;
	return 0;
}

AFPC *ParseFS(SSIM *fs,AUI *all,int *count) {
	SSIM *g=fs;
	AFPC *curg=NULL,*root=NULL;
	*count=0;
	if(g)
		do {
			if(g->comtype==NONE_COMMENT) {
				if(!root) {
					root=curg=new AFPC;
				} else {
					curg->next=new AFPC;
					curg=curg->next;
				}
				memset(curg,0,sizeof(AFPC));
				if(!SetAFPC(g->str.str,g->str.len,all,curg))
					*count+=1;
			}
		} while((g=g->next));
	return root;
}


void fs_config(const char *path, int dir,unsigned *uid, unsigned *gid, unsigned *mode) {
	AFPC *pc=config;
	int plen=0,len=0;
		plen = strlen(path);
		if(pc)
			do {
				len = strlen(pc->prefix);
				if (dir) {
					if (plen < len)
						continue;
					if (!strncmp(pc->prefix, path, len))
						break;
					continue;
				}
				/* If name ends in * then allow partial matches. */
				if (pc->prefix[len - 1] == '*') {
					if (!strncmp(pc->prefix, path, len - 1))
						break;
				} else if (plen == len) {
					if (!strncmp(pc->prefix, path, len))
						break;
				}
			} while((pc=pc->next));
		if(!pc) {
			if(dir)
				pc=&def_dir;
			else
				pc=&def_file;
		}

		*uid = pc->u_uid->aid;
		*gid = pc->u_gid->aid;
		*mode = (*mode & (~07777)) | pc->mode;

		#if 0
		fprintf(stderr, "< '%s' '%s' %d %d %o >\n",
		        path, pc->prefix ? pc->prefix : "", *uid, *gid, *mode);
		#endif
}

void FreeAIDs(AUI *root) {
	AUI *uids=root,*rmpr=NULL;
	if(uids)
		do {
			rmpr=uids->next;
			delete uids->name;
			delete uids;
		} while((uids=rmpr));
}

void FreeAFPCs(AFPC *root) {
	AFPC *files=root,*fmpr=NULL;
	if(files)
		do {
			fmpr=files->next;
			delete files->prefix;
			delete files;
		} while((files=fmpr));
}

