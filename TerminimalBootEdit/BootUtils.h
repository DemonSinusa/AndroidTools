/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   BootUtils.h
 * Author: aveman
 *
 * Created on 11 марта 2016 г., 20:16
 */

#ifndef BOOTUTILS_H
#define BOOTUTILS_H

#define KERNEL_BLK		1
#define RAMFS_BLK		2
#define SECONDFS_BLK	3

#define UfNtype char

#include <openssl/sha.h>

typedef struct boot_img_hdr_v1 droid_boot_header;

typedef struct mtk_boot_loader{
	char magic[4];
	unsigned int imagesize;
	char logo[32];
	char data[0x1d8];
}MTKBL;

typedef struct _main_page_block_{
	char *Loader,*data;
	int llen,dlen,pcount;
}MPBLK;

typedef struct _kernel_data_{
	char *gz,*dtb;
	int gzsz,dtbsz;
}Kdata;

class BootUtils
{
public:
	BootUtils(const BootUtils&);
	BootUtils(UfNtype *fname, int offset);
	virtual ~BootUtils();
	char *FullBoot = NULL;
	unsigned int static_flen = 0;
	MPBLK kernel,root_fs,second_part;
	Kdata kernel_data;
	droid_boot_header *GetCurMainConfig();
	void SetCurMainConfig(droid_boot_header *conf);

	void SetPageLoader(int BLK,int imglen,char *logo,int lsize);
	void SetPageLoader(int BLK,void *dump,int len);
	int SetPageLoader(int BLK,char *file);
	void SetPageData(int BLK,void *dump,int len);
	int SetPageData(int BLK,char *file);
	void *GetPageLoader(int BLK,int *len);
	void *GetPageData(int BLK,int *len);

	//4 Kernel only
	void SetDTBList(void *dump, int len);
	int SetDTBList(char *file);
	void *GetDTBList(int *len);
	void SetKernelGz(void *dump,int len);
	int SetKernelGz(char *file);
	void *GetKernelGz(int *len);

	int InjKernel(void *dump, int len);
	int InjKernel(char *fname);
	//4 Kernel only
	int InjROOTFS(void *dump, int len);
	int InjROOTFS(char *fname);
	int InjXZ401(void *dump, int len);
	int InjXZ401(char *fname);
	int InjDbo(void *dump,int len);
	int InjDbo(char *fname);
	void *GetCurKernel(int *len);
	void *GetCurROOTFS(int *len);
	void *GetCurXZ401(int *len);
	void *GetDbo(int *len);
private:
	UfNtype *Bimgname = NULL;

	droid_boot_header boot_h, *wbh = NULL;
	SHA_CTX ctx;
	unsigned char hash[SHA_DIGEST_LENGTH];
	int sha = 0;

	FILE *boot = NULL;
	unsigned int PhysOS = 0;



	int dbo_p_len=0x00;
	char *kernel_block_lnk = NULL,*rootfs_block_lnk = NULL,
		*unk_xzblk_lnk = NULL,*dbo_block_lnk=NULL;
	//
	int SpilitPage(int BLK,void *blk,int len);
	void *GluedPage(int BLK);
	void FreeInPage(int BLK);
	//
	int UnCollectKernelData();
	int CollectKernelData();
	bool OpenBFGrab(UfNtype *fname, int offset);
	void CloseBFile();

};

#endif /* BOOTUTILS_H */

