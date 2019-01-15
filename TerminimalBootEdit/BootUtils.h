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

#define UfNtype char

#include <openssl/sha.h>

typedef struct boot_img_hdr_v1 droid_boot_header;

class BootUtils
{
public:
	BootUtils(const BootUtils&);
	BootUtils(UfNtype *fname, int offset);
	virtual ~BootUtils();
	char *FullBoot = NULL;
	unsigned int static_flen = 0;
	droid_boot_header *GetCurMainConfig();
	void SetCurMainConfig(droid_boot_header *conf);
	//4 Kernel only
	void SetBootLoader(void *dump,int len);
	int SetBootLoader(char *file);
	void *GetBootLoader(int *len);
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
	int kernel_p_len = 0x00, rootfs_p_len = 0x00, unk_xzblk_p_len = 0x00,dbo_p_len=0x00;
	char *kernel_block_lnk = NULL,*rootfs_block_lnk = NULL,\
		*unk_xzblk_lnk = NULL,*dbo_block_lnk=NULL;

	char *kernel_loader=NULL,*kernel_DTB_list=NULL,*kernel_gz_img=NULL;
	int kernel_loader_len=0,kernel_DTB_list_len=0,kernel_gz_img_len=0;

	int UnCollectKernel(void *kernel,int len);
	void *CollectKernel(int *len);
	bool OpenBFGrab(UfNtype *fname, int offset);
	void CloseBFile();

};

#endif /* BOOTUTILS_H */

