/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   BootUtils.cpp
 * Author: aveman
 *
 * Created on 11 РјР°СЂС‚Р° 2016 Рі., 20:16
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <bootimg.h>

#include "BootUtils.h"

char zeroid = 0x00;

void ZeroMem(void *ptr, int sz)
{
	memset(ptr, zeroid, sz);
}

BootUtils::BootUtils(UfNtype *fname, int offset)
{
	boot = NULL;
	static_flen = 0;
	Bimgname = new UfNtype[strlen(fname) + 1];
	strcpy(Bimgname, fname);
	memset(&boot_h, 0x00, sizeof (droid_boot_header));
	memset(&ctx, 0x00, sizeof (SHA_CTX));

	PhysOS = offset;
	kernel_p_len = rootfs_p_len = unk_xzblk_p_len = 0;

	OpenBFGrab(fname, offset);
}

BootUtils::~BootUtils()
{
	CloseBFile();
	delete Bimgname;
}

bool BootUtils::OpenBFGrab(UfNtype *fname, int offset)
{

	bool retval = true;
	char *bootdump=NULL;

	if ((boot = fopen(fname, "r+b")) == NULL)
		return false;
	fseek(boot, 0, SEEK_END);
	static_flen = ftell(boot);
	fseek(boot, PhysOS, SEEK_SET);

	if ((fread(&boot_h, sizeof (boot_img_hdr), 1, boot)) == 1)
	{
		if(boot_h.header_version==1){
			fread(&((char *)&boot_h)[sizeof (boot_img_hdr)], sizeof(droid_boot_header)-sizeof (boot_img_hdr), 1, boot);
		}
		wbh = &boot_h;
		if (static_flen > boot_h.page_size)
		{

			bootdump=new char[static_flen-(boot_h.page_size + PhysOS)];
			fseek(boot, boot_h.page_size + PhysOS, SEEK_SET);
			fread(bootdump, static_flen-(boot_h.page_size + PhysOS), 1, boot);

			//Сгенерируем размеры блоков
			kernel_p_len = (boot_h.kernel_size + boot_h.page_size - 1) / boot_h.page_size;
			rootfs_p_len = (boot_h.ramdisk_size + boot_h.page_size - 1) / boot_h.page_size;
			unk_xzblk_p_len = (boot_h.second_size + boot_h.page_size - 1) / boot_h.page_size;
			dbo_p_len=(boot_h.recovery_dtbo_size+boot_h.page_size-1)/boot_h.page_size;

            InjKernel(bootdump,boot_h.kernel_size);
            InjROOTFS(&bootdump[kernel_p_len * boot_h.page_size],boot_h.ramdisk_size);
            if (boot_h.second_size > 0){
            InjXZ401(&bootdump[kernel_p_len * boot_h.page_size+rootfs_p_len * boot_h.page_size],boot_h.second_size);
            }
            if(boot_h.recovery_dtbo_size>0){
            InjDbo(&bootdump[kernel_p_len * boot_h.page_size+rootfs_p_len * boot_h.page_size+unk_xzblk_p_len * boot_h.page_size],boot_h.recovery_dtbo_size);
            }

            delete bootdump;
		}
		else
			retval = false;
	}
	else
		retval = false;
	fclose(boot);
	boot = NULL;
	return retval;
}


void BootUtils::CloseBFile()
{
//	int p = 0;
	if (wbh)
	{

		CollectKernel(NULL);

		int bootlen = PhysOS + wbh->page_size; //+1page to config

		bootlen += kernel_p_len * wbh->page_size; //+kernel

		bootlen += rootfs_p_len * wbh->page_size; //+rootfs

		bootlen += unk_xzblk_p_len * wbh->page_size; //Second stage

		bootlen+=dbo_p_len*wbh->page_size;

		static_flen = bootlen;


		if (static_flen > PhysOS)
		{
			if ((boot = fopen(Bimgname, "wt+b")) != NULL)
			{
				FullBoot = new char[static_flen];

				//0xFF?>:)
				ZeroMem(FullBoot, static_flen);
				//0x0
				ZeroMem(wbh->id, sizeof (wbh->id));

				SHA1_Init(&ctx);
				SHA1_Update(&ctx, kernel_block_lnk, wbh->kernel_size);
				SHA1_Update(&ctx, &wbh->kernel_size, sizeof (wbh->kernel_size));
				SHA1_Update(&ctx, rootfs_block_lnk, wbh->ramdisk_size);
				SHA1_Update(&ctx, &wbh->ramdisk_size, sizeof (wbh->ramdisk_size));
				SHA1_Update(&ctx, unk_xzblk_lnk, wbh->second_size);
				SHA1_Update(&ctx, &wbh->second_size, sizeof (wbh->second_size));
				if(wbh->header_version==1){
					SHA1_Update(&ctx, dbo_block_lnk, wbh->recovery_dtbo_size);
					SHA1_Update(&ctx, &wbh->recovery_dtbo_size, sizeof (wbh->recovery_dtbo_size));
				}
				sha = SHA1_Final(hash, &ctx);
				if (sha == 1)
					memcpy(wbh->id, hash,
					       SHA_DIGEST_LENGTH > sizeof (wbh->id) ? sizeof (wbh->id) : SHA_DIGEST_LENGTH);


				memcpy(wbh->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
				memcpy(&FullBoot[ PhysOS], wbh, sizeof (boot_img_hdr));

				memcpy(&FullBoot[wbh->page_size + PhysOS], kernel_block_lnk,
				       wbh->kernel_size);
				memcpy(&FullBoot[(kernel_p_len + 1) * wbh->page_size + PhysOS],
				       rootfs_block_lnk, wbh->ramdisk_size);
				if (unk_xzblk_p_len > 0)
					memcpy(&FullBoot[(kernel_p_len + rootfs_p_len + 1) * boot_h.page_size + PhysOS],
					       unk_xzblk_lnk, wbh->second_size);

				if(wbh->header_version==1){
					memcpy(&FullBoot[ PhysOS+sizeof (boot_img_hdr)], &((char *)wbh)[sizeof (boot_img_hdr)], sizeof(droid_boot_header)-sizeof (boot_img_hdr));
                    if(dbo_p_len>0)
                    	memcpy(&FullBoot[(kernel_p_len + rootfs_p_len+unk_xzblk_p_len + 1) * boot_h.page_size + PhysOS],
					       dbo_block_lnk, wbh->recovery_dtbo_size);
				}

				fseek(boot, 0, SEEK_SET);
				fwrite(FullBoot, static_flen, 1, boot);

				//---++++++++++++++++++++++=)
				fclose(boot);
				delete FullBoot;
			}
		}

		if (kernel_block_lnk)
					delete kernel_block_lnk;
				if (rootfs_block_lnk)
					delete rootfs_block_lnk;
				if (unk_xzblk_lnk)
					delete unk_xzblk_lnk;
				if(dbo_block_lnk)
					delete dbo_block_lnk;
	}
}

void BootUtils::SetBootLoader(void *dump,int len){
	if(!dump||len<1)return;
	if(kernel_loader_len>0) delete kernel_loader;
	kernel_loader_len=len;
    kernel_loader=new char[len];
    memcpy(kernel_loader,dump,len);
}

int BootUtils::SetBootLoader(char *fname)
{
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL)
	{
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if(fread(data, datalen, 1, fh)==1)
			SetBootLoader(data, datalen);
		delete data;
		fclose(fh);
		return 1;
	}
	else
		return 0;
}

void *BootUtils::GetBootLoader(int *len){
	if (kernel_loader_len == 0)return NULL;

	if (len)
		*len=kernel_loader_len;

	return kernel_loader;
}

void BootUtils::SetDTBList(void *dump, int len){
	if(!dump||len<1)return;
	if(kernel_DTB_list_len>0)delete kernel_DTB_list;
	kernel_DTB_list_len=len;
	kernel_DTB_list=new char[len];
	memcpy(kernel_DTB_list,dump,len);
}
int BootUtils::SetDTBList(char *fname)
{
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL)
	{
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if(fread(data, datalen, 1, fh)==1)
			SetDTBList(data, datalen);
		delete data;
		fclose(fh);
		return 1;
	}
	else
		return 0;
}

void *BootUtils::GetDTBList(int *len){
	if (kernel_DTB_list_len == 0)return NULL;

	if (len)
		*len=kernel_DTB_list_len;
	return kernel_DTB_list;
}

void BootUtils::SetKernelGz(void *dump,int len){
	if(!dump||len<1)return;
	if(kernel_gz_img_len>0)delete kernel_gz_img;
	kernel_gz_img_len=len;
	kernel_gz_img=new char[len];
	memcpy(kernel_gz_img,dump,len);
}
int BootUtils::SetKernelGz(char *fname){
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL)
	{
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if(fread(data, datalen, 1, fh)==1)
			SetKernelGz(data, datalen);
		delete data;
		fclose(fh);
		return 1;
	}
	else
		return 0;
}
void *BootUtils::GetKernelGz(int *len){
if (kernel_gz_img_len == 0)
return NULL;

if (len)
*len=kernel_gz_img_len;

	return kernel_gz_img;
}

int BootUtils::UnCollectKernel(void *kernel,int len){
	int retval=0,BootloadLen=0,KernelGz=0;
	char *sig=(char *)kernel;

    while((sig!=NULL)&&(*(unsigned int *)(sig)!=0x00088B1F)){ //gzip сигнатура
    sig++;
    BootloadLen++;
    }
    if(BootloadLen>0)retval+=1;
    SetBootLoader(kernel,BootloadLen);
    while((sig!=NULL)&&*(unsigned int *)(sig)!=0xEDFE0DD0){ //DTB сигнатура
    sig++;
    KernelGz++;
    }
    if(KernelGz>0)retval+=1;
    SetKernelGz(((char *)kernel)+BootloadLen,KernelGz);

    if(len-KernelGz-BootloadLen>0)retval+=1;
    SetDTBList(sig,len-KernelGz-BootloadLen);

	return retval;
}

void *BootUtils::CollectKernel(int *len){
if(!wbh)return NULL;
boot_h.kernel_size = kernel_gz_img_len+kernel_loader_len+kernel_DTB_list_len;

if (boot_h.kernel_size == 0)
		return NULL;
	if (len) *len=boot_h.kernel_size;
kernel_p_len = (boot_h.kernel_size+ boot_h.page_size - 1) / boot_h.page_size;
if(kernel_block_lnk!=NULL)delete kernel_block_lnk;
kernel_block_lnk=new char[boot_h.kernel_size];

if(kernel_loader_len>0){
	memcpy(kernel_block_lnk,kernel_loader,kernel_loader_len);
	}

if(kernel_gz_img_len>0){
	memcpy(kernel_block_lnk, kernel_gz_img, kernel_gz_img_len);
	}

if(kernel_DTB_list_len>0){
	memcpy(&kernel_block_lnk[kernel_loader_len+kernel_gz_img_len],kernel_DTB_list,kernel_DTB_list_len);
	}

	return kernel_block_lnk;
}


int BootUtils::InjKernel(void *dump, int len)
{
	if(!dump||len<1)return 0;

	wbh->kernel_size = len;

	//Сразу разбираем
	UnCollectKernel(dump,len);

	kernel_p_len = (len + boot_h.page_size - 1) / boot_h.page_size;
	if (kernel_block_lnk != NULL)
		delete kernel_block_lnk;
	kernel_block_lnk = new char[len];

	memcpy(kernel_block_lnk, dump, len);

	return 1;
}

int BootUtils::InjKernel(char *fname)
{
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL)
	{
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if(fread(data, datalen, 1, fh)==1)
			InjKernel(data, datalen);
		delete data;
		fclose(fh);
		return 1;
	}
	else
		return 0;
}

int BootUtils::InjROOTFS(void *dump, int len)
{
	if(!dump||len<1)return 0;

	wbh->ramdisk_size = len;

	rootfs_p_len = (len + boot_h.page_size - 1) / boot_h.page_size;
	if (rootfs_block_lnk != NULL)
		delete rootfs_block_lnk;
	rootfs_block_lnk = new char [len];
//	ZeroMem(rootfs_block_lnk, len);
	memcpy(rootfs_block_lnk, dump, len);

	return 1;
}

int BootUtils::InjROOTFS(char *fname)
{
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL)
	{
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if(fread(data, datalen, 1, fh)==1)
			InjROOTFS(data, datalen);
		delete data;
		fclose(fh);
		return 1;
	}
	else
		return 0;
}

int BootUtils::InjXZ401(void *dump, int len)
{
	if(!dump||len<1)return 0;

	wbh->second_size = len;

	unk_xzblk_p_len = (len + boot_h.page_size - 1) / boot_h.page_size;
	if (unk_xzblk_lnk != NULL)
		delete unk_xzblk_lnk;
	unk_xzblk_lnk = new char[len];
//	ZeroMem(unk_xzblk_lnk, len);
	memcpy(unk_xzblk_lnk, dump, len);

	return 1;
}

int BootUtils::InjXZ401(char *fname)
{
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL)
	{
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if(fread(data, datalen, 1, fh)==1)
			InjXZ401(data, datalen);
		delete data;
		fclose(fh);
		return 1;
	}
	else
		return 0;
}

int BootUtils::InjDbo(void *dump,int len){
	if(!dump||len<1)return 0;

	wbh->recovery_dtbo_size=len;

	dbo_p_len=(len + boot_h.page_size - 1) / boot_h.page_size;
	if(dbo_block_lnk!=NULL)
		delete dbo_block_lnk;
	dbo_block_lnk=new char[len];
//    ZeroMem(dbo_block_lnk,len);
    memcpy(dbo_block_lnk,dump,len);
    return 1;
}

int BootUtils::InjDbo(char *fname){
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL)
	{
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if(fread(data, datalen, 1, fh)==1)
			InjDbo(data, datalen);
		delete data;
		fclose(fh);
		return 1;
	}
	else
		return 0;
}

void *BootUtils::GetCurKernel(int *len)
{
	if (wbh == NULL)
		return NULL;
	if (len)
		*len = wbh->kernel_size;

	CollectKernel(NULL);

	return kernel_block_lnk;
}

void *BootUtils::GetCurROOTFS(int *len)
{
	if (wbh == NULL)
		return NULL;
	if (len)
		*len = wbh->ramdisk_size;

	unk_xzblk_p_len = (*len + boot_h.page_size - 1) / boot_h.page_size;
	return rootfs_block_lnk;
}

void *BootUtils::GetCurXZ401(int *len)
{
	if (wbh == NULL)
		return NULL;
	if (len)
		*len = wbh->second_size;
unk_xzblk_p_len = (*len + boot_h.page_size - 1) / boot_h.page_size;
	return unk_xzblk_lnk;
}

void *BootUtils::GetDbo(int *len){
	if(wbh==NULL)
		return NULL;
	if(len)
		*len=wbh->recovery_dtbo_size;
	dbo_p_len=(*len + boot_h.page_size - 1) / boot_h.page_size;
	return dbo_block_lnk;
}

droid_boot_header *BootUtils::GetCurMainConfig()
{
	return wbh;
}

void BootUtils::SetCurMainConfig(droid_boot_header *conf)
{
	if (conf)
	{
		wbh = &boot_h;
		memcpy(wbh, conf, sizeof (droid_boot_header));
	}
}

