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

	ZeroMem(&kernel,sizeof(MPBLK));
	ZeroMem(&kernel_data,sizeof(Kdata));

	ZeroMem(&root_fs,sizeof(MPBLK));
	ZeroMem(&second_part,sizeof(MPBLK));

	OpenBFGrab(fname, offset);
}

BootUtils::~BootUtils()
{
	CloseBFile();
	if(kernel_data.dtbsz>0)delete kernel_data.dtb;
	if(kernel_data.gzsz>0)delete kernel_data.gz;
	FreeInPage(SECONDFS_BLK);
	FreeInPage(RAMFS_BLK);
	FreeInPage(KERNEL_BLK);
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
	PhysOS = offset;
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
			kernel.pcount = (boot_h.kernel_size + boot_h.page_size - 1) / boot_h.page_size;
			root_fs.pcount = (boot_h.ramdisk_size + boot_h.page_size - 1) / boot_h.page_size;
			second_part.pcount = (boot_h.second_size + boot_h.page_size - 1) / boot_h.page_size;
			dbo_p_len=(boot_h.recovery_dtbo_size+boot_h.page_size-1)/boot_h.page_size;

            InjKernel(bootdump,boot_h.kernel_size);
            InjROOTFS(&bootdump[kernel.pcount * boot_h.page_size],boot_h.ramdisk_size);
            if (boot_h.second_size > 0){
            InjXZ401(&bootdump[kernel.pcount * boot_h.page_size+root_fs.pcount * boot_h.page_size],boot_h.second_size);
            }
            if(boot_h.recovery_dtbo_size>0){
            InjDbo(&bootdump[kernel.pcount * boot_h.page_size+root_fs.pcount * boot_h.page_size+second_part.pcount * boot_h.page_size],boot_h.recovery_dtbo_size);
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

		CollectKernelData();
		GluedPage(KERNEL_BLK);
		GluedPage(RAMFS_BLK);
		GluedPage(SECONDFS_BLK);

		int bootlen = PhysOS + wbh->page_size; //+1page to config

		bootlen += kernel.pcount * wbh->page_size; //+kernel

		bootlen += root_fs.pcount * wbh->page_size; //+rootfs

		bootlen += second_part.pcount * wbh->page_size; //Second stage

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
				memcpy(&FullBoot[(kernel.pcount + 1) * wbh->page_size + PhysOS],
				       rootfs_block_lnk, wbh->ramdisk_size);
				if (second_part.pcount > 0)
					memcpy(&FullBoot[(kernel.pcount + root_fs.pcount + 1) * wbh->page_size + PhysOS],
					       unk_xzblk_lnk, wbh->second_size);

				if(wbh->header_version==1){
					memcpy(&FullBoot[ PhysOS+sizeof (boot_img_hdr)], &((char *)wbh)[sizeof (boot_img_hdr)], sizeof(droid_boot_header)-sizeof (boot_img_hdr));
                    if(dbo_p_len>0)
                    	memcpy(&FullBoot[(kernel.pcount + root_fs.pcount+second_part.pcount + 1) * boot_h.page_size + PhysOS],
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

void BootUtils::SetPageLoader(int BLK,int imglen,char *logo,int lsize){
	MTKBL mkloder;
	memset(&mkloder,0xff,sizeof(MTKBL));
	*(unsigned int *)(mkloder.magic)=0x58881688;
	mkloder.imagesize=imglen;
	memset(mkloder.logo,0x00,32);
    strcpy(mkloder.logo,logo);
    memset(mkloder.data,0xff,0x1d8);

	MPBLK *dest=&root_fs;
    if(BLK==KERNEL_BLK)dest=&kernel;
    else if(BLK==SECONDFS_BLK)dest=&second_part;

	if(dest->llen>0) delete dest->Loader;
	dest->llen=lsize;
    dest->Loader=new char[dest->llen];
    memcpy(dest->Loader,&mkloder,dest->llen);
}

void BootUtils::SetPageLoader(int BLK,void *dump,int len){
	if(!dump||len<1)return;
	MPBLK *dest=&root_fs;
    if(BLK==KERNEL_BLK)dest=&kernel;
    else if(BLK==SECONDFS_BLK)dest=&second_part;

	if(dest->llen>0) delete dest->Loader;
	dest->llen=len;
    dest->Loader=new char[len];
    memcpy(dest->Loader,dump,len);
}

void BootUtils::SetPageData(int BLK,void *dump,int len){
	if(!dump||len<1)return;
	MPBLK *dest=&root_fs;
    if(BLK==KERNEL_BLK)dest=&kernel;
    else if(BLK==SECONDFS_BLK)dest=&second_part;

	if(dest->dlen>0) delete dest->data;
	dest->dlen=len;
    dest->data=new char[len];
    memcpy(dest->data,dump,len);
}

int BootUtils::SetPageLoader(int BLK,char *fname)
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
			SetPageLoader(BLK,data, datalen);
		delete data;
		fclose(fh);
		return 1;
	}
	else
		return 0;
}

int BootUtils::SetPageData(int BLK,char *fname)
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
			SetPageData(BLK,data, datalen);
		delete data;
		fclose(fh);
		return 1;
	}
	else
		return 0;
}


void BootUtils::SetDTBList(void *dump, int len){
	if(!dump||len<1)return;
	if(kernel_data.dtbsz>0)delete kernel_data.dtb;
	kernel_data.dtbsz=len;
	kernel_data.dtb=new char[len];
	memcpy(kernel_data.dtb,dump,len);
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
	if (len)
		*len=kernel_data.dtbsz;
	if (kernel_data.dtbsz == 0)return NULL;

	return kernel_data.dtb;
}

void BootUtils::SetKernelGz(void *dump,int len){
	if(!dump||len<1)return;
	if(kernel_data.gzsz>0)delete kernel_data.gz;
	kernel_data.gzsz=len;
	kernel_data.gz=new char[len];
	memcpy(kernel_data.gz,dump,len);
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
if (len) *len=kernel_data.gzsz;
if (kernel_data.gzsz == 0)return NULL;

return kernel_data.gz;
}

int BootUtils::SpilitPage(int BLK,void *blk,int len){
	int retval=0;
	char *sig=(char *)blk,*b=(char *)blk;
    MPBLK *dest=(BLK==KERNEL_BLK)?&kernel:(BLK==RAMFS_BLK)?&root_fs:&second_part;

    if(BLK==RAMFS_BLK){
	    if((*(unsigned short *)(sig)==0x8B1F) && (*(sig+2)==0x08)){	//gzip сигнатура
				if(dest->Loader){delete dest->Loader;}
					dest->Loader=NULL;
					dest->llen=0;
				if(dest->data){delete dest->data;}
					dest->data=new char[len];
					memcpy(dest->data,b,len);
					dest->dlen=len;
					retval+=1;
    }else{														//Какой-то лоадер
    			if(dest->Loader){delete dest->Loader;}
					dest->Loader=new char[0x200];
					dest->llen=0x200;
					memcpy(dest->Loader,b,0x200);
					retval+=1;
				if(dest->data){delete dest->data;}
					dest->data=new char[len-0x200];
					dest->dlen=len-0x200;
					memcpy(dest->data,&b[0x200],len-0x200);
					retval+=1;
    }
    }else{											//Kernel && Second_part
        if((*(unsigned int *)(sig)==0x58881688)){
	        if(dest->Loader){delete dest->Loader;}
					dest->Loader=new char[0x200];
					dest->llen=0x200;
					memcpy(dest->Loader,b,0x200);
					retval+=1;
				if(dest->data){delete dest->data;}
					dest->data=new char[len-0x200];
					dest->dlen=len-0x200;
					memcpy(dest->data,&b[0x200],len-0x200);
					retval+=1;
        }else{
        	if(dest->Loader){delete dest->Loader;}
					dest->Loader=NULL;
					dest->llen=0;
				if(dest->data){delete dest->data;}
					dest->data=new char[len];
					memcpy(dest->data,b,len);
					dest->dlen=len;
					retval+=1;
        }
    }
return retval;
}

void *BootUtils::GluedPage(int BLK){
	MPBLK *dest=&root_fs;
	char *block=NULL;
    if(BLK==KERNEL_BLK)dest=&kernel;
    else if(BLK==SECONDFS_BLK)dest=&second_part;

    if(wbh){
	    dest->pcount=(dest->dlen+dest->llen+wbh->page_size-1)/wbh->page_size;
	    if(dest->pcount>1){
	    block=new char[dest->pcount*wbh->page_size];
	    //MTK yobik patch
	    if(dest->llen>0){
	    		*(unsigned int *)(&dest->Loader[4])=dest->dlen;
	    		memcpy(block,dest->Loader,dest->llen);
	    }
	    if(dest->dlen>0)memcpy(&block[dest->llen],dest->data,dest->dlen);
	    ZeroMem(&block[dest->llen+dest->dlen],dest->pcount*wbh->page_size-(dest->llen+dest->dlen));
        if(BLK==KERNEL_BLK){
        		wbh->kernel_size=dest->llen+dest->dlen;
			if(kernel_block_lnk)delete kernel_block_lnk;
			kernel_block_lnk=block;
        }else if(BLK==SECONDFS_BLK){
        	wbh->second_size=dest->llen+dest->dlen;
        	if(unk_xzblk_lnk)delete unk_xzblk_lnk;
			unk_xzblk_lnk=block;
        }else{
        	wbh->ramdisk_size=dest->llen+dest->dlen;
        	if(rootfs_block_lnk)delete rootfs_block_lnk;
        	rootfs_block_lnk=block;
        }
	    }
    }
    return block;
}

void BootUtils::FreeInPage(int BLK){
	MPBLK *dest=&root_fs;
    if(BLK==KERNEL_BLK)dest=&kernel;
    else if(BLK==SECONDFS_BLK)dest=&second_part;
    if(dest->dlen>0)delete dest->data;
    if(dest->llen>0)delete dest->Loader;
    ZeroMem(dest,sizeof(MPBLK));
}

void *BootUtils::GetPageLoader(int BLK,int *len){
	MPBLK *dest=&root_fs;
    if(BLK==KERNEL_BLK)dest=&kernel;
    else if(BLK==SECONDFS_BLK)dest=&second_part;
    if(len)*len=dest->llen;
    return dest->Loader;
}
void *BootUtils::GetPageData(int BLK,int *len){
	MPBLK *dest=&root_fs;
    if(BLK==KERNEL_BLK)dest=&kernel;
    else if(BLK==SECONDFS_BLK)dest=&second_part;
    if(len)*len=dest->dlen;
    return dest->data;
}


int BootUtils::UnCollectKernelData(){
	int retval=0,KernelGz=0,i=kernel.dlen;
	char *sig=(char *)kernel.data;

    while(i!=0){
	if(*(unsigned int *)(sig)==0xEDFE0DD0)break;					//DTB сигнатура
    i--;
    sig++;
    KernelGz++;
    }
    if(KernelGz>0){
    		retval+=1;
    SetKernelGz(kernel.data,KernelGz);
    }


    if(kernel.dlen-KernelGz>0){
    		retval+=1;
    SetDTBList(sig,kernel.dlen-KernelGz);
    }


	return retval;
}

int BootUtils::CollectKernelData(){
char *kdata=NULL;
int ret=0;

if((kdata=new char[kernel_data.gzsz+kernel_data.dtbsz])!=NULL){
	if(kernel_data.gzsz>0){
		memcpy(kdata,kernel_data.gz,kernel_data.gzsz);
		ret++;
	}
	if(kernel_data.dtbsz>0){
		memcpy(&kdata[kernel_data.gzsz],kernel_data.dtb,kernel_data.dtbsz);
		ret++;
	}

	if(kernel.dlen>0)delete kernel.data;

	kernel.data=kdata;
	kernel.dlen=kernel_data.gzsz+kernel_data.dtbsz;
}

return ret;
}


int BootUtils::InjKernel(void *dump, int len)
{
	if(!dump||len<1)return 0;

	wbh->kernel_size = len;

	SpilitPage(KERNEL_BLK,dump,len);
	//Сразу разбираем
	UnCollectKernelData();

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

	SpilitPage(RAMFS_BLK,dump,len);

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

	SpilitPage(SECONDFS_BLK,dump,len);

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

void *BootUtils::GetCurKernel(int *len){
	if (wbh == NULL)return NULL;

	CollectKernelData();
	GluedPage(KERNEL_BLK);
	if (len)
		*len = wbh->kernel_size;
	return kernel_block_lnk;
}

void *BootUtils::GetCurROOTFS(int *len){
	if (wbh == NULL)return NULL;

		GluedPage(RAMFS_BLK);
	if (len)
		*len = wbh->ramdisk_size;
	return rootfs_block_lnk;
}

void *BootUtils::GetCurXZ401(int *len){
	if (wbh == NULL)return NULL;

		GluedPage(SECONDFS_BLK);
	if (len)
		*len = wbh->second_size;
	return unk_xzblk_lnk;
}

void *BootUtils::GetDbo(int *len){
	if(wbh==NULL)return NULL;

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
uint32_t kernel=boot_h.kernel_size,
		ramfs=boot_h.ramdisk_size,
		secondfs=boot_h.second_size,
		dtbo=boot_h.recovery_dtbo_size,
		header=boot_h.header_size;
	if (conf)
	{
		wbh = &boot_h;
		memcpy(wbh, conf, sizeof (droid_boot_header));
        wbh->header_size=header;
        wbh->kernel_size=kernel;
        wbh->ramdisk_size=ramfs;
        wbh->recovery_dtbo_size=dtbo;
        wbh->second_size=secondfs;
	}
}
