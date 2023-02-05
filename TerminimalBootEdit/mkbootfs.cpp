#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <stdarg.h>
#include <fcntl.h>

#include <bootimg.h>
#include "FRulezConfigs.h"

#include "mkbootfs.h"
//Я этого тааак нехотел...(
#include "BootUtils.h"
#include "LowLvlPck.h"
#include "Config.h"
//-----------((((((((((

extern OPT options[NUM_PROPS];

static void fix_stat(const char *path, struct stat *s) {
	fs_config(path, S_ISDIR(s->st_mode), &s->st_uid, &s->st_gid, &s->st_mode);
}

int PackFile(char *path, PCK *p) {
	int picesz = 0, dblksz = strlen(path), sotag = 0;
	char *packpath = new char[dblksz - p->cat_len + 1], *addyprt = NULL;
	struct stat opt;
	unsigned int omode=0;
//	unsigned int ouid=0,ogid=0;
	FILE *f = NULL;
	strcpy(packpath, &path[p->cat_len]);
	//Home virus inside in our brain with little small step ps\|
	if ((f = fopen(path, "rb")) != NULL) {
		picesz = dblksz - p->cat_len;
		dblksz = 6 + 8 * 13 + picesz + 1;
		lstat(path, &opt);
//		ogid=opt.st_gid;
//		ouid=opt.st_uid;
		omode=opt.st_mode;
		fix_stat(packpath, &opt);
		if (opt.st_mode==0) {		//Нет предустановленой информации
			opt.st_mode=omode;
		}

		while ((p->packsize + dblksz + sotag) & 3)
			sotag++;

		addyprt = new char[dblksz + sotag + 1];
		memset(addyprt, 0x00, dblksz + sotag);
		sprintf(addyprt, "%06x%08x%08x%08x%08x%08x%08x"
				"%08x%08x%08x%08x%08x%08x%08x%s%c",
				0x070701,
				p->inode_count++, //  s.st_ino,
				opt.st_mode,
				0, // s.st_uid,
				0, // s.st_gid,
				1, // s.st_nlink,
				(unsigned int) opt.st_mtime, // s.st_mtime,
				(unsigned int) opt.st_size, //data len to pack
				0, // volmajor
				0, // volminor
				0, // devmajor
				0, // devminor,
				picesz + 1, //path in pack len
				0,
				packpath, //path in pack
				0);

		if (fwrite(addyprt, dblksz + sotag, 1, p->fh) == 1) {
			p->packsize += dblksz + sotag;
			picesz = sotag;
			sotag = 0;
			delete[] addyprt;

			if (opt.st_size > 0) {
				while ((p->packsize + opt.st_size + sotag) & 3)
					sotag++;
				addyprt = new char[opt.st_size + sotag + 1];
				if (sotag > 0)
					memset(addyprt + opt.st_size, 0x00, sotag);
				if (fread(addyprt, opt.st_size, 1, f) != 1)
					fprintf(stderr, "*File:%s\r\n--File System KARRUPA&readn't but opened to read(%08X)! WTF!?(:)\r\n", path, f->_fileno);
				//Расклад такой, если не читаешь stderr то не узнаешь почему ниворкс
				fwrite(addyprt, opt.st_size + sotag, 1, p->fh);
				p->packsize += opt.st_size + sotag;

				delete[] addyprt;
			}
		} else {
			delete[] addyprt;
		}
		fclose(f);
	}
	delete[] packpath;

	return dblksz + picesz + opt.st_size + sotag;
}

int PackDir(char *path, PCK *p) {
	int picesz = 0, dblksz = strlen(path), sotag = 0;
	char *packpath = new char[dblksz - p->cat_len + 1], *addyprt = NULL;
	struct stat opt;
	unsigned int omode=0;
//	unsigned int ouid=0,ogid=0;
	strcpy(packpath, &path[p->cat_len]);
	picesz = dblksz - p->cat_len;
	dblksz = 6 + 8 * 13 + picesz + 1;
	lstat(path, &opt);
//	ogid=opt.st_gid;
//	ouid=opt.st_uid;
	omode=opt.st_mode;
	fix_stat(packpath, &opt);
	if (opt.st_mode==0) {		//Нет предустановленой информации
		opt.st_mode=omode;
	}
	while ((p->packsize + dblksz + sotag) & 3)
		sotag++;
	addyprt = new char[dblksz + sotag + 1];
	memset(addyprt, 0x00, dblksz + sotag);


	sprintf(addyprt, "%06x%08x%08x%08x%08x%08x%08x"
			"%08x%08x%08x%08x%08x%08x%08x%s%c",
			0x070701,
			p->inode_count++, 				//  s.st_ino,
			opt.st_mode,					//	privelegy
			0, 								// s.st_uid,
			0, 								// s.st_gid,
			1, 								// s.st_nlink,
			(unsigned int) opt.st_mtime,	// s.st_mtime,
			0, 								//data len to pack
			0, 								// volmajor
			0, 								// volminor
			0, 								// devmajor
			0, 								// devminor,
			picesz + 1, 					//path in pack len
			0,
			packpath, 						//path in pack
			0);

	if (fwrite(addyprt, dblksz + sotag, 1, p->fh) == 1) {
		p->packsize += dblksz + sotag;
	} else
		dblksz = sotag = 0;

	delete[] addyprt;
	delete[] packpath;
	return dblksz + sotag;
}

int PackAdata(struct stat *prop, char *data, int datalen, char *pckname, PCK *p) {
	int namelen = strlen(pckname);
	int picesz = 0, dblksz = 6 + 8 * 13 + namelen + 1, sotag = 0;
	char *addyprt = NULL;
	struct stat opt;
	unsigned int omode=0;
	if (!prop)
		memset(&opt, 0x00, sizeof (struct stat));
	else {
		memcpy(&opt, prop, sizeof (struct stat));
		omode=opt.st_mode;
		fix_stat(pckname, &opt);
		if (opt.st_mode==0) {		//Нет предустановленой информации
			opt.st_mode=omode;
		}
	}

	while ((p->packsize + dblksz + sotag) & 3)
		sotag++;

	addyprt = new char[dblksz + sotag + 1];
	memset(addyprt, 0x00, dblksz + sotag);


	sprintf(addyprt, "%06x%08x%08x%08x%08x%08x%08x"
			"%08x%08x%08x%08x%08x%08x%08x%s%c",
			0x070701,
			p->inode_count++, 		// s.st_ino,
			opt.st_mode,			//privelegy
			0, 						// s.st_uid,
			0, 						// s.st_gid,
			1, 						// s.st_nlink,
			(unsigned int)opt.st_mtime, 	// s.st_mtime,
			datalen, 				//data len to pack
			0, 						// volmajor
			0, 						// volminor
			0, 						// devmajor
			0, 						// devminor,
			namelen + 1, 			//path in pack len
			0,
			pckname, 				//path in pack
			0);

	if (fwrite(addyprt, dblksz + sotag, 1, p->fh) == 1) {
		p->packsize += dblksz + sotag;
		delete[] addyprt;

		if (datalen > 0) {
			while ((p->packsize + datalen + picesz) & 3)
				picesz++;
			addyprt = new char[datalen + picesz + 1];
			memcpy(addyprt, data, datalen);
			memset(&addyprt[datalen], 0x00, picesz);
			fwrite(addyprt, datalen + picesz, 1, p->fh);
			p->packsize += datalen + picesz;
			delete[] addyprt;
		}
	} else {
		delete[] addyprt;
	}

	return dblksz + sotag + datalen + picesz;
}

int CreateList(char *curdir, PCK *p) {
	int itemcount = 0, seglen=0;
	char *path = NULL, *buf = NULL;
	struct stat opt,yopt;
	struct dirent *de;
	DIR *d;

	if ((d = opendir(curdir)) != NULL) {
		while ((de = readdir(d)) != NULL) {
			if ((!strcmp(de->d_name, ".")) || (!strcmp(de->d_name, "..")))
				continue;
			if ((de->d_type == DT_DIR) || (de->d_type == DT_REG) || (de->d_type == DT_LNK)) {
				path = new char[strlen(curdir) + strlen(de->d_name) + 2];
				sprintf(path, "%s/%s", curdir, de->d_name);
				if (de->d_type == DT_DIR) {
					seglen = PackDir(path, p);
					itemcount++;
					itemcount += CreateList(path, p);
				} else if (de->d_type == DT_REG) {
					seglen = PackFile(path, p);
					itemcount++;
				} else if (de->d_type == DT_LNK) {
					if (!lstat(path, &opt)) {
						buf = new char[opt.st_size + 1];
						memset(buf, 0x00, opt.st_size + 1);
						readlink(path, buf, opt.st_size);
						if (!stat(path,&yopt)) {
							opt.st_mode=yopt.st_mode|__S_IFLNK;
						}
						seglen = PackAdata(&opt, buf, opt.st_size, &path[p->cat_len], p);
						itemcount++;
						delete[] buf;

					}
				}
#ifndef DEBUG
				fprintf(stdout, "*Каталог:%s\r\n--Вхождение:%s(%dbsz)\r\n", curdir, de->d_name, seglen);
#else
				fprintf(stdout, "%s/%s\r\n", curdir, de->d_name);
				seglen=seglen;
#endif
				delete[] path;
			} else
				fprintf(stderr, "*File:%s\r\n--Чокнутый штолэ? такого вообще-то не бывает в нашем случаеX)!\r\n", de->d_name);

		}
		closedir(d);
	}
	return itemcount;
}

PCK *InitPacker(char *dir, char *afile, unsigned int nsc) {
	PCK *packer = new PCK;
	packer->cat_len = strlen(dir) + 1;
	packer->catalog = new char[packer->cat_len];
	strcpy(packer->catalog, dir);


	if ((packer->fh = fopen(afile, "wt")) == NULL) {
		delete[] packer->catalog;
		delete packer;
		packer = NULL;
	}

	packer->inode_count = nsc;
	packer->packsize = 0;

	return packer;
}

void FInitPacker(PCK *p) {
	int ffcount = 0,aliseek=0xff;
	char *dich = (char *) "TRAILER!!!", *sotag = NULL;
	struct stat opt;
	opt.st_mode=420;
	PackAdata(&opt, NULL, 0, dich, p);
//#warning "Эксперементальные рассчеты-приmo4ki"
if(options[PROP_CPIOBLK_ALIGN].len>0){
	sscanf((char *)options[PROP_CPIOBLK_ALIGN].txtname,"0x%08x",&aliseek);
	while ((p->packsize + ffcount)& aliseek)ffcount++;
	}
//-----------------------------------
	if (ffcount > 0) {
		sotag = new char[ffcount];
		memset(sotag, 0x00, ffcount);
		fwrite(sotag, ffcount, 1, p->fh);
		p->packsize += ffcount;
		delete[] sotag;
	}
	fclose(p->fh);
	delete[] p->catalog;
	delete p;
}
