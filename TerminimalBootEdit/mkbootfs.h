/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   mkbootfs.h
 * Author: senjord
 *
 * Created on 20 мая 2018 г., 13:49
 */

#ifndef MKBOOTFS_H
#define MKBOOTFS_H

typedef struct _packer_ {
	unsigned int inode_count, packsize, cat_len;
	char *catalog;
	FILE *fh;
} PCK;



PCK *InitPacker(char *dir, char *afile, unsigned int nsc);
int CreateList(char *curdir, PCK *p);
void FInitPacker(PCK *p);

#endif /* MKBOOTFS_H */


