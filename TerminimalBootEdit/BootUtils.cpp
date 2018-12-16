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
	memset(&boot_h, 0x00, sizeof (boot_img_hdr));
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

	if ((boot = fopen(fname, "r+b")) == NULL)
		return false;
	fseek(boot, 0, SEEK_END);
	static_flen = ftell(boot);
	fseek(boot, PhysOS, SEEK_SET);

	if ((fread(&boot_h, sizeof (boot_img_hdr), 1, boot)) == 1)
	{
		wbh = &boot_h;
		if (static_flen > boot_h.page_size)
		{
			//Сгенерируем размеры блоков
			kernel_p_len = (boot_h.kernel_size + boot_h.page_size - 1) / boot_h.page_size;
			rootfs_p_len = (boot_h.ramdisk_size + boot_h.page_size - 1) / boot_h.page_size;
			unk_xzblk_p_len = (boot_h.second_size + boot_h.page_size - 1) / boot_h.page_size;

			fseek(boot, boot_h.page_size + PhysOS, SEEK_SET);

			//Начальное ядро
			kernel_block_lnk = new char[kernel_p_len * boot_h.page_size];
			memset(kernel_block_lnk, 0x00, kernel_p_len * boot_h.page_size);
			fread(kernel_block_lnk, boot_h.kernel_size, 1, boot);
			fseek(boot, (kernel_p_len + 1) * boot_h.page_size +
			      PhysOS, SEEK_SET);

			rootfs_block_lnk = new char[rootfs_p_len * boot_h.page_size];
			memset(rootfs_block_lnk, 0x00, rootfs_p_len * boot_h.page_size);
			fread(rootfs_block_lnk, boot_h.ramdisk_size, 1, boot);
			fseek(boot, (rootfs_p_len + kernel_p_len + 1) * boot_h.page_size +
			      PhysOS, SEEK_SET);

			if (boot_h.second_size > 0)
			{
				unk_xzblk_lnk = new char[unk_xzblk_p_len * boot_h.page_size];
				memset(unk_xzblk_lnk, 0x00, unk_xzblk_p_len * boot_h.page_size);
				fread(unk_xzblk_lnk, boot_h.second_size, 1, boot);
			}
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
	//    int p = 0;
	if (wbh)
	{

		int bootlen = PhysOS + wbh->page_size; //+1page to config

		bootlen += kernel_p_len * wbh->page_size; //+kernel

		bootlen += rootfs_p_len * wbh->page_size; //+rootfs

		bootlen += unk_xzblk_p_len * wbh->page_size; //Second stage

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
				sha = SHA1_Final(hash, &ctx);
				if (sha == 1)
					memcpy(wbh->id, hash,
					       SHA_DIGEST_LENGTH > sizeof (wbh->id) ? sizeof (wbh->id) : SHA_DIGEST_LENGTH);


				memcpy(wbh->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

				memcpy(&FullBoot[wbh->page_size + PhysOS], kernel_block_lnk,
				       wbh->kernel_size);
				memcpy(&FullBoot[(kernel_p_len + 1) * wbh->page_size + PhysOS],
				       rootfs_block_lnk, wbh->ramdisk_size);
				if (unk_xzblk_p_len > 0)
					memcpy(&FullBoot[(kernel_p_len + rootfs_p_len + 1) * boot_h.page_size + PhysOS],
					       unk_xzblk_lnk, wbh->second_size);
				memcpy(&FullBoot[ PhysOS], wbh, sizeof (boot_img_hdr));

				fseek(boot, 0, SEEK_SET);
				fwrite(FullBoot, static_flen, 1, boot);

				//---++++++++++++++++++++++=)
				fclose(boot);
				if (kernel_block_lnk)
					delete kernel_block_lnk;
				if (rootfs_block_lnk)
					delete rootfs_block_lnk;
				if (unk_xzblk_lnk)
					delete unk_xzblk_lnk;
				delete FullBoot;
			}
		}
	}
}

int BootUtils::InjKernel(void *dump, int len)
{
	if (!dump)
		return 0;
	if (len <= 0)
		return -1;

	wbh->kernel_size = len;

	kernel_p_len = (len + boot_h.page_size - 1) / boot_h.page_size;
	if (kernel_block_lnk != NULL)
		delete kernel_block_lnk;
	kernel_block_lnk = new char[kernel_p_len * boot_h.page_size];
	ZeroMem(kernel_block_lnk, kernel_p_len * boot_h.page_size);
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
	if (!dump)
		return 0;
	if (len <= 0)
		return -1;

	wbh->ramdisk_size = len;

	rootfs_p_len = (len + boot_h.page_size - 1) / boot_h.page_size;
	if (rootfs_block_lnk != NULL)
		delete rootfs_block_lnk;
	rootfs_block_lnk = new char [rootfs_p_len * boot_h.page_size];
	ZeroMem(rootfs_block_lnk, rootfs_p_len * boot_h.page_size);
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
	if (!dump)
		return 0;
	if (len <= 0)
		return -1;

	wbh->second_size = len;

	unk_xzblk_p_len = (len + boot_h.page_size - 1) / boot_h.page_size;
	if (unk_xzblk_lnk != NULL)
		delete unk_xzblk_lnk;
	unk_xzblk_lnk = new char[unk_xzblk_p_len * boot_h.page_size];
	ZeroMem(unk_xzblk_lnk, unk_xzblk_p_len * boot_h.page_size);
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

void *BootUtils::GetCurKernel(int *len)
{
	if (wbh == NULL)
		return NULL;
	if (len)
		*len = wbh->kernel_size;
	return kernel_block_lnk;
}

void *BootUtils::GetCurROOTFS(int *len)
{
	if (wbh == NULL)
		return NULL;
	if (len)
		*len = wbh->ramdisk_size;
	return rootfs_block_lnk;
}

void *BootUtils::GetCurXZ401(int *len)
{
	if (wbh == NULL)
		return NULL;
	if (len)
		*len = wbh->second_size;
	return unk_xzblk_lnk;
}

boot_img_hdr *BootUtils::GetCurMainConfig()
{
	return wbh;
}

void BootUtils::SetCurMainConfig(boot_img_hdr *conf)
{
	if (conf)
	{
		wbh = &boot_h;
		memcpy(wbh, conf, sizeof (boot_img_hdr));
	}
}

