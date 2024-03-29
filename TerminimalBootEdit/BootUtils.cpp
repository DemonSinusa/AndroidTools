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

void ZeroMem(void *ptr, int sz) {
	memset(ptr, zeroid, sz);
}

BootUtils::BootUtils(UfNtype *fname, int offset) {
	boot = NULL;
	static_flen = 0;
	Bimgname = new UfNtype[strlen(fname) + 1];
	strcpy(Bimgname, fname);
	memset(&boot_h, 0x00, sizeof (onegen_max_bootheader));
	memset(&ctx, 0x00, sizeof (SHA_CTX));

	ZeroMem(&kernel,sizeof(MPBLK));
	ZeroMem(&kernel_data,sizeof(Kdata));

	ZeroMem(&root_fs,sizeof(MPBLK));
	ZeroMem(&second_part,sizeof(MPBLK));

	OpenBFGrab(fname, offset);
}

BootUtils::~BootUtils() {
	CloseBFile();
	if (kernel_data.dtbsz>0)delete[] kernel_data.dtb;
	if (kernel_data.gzsz>0)delete[] kernel_data.gz;
	FreeInPage(SECONDFS_BLK);
	FreeInPage(RAMFS_BLK);
	FreeInPage(KERNEL_BLK);
	delete[] Bimgname;
}


int BootUtils::detect_hash_type(boot_img_hdr_v2 *hdr)
{
    // sha1 is expected to have zeroes in id[20] and higher
    // offset by 4 to accomodate bootimg variants with BOOT_NAME_SIZE 20
    uint8_t id[SHA256_DIGEST_LENGTH];
    memcpy(&id, hdr->id, sizeof(id));
    int i;
    for(i = SHA_DIGEST_LENGTH + 4; i < SHA256_DIGEST_LENGTH; ++i) {
        if(id[i]) {
            return 1;
        }
    }
    return 0;
}


void BootUtils::generate_id_sha1(void *kernel_data, void *ramdisk_data,
                      void *second_data, void *dt_data, void *recovery_dtbo_data, void *dtb_data)
{
    SHA_CTX ctx;

    SHA1_Init(&ctx);
    SHA1_Update(&ctx, kernel_data, wbh->kernel_size);
    SHA1_Update(&ctx, &wbh->kernel_size, sizeof(wbh->kernel_size));
    SHA1_Update(&ctx, ramdisk_data, wbh->ramdisk_size);
    SHA1_Update(&ctx, &wbh->ramdisk_size, sizeof(wbh->ramdisk_size));
    SHA1_Update(&ctx, second_data, wbh->second_size);
    SHA1_Update(&ctx, &wbh->second_size, sizeof(wbh->second_size));
    if(dt_data) {
        SHA1_Update(&ctx, dt_data, wbh->header_version);
        SHA1_Update(&ctx, &wbh->header_version, sizeof(wbh->header_version));
    } else if(wbh->header_version > 0) {
        SHA1_Update(&ctx, recovery_dtbo_data, wbh->recovery_dtbo_size);
        SHA1_Update(&ctx, &wbh->recovery_dtbo_size, sizeof(wbh->recovery_dtbo_size));
        if(wbh->header_version > 1) {
            SHA1_Update(&ctx, dtb_data, wbh->dtb_size);
            SHA1_Update(&ctx, &wbh->dtb_size, sizeof(wbh->dtb_size));
        }
    }
    sha = SHA1_Final(hash, &ctx);
    memcpy(wbh->id, hash, SHA_DIGEST_LENGTH > sizeof(wbh->id) ? sizeof(wbh->id) : SHA_DIGEST_LENGTH);
}

void BootUtils::generate_id_sha256( void *kernel_data, void *ramdisk_data,
                        void *second_data, void *dt_data, void *recovery_dtbo_data, void *dtb_data)
{
    SHA256_CTX ctx;

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, kernel_data, wbh->kernel_size);
    SHA256_Update(&ctx, &wbh->kernel_size, sizeof(wbh->kernel_size));
    SHA256_Update(&ctx, ramdisk_data, wbh->ramdisk_size);
    SHA256_Update(&ctx, &wbh->ramdisk_size, sizeof(wbh->ramdisk_size));
    SHA256_Update(&ctx, second_data, wbh->second_size);
    SHA256_Update(&ctx, &wbh->second_size, sizeof(wbh->second_size));
    if(dt_data) {
        SHA256_Update(&ctx, dt_data, wbh->header_version);
        SHA256_Update(&ctx, &wbh->header_version, sizeof(wbh->header_version));
    } else if(wbh->header_version > 0) {
        SHA256_Update(&ctx, recovery_dtbo_data, wbh->recovery_dtbo_size);
        SHA256_Update(&ctx, &wbh->recovery_dtbo_size, sizeof(wbh->recovery_dtbo_size));
        if(wbh->header_version > 1) {
            SHA256_Update(&ctx, dtb_data, wbh->dtb_size);
            SHA256_Update(&ctx, &wbh->dtb_size, sizeof(wbh->dtb_size));
        }
    }
    sha = SHA256_Final(hash,&ctx);
    memcpy(wbh->id, hash, SHA256_DIGEST_LENGTH > sizeof(wbh->id) ? sizeof(wbh->id) : SHA256_DIGEST_LENGTH);
}

bool BootUtils::OpenBFGrab(UfNtype *fname, int offset) {

	bool retval = true;
	char *bootdump=NULL;


	if ((boot = fopen(fname, "r+b")) == NULL)
		return false;
	fseek(boot, 0, SEEK_END);
	static_flen = ftell(boot);
	PhysOS = offset;
	fseek(boot, PhysOS, SEEK_SET);

	if ((fread(&boot_h, sizeof (boot_img_hdr), 1, boot)) == 1) {

		if (boot_h.header_version>0&&boot_h.header_version<=hdr_ver_max) {
			if(boot_h.header_version==1)
			fread(&((char *)&boot_h)[sizeof (boot_img_hdr)], sizeof(struct boot_img_hdr_v1)-sizeof (boot_img_hdr), 1, boot);
			else fread(&((char *)&boot_h)[sizeof (boot_img_hdr)], sizeof(struct boot_img_hdr_v2)-sizeof (boot_img_hdr), 1, boot);
		}
		wbh = &boot_h;
		if(boot_h.header_version<=hdr_ver_max)fprintf(stdout,"Boot header %d version.\r\n",boot_h.header_version);

		//Выясним что за SHA type
		sha_type=detect_hash_type(wbh);

		if (static_flen > boot_h.page_size) {

			bootdump=new char[static_flen-(boot_h.page_size + PhysOS)];
			fseek(boot, boot_h.page_size + PhysOS, SEEK_SET);
			fread(bootdump, static_flen-(boot_h.page_size + PhysOS), 1, boot);

			//Сгенерируем размеры блоков
			kernel.pcount = (boot_h.kernel_size + boot_h.page_size - 1) / boot_h.page_size;
			root_fs.pcount = (boot_h.ramdisk_size + boot_h.page_size - 1) / boot_h.page_size;
			second_part.pcount = (boot_h.second_size + boot_h.page_size - 1) / boot_h.page_size;

			if(boot_h.header_version>hdr_ver_max)dt_p_len=(boot_h.header_version+boot_h.page_size-1)/boot_h.page_size;
			else{
			dtbo_p_len=(boot_h.recovery_dtbo_size+boot_h.page_size-1)/boot_h.page_size;
			dtb_p_len=(boot_h.dtb_size+boot_h.page_size-1)/boot_h.page_size;
			}

			PhysElse=(kernel.pcount+root_fs.pcount+second_part.pcount+dt_p_len+dtbo_p_len+dtb_p_len+1)*boot_h.page_size+PhysOS;

			InjKernel(bootdump,boot_h.kernel_size);
			InjROOTFS(&bootdump[kernel.pcount * boot_h.page_size],boot_h.ramdisk_size);
			if (boot_h.second_size > 0) {
				InjXZ401(&bootdump[(kernel.pcount+root_fs.pcount)*boot_h.page_size],boot_h.second_size);
			}
			if(boot_h.header_version>hdr_ver_max)InjDTree(&bootdump[(kernel.pcount+root_fs.pcount+second_part.pcount)*boot_h.page_size],boot_h.header_version);
			else{
					if (boot_h.recovery_dtbo_size>0) {
				InjDtbo(&bootdump[(kernel.pcount+root_fs.pcount+second_part.pcount)*boot_h.page_size],boot_h.recovery_dtbo_size);
			}
			if(boot_h.dtb_size>0){
				InjDtb(&bootdump[(kernel.pcount+root_fs.pcount+second_part.pcount+dtbo_p_len)*boot_h.page_size],boot_h.dtb_size);
			}
			}
			//PhysElse=ftell(boot);
			if(static_flen-PhysElse>0){
				InjAnythink(&bootdump[PhysElse-boot_h.page_size-PhysOS],static_flen-PhysElse);
			}

			delete[] bootdump;
		} else
			retval = false;
	} else
		retval = false;
	fclose(boot);
	boot = NULL;
	return retval;
}


void BootUtils::CloseBFile() {
//	int p = 0;
	if (wbh) {

		CollectKernelData();
		GluedPage(KERNEL_BLK);
		GluedPage(RAMFS_BLK);
		GluedPage(SECONDFS_BLK);

		int bootlen = PhysOS + wbh->page_size; //+1page to config

		bootlen += kernel.pcount * wbh->page_size; //+kernel

		bootlen += root_fs.pcount * wbh->page_size; //+rootfs

		bootlen += second_part.pcount * wbh->page_size; //Second stage

		bootlen += makeweight_part.dlen;				//Остатки для конца

		if(wbh->header_version>hdr_ver_max)bootlen+=dt_p_len*wbh->page_size;
		else{
				bootlen+=dtbo_p_len*wbh->page_size;
				bootlen+=dtb_p_len*wbh->page_size;
		}
		static_flen = bootlen;


		if (static_flen > PhysOS) {
			if ((boot = fopen(Bimgname, "wt")) != NULL) {
				FullBoot = new char[static_flen];

				//0xFF?>:)
				ZeroMem(FullBoot, static_flen);
				//0x0
				ZeroMem(wbh->id, sizeof (wbh->id));

				memcpy(wbh->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

				if(!sha_type)generate_id_sha1(kernel_block_lnk,rootfs_block_lnk,unk_xzblk_lnk,dt_block_lnk,dtbo_block_lnk,dtb_block_lnk);
				else generate_id_sha256(kernel_block_lnk,rootfs_block_lnk,unk_xzblk_lnk,dt_block_lnk,dtbo_block_lnk,dtb_block_lnk);

				memcpy(&FullBoot[ PhysOS], wbh, sizeof (boot_img_hdr));

				memcpy(&FullBoot[wbh->page_size + PhysOS], kernel_block_lnk,
					   wbh->kernel_size);
				memcpy(&FullBoot[(kernel.pcount + 1) * wbh->page_size + PhysOS],
					   rootfs_block_lnk, wbh->ramdisk_size);
				if (second_part.pcount > 0)
					memcpy(&FullBoot[(kernel.pcount + root_fs.pcount + 1) * wbh->page_size + PhysOS],
						   unk_xzblk_lnk, wbh->second_size);

//Поправки на версии бутов
				if(wbh->header_version>hdr_ver_max){
						memcpy(&FullBoot[(kernel.pcount + root_fs.pcount+second_part.pcount + 1) * boot_h.page_size + PhysOS],
							   dt_block_lnk, wbh->header_version);
				}else{
				if (wbh->header_version==1) {
					memcpy(&FullBoot[ PhysOS+sizeof (boot_img_hdr)], &((char *)wbh)[sizeof (boot_img_hdr)], sizeof(struct boot_img_hdr_v1)-sizeof (boot_img_hdr));
					if (dtbo_p_len>0)
						memcpy(&FullBoot[(kernel.pcount + root_fs.pcount+second_part.pcount + 1) * boot_h.page_size + PhysOS],
							   dtbo_block_lnk, wbh->recovery_dtbo_size);
				}else{
//Версия 2 из первого поколения
					memcpy(&FullBoot[ PhysOS+sizeof (boot_img_hdr)], &((char *)wbh)[sizeof (boot_img_hdr)], sizeof(onegen_max_bootheader)-sizeof (boot_img_hdr));
					if (dtbo_p_len>0)
						memcpy(&FullBoot[(kernel.pcount + root_fs.pcount+second_part.pcount + 1) * boot_h.page_size + PhysOS],
							   dtbo_block_lnk, wbh->recovery_dtbo_size);
					if(dtb_p_len>0)
						memcpy(&FullBoot[(kernel.pcount+root_fs.pcount+second_part.pcount+dtbo_p_len+1)*boot_h.page_size+PhysOS],
									dtb_block_lnk,wbh->dtb_size);
				}
				}
//-------------------------
			if (makeweight_part.dlen > 0)
					memcpy(&FullBoot[(kernel.pcount + root_fs.pcount+second_part.pcount +dt_p_len+dtbo_p_len+dtb_p_len+ 1) * wbh->page_size + PhysOS],
						   makeweight_lnk, makeweight_part.dlen);
//=========================
				fseek(boot, 0, SEEK_SET);
				fwrite(FullBoot, static_flen, 1, boot);

				//---++++++++++++++++++++++=)
				fclose(boot);
				delete[] FullBoot;
			}
		}

		if (kernel_block_lnk)
			delete[] kernel_block_lnk;
		if (rootfs_block_lnk)
			delete[] rootfs_block_lnk;
		if (unk_xzblk_lnk)
			delete[] unk_xzblk_lnk;
		if (dtbo_block_lnk)
			delete[] dtbo_block_lnk;
		if(dtb_block_lnk)
			delete[] dtb_block_lnk;
		if(makeweight_lnk)
			delete[] makeweight_lnk;
	}
}

void BootUtils::SetPageLoader(int BLK,int imglen,char *logo,int lsize) {
	MTKBL mkloder;
	memset(&mkloder,0xff,sizeof(MTKBL));
	*(unsigned int *)(mkloder.magic)=0x58881688;
	mkloder.imagesize=imglen;
	memset(mkloder.logo,0x00,32);
	strcpy(mkloder.logo,logo);
	memset(mkloder.data,0xff,0x1d8);

	MPBLK *dest=&root_fs;
	if (BLK==KERNEL_BLK)dest=&kernel;
	else if (BLK==SECONDFS_BLK)dest=&second_part;

	if (dest->llen>0) delete[] dest->Loader;
	dest->llen=lsize;
	dest->Loader=new char[dest->llen];
	memcpy(dest->Loader,&mkloder,dest->llen);
}

void BootUtils::SetPageLoader(int BLK,void *dump,int len) {
	if (!dump||len<1)return;
	MPBLK *dest=&root_fs;
	if (BLK==KERNEL_BLK)dest=&kernel;
	else if (BLK==SECONDFS_BLK)dest=&second_part;

	if (dest->llen>0) delete[] dest->Loader;
	dest->llen=len;
	dest->Loader=new char[len];
	memcpy(dest->Loader,dump,len);
}

void BootUtils::SetPageData(int BLK,void *dump,int len) {
	if (!dump||len<1)return;
	MPBLK *dest=&root_fs;
	if (BLK==KERNEL_BLK)dest=&kernel;
	else if (BLK==SECONDFS_BLK)dest=&second_part;

	if (dest->dlen>0) delete[] dest->data;
	dest->dlen=len;
	dest->data=new char[len];
	memcpy(dest->data,dump,len);
}

int BootUtils::SetPageLoader(int BLK,char *fname) {
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			SetPageLoader(BLK,data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}

int BootUtils::SetPageData(int BLK,char *fname) {
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			SetPageData(BLK,data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}


void BootUtils::SetDTBList(void *dump, int len) {
	if (!dump||len<1)return;
	if (kernel_data.dtbsz>0)delete[] kernel_data.dtb;
	kernel_data.dtbsz=len;
	kernel_data.dtb=new char[len];
	memcpy(kernel_data.dtb,dump,len);
}
int BootUtils::SetDTBList(char *fname) {
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			SetDTBList(data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}

void *BootUtils::GetDTBList(int *len) {
	if (len)
		*len=kernel_data.dtbsz;
	if (kernel_data.dtbsz == 0)return NULL;

	return kernel_data.dtb;
}

void BootUtils::SetKernelGz(void *dump,int len) {
	if (!dump||len<1)return;
	if (kernel_data.gzsz>0)delete[] kernel_data.gz;
	kernel_data.gzsz=len;
	kernel_data.gz=new char[len];
	memcpy(kernel_data.gz,dump,len);
}
int BootUtils::SetKernelGz(char *fname) {
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			SetKernelGz(data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}
void *BootUtils::GetKernelGz(int *len) {
	if (len) *len=kernel_data.gzsz;
	if (kernel_data.gzsz == 0)return NULL;

	return kernel_data.gz;
}

int BootUtils::SpilitPage(int BLK,void *blk,int len) {
	int retval=0;
	char *sig=(char *)blk,*b=(char *)blk;
	MPBLK *dest=(BLK==KERNEL_BLK)?&kernel:(BLK==RAMFS_BLK)?&root_fs:&second_part;

	if (BLK==RAMFS_BLK) {
		if ((*(unsigned short *)(sig)==0x8B1F) && (*(sig+2)==0x08)) {	//gzip сигнатура
			if (dest->Loader) {
				delete[] dest->Loader;
			}
			dest->Loader=NULL;
			dest->llen=0;
			if (dest->data) {
				delete[] dest->data;
			}
			dest->data=new char[len];
			memcpy(dest->data,b,len);
			dest->dlen=len;
			retval+=1;
		} else {														//Какой-то лоадер
			if (dest->Loader) {
				delete[] dest->Loader;
			}
			dest->Loader=new char[0x200];
			dest->llen=0x200;
			memcpy(dest->Loader,b,0x200);
			retval+=1;
			if (dest->data) {
				delete[] dest->data;
			}
			dest->data=new char[len-0x200];
			dest->dlen=len-0x200;
			memcpy(dest->data,&b[0x200],len-0x200);
			retval+=1;
		}
	} else {											//Kernel && Second_part
		if ((*(unsigned int *)(sig)==0x58881688)) {
			if (dest->Loader) {
				delete[] dest->Loader;
			}
			dest->Loader=new char[0x200];
			dest->llen=0x200;
			memcpy(dest->Loader,b,0x200);
			retval+=1;
			if (dest->data) {
				delete[] dest->data;
			}
			dest->data=new char[len-0x200];
			dest->dlen=len-0x200;
			memcpy(dest->data,&b[0x200],len-0x200);
			retval+=1;
		} else {
			if (dest->Loader) {
				delete[] dest->Loader;
			}
			dest->Loader=NULL;
			dest->llen=0;
			if (dest->data) {
				delete[] dest->data;
			}
			dest->data=new char[len];
			memcpy(dest->data,b,len);
			dest->dlen=len;
			retval+=1;
		}
	}
	return retval;
}

void *BootUtils::GluedPage(int BLK) {
	MPBLK *dest=&root_fs;
	char *block=NULL;
	if (BLK==KERNEL_BLK)dest=&kernel;
	else if (BLK==SECONDFS_BLK)dest=&second_part;

	if (wbh) {
		dest->pcount=(dest->dlen+dest->llen+wbh->page_size-1)/wbh->page_size;
		if (dest->pcount>1) {
			block=new char[dest->pcount*wbh->page_size];
			//MTK yobik patch
			if (dest->llen>0) {
				*(unsigned int *)(&dest->Loader[4])=dest->dlen;
				memcpy(block,dest->Loader,dest->llen);
			}
			if (dest->dlen>0)memcpy(&block[dest->llen],dest->data,dest->dlen);
			ZeroMem(&block[dest->llen+dest->dlen],dest->pcount*wbh->page_size-(dest->llen+dest->dlen));
			if (BLK==KERNEL_BLK) {
				wbh->kernel_size=dest->llen+dest->dlen;
				if (kernel_block_lnk)delete[] kernel_block_lnk;
				kernel_block_lnk=block;
			} else if (BLK==SECONDFS_BLK) {
				wbh->second_size=dest->llen+dest->dlen;
				if (unk_xzblk_lnk)delete[] unk_xzblk_lnk;
				unk_xzblk_lnk=block;
			} else{
				wbh->ramdisk_size=dest->llen+dest->dlen;
				if (rootfs_block_lnk)delete[] rootfs_block_lnk;
				rootfs_block_lnk=block;
			}
		}
	}
	return block;
}

void BootUtils::FreeInPage(int BLK) {
	MPBLK *dest=&root_fs;
	if (BLK==KERNEL_BLK)dest=&kernel;
	else if (BLK==SECONDFS_BLK)dest=&second_part;
	if (dest->dlen>0)delete[] dest->data;
	if (dest->llen>0)delete[] dest->Loader;
	ZeroMem(dest,sizeof(MPBLK));
}

void *BootUtils::GetPageLoader(int BLK,int *len) {
	MPBLK *dest=&root_fs;
	if (BLK==KERNEL_BLK)dest=&kernel;
	else if (BLK==SECONDFS_BLK)dest=&second_part;
	if (len)*len=dest->llen;
	return dest->Loader;
}
void *BootUtils::GetPageData(int BLK,int *len) {
	MPBLK *dest=&root_fs;
	if (BLK==KERNEL_BLK)dest=&kernel;
	else if (BLK==SECONDFS_BLK)dest=&second_part;
	if (len)*len=dest->dlen;
	return dest->data;
}


int BootUtils::UnCollectKernelData() {
	int retval=0,KernelGz=0,i=kernel.dlen;
	char *sig=(char *)kernel.data;

	while (i!=0) {
		if (*(unsigned int *)(sig)==0xEDFE0DD0)break;					//DTB сигнатура
		i--;
		sig++;
		KernelGz++;
	}
	if (KernelGz>0) {
		retval+=1;
		SetKernelGz(kernel.data,KernelGz);
	}


	if (kernel.dlen-KernelGz>0) {
		retval+=1;
		SetDTBList(sig,kernel.dlen-KernelGz);
	}


	return retval;
}

int BootUtils::CollectKernelData() {
	char *kdata=NULL;
	int ret=0;

	if ((kdata=new char[kernel_data.gzsz+kernel_data.dtbsz])!=NULL) {
		if (kernel_data.gzsz>0) {
			memcpy(kdata,kernel_data.gz,kernel_data.gzsz);
			ret++;
		}
		if (kernel_data.dtbsz>0) {
			memcpy(&kdata[kernel_data.gzsz],kernel_data.dtb,kernel_data.dtbsz);
			ret++;
		}

		if (kernel.dlen>0)delete[] kernel.data;

		kernel.data=kdata;
		kernel.dlen=kernel_data.gzsz+kernel_data.dtbsz;
	}

	return ret;
}


int BootUtils::InjKernel(void *dump, int len) {
	if (!dump||len<1)return 0;

	wbh->kernel_size = len;

	SpilitPage(KERNEL_BLK,dump,len);
	//Сразу разбираем
	UnCollectKernelData();

	return 1;
}

int BootUtils::InjKernel(char *fname) {
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			InjKernel(data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}

int BootUtils::InjROOTFS(void *dump, int len) {
	if (!dump||len<1)return 0;

	wbh->ramdisk_size = len;

	SpilitPage(RAMFS_BLK,dump,len);

	return 1;
}

int BootUtils::InjROOTFS(char *fname) {
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			InjROOTFS(data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}

int BootUtils::InjXZ401(void *dump, int len) {
	if (!dump||len<1)return 0;

	wbh->second_size = len;

	SpilitPage(SECONDFS_BLK,dump,len);

	return 1;
}

int BootUtils::InjXZ401(char *fname) {
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			InjXZ401(data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}

int BootUtils::InjDTree(void *dump,int len){
	if (!dump||len<1)return 0;

	wbh->header_version=len;

	dt_p_len=(len + boot_h.page_size - 1) / boot_h.page_size;
	if (dt_block_lnk!=NULL)
		delete[] dt_block_lnk;
	dt_block_lnk=new char[len];
//    ZeroMem(dbo_block_lnk,len);
	memcpy(dt_block_lnk,dump,len);
	return 1;
}

int BootUtils::InjDTree(char *fname){
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			InjDTree(data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}

int BootUtils::InjDtbo(void *dump,int len) {
	if (!dump||len<1)return 0;

	wbh->recovery_dtbo_size=len;

	dtbo_p_len=(len + boot_h.page_size - 1) / boot_h.page_size;
	if (dtbo_block_lnk!=NULL)
		delete[] dtbo_block_lnk;
	dtbo_block_lnk=new char[len];
//    ZeroMem(dbo_block_lnk,len);
	memcpy(dtbo_block_lnk,dump,len);
	return 1;
}

int BootUtils::InjDtbo(char *fname) {
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			InjDtbo(data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}

int BootUtils::InjDtb(void *dump,int len){
	if (!dump||len<1)return 0;
	wbh->dtb_size=len;

	dtb_p_len=(len + boot_h.page_size - 1) / boot_h.page_size;
	if(dtb_block_lnk!=NULL)delete[] dtb_block_lnk;
	dtb_block_lnk=new char[len];
	memcpy(dtb_block_lnk,dump,len);
	return 1;
}

int BootUtils::InjDtb(char *fname){
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			InjDtb(data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}

int BootUtils::InjAnythink(void *dump,int len){
	if (!dump||len<1)return 0;
	makeweight_part.dlen=len;

//#warning "Здесь нужно расположить отделятор говна от примесей."
	if(makeweight_lnk!=NULL)delete[] makeweight_lnk;
	makeweight_lnk=new char[len];
	memcpy(makeweight_lnk,dump,len);
	return 1;
}
int BootUtils::InjAnythink(char *fname){
	FILE *fh=NULL;
	char *data=NULL;
	int datalen=0;
	if ((fh = fopen(fname, "rb")) != NULL) {
		fseek(fh, 0, SEEK_END);
		datalen = ftell(fh);
		data = new char[datalen];
		fseek(fh, 0, SEEK_SET);
		if (fread(data, datalen, 1, fh)==1)
			InjAnythink(data, datalen);
		delete[] data;
		fclose(fh);
		return 1;
	} else
		return 0;
}

void *BootUtils::GetCurKernel(int *len) {
	if (wbh == NULL)return NULL;

	CollectKernelData();
	GluedPage(KERNEL_BLK);
	if (len)
		*len = wbh->kernel_size;
	return kernel_block_lnk;
}

void *BootUtils::GetCurROOTFS(int *len) {
	if (wbh == NULL)return NULL;

	GluedPage(RAMFS_BLK);
	if (len)
		*len = wbh->ramdisk_size;
	return rootfs_block_lnk;
}

void *BootUtils::GetCurXZ401(int *len) {
	if (wbh == NULL)return NULL;

	GluedPage(SECONDFS_BLK);
	if (len)
		*len = wbh->second_size;
	return unk_xzblk_lnk;
}

void *BootUtils::GetDTree(int *len){
	if (wbh==NULL)return NULL;

	if (len&&wbh->header_version>hdr_ver_max){
		*len=wbh->header_version;
		dt_p_len=(wbh->header_version + boot_h.page_size - 1) / boot_h.page_size;
	}else{
		*len=0;
		 return NULL;
	}
	return dt_block_lnk;
}

void *BootUtils::GetDtbo(int *len) {
	if (wbh==NULL)return NULL;

	if (len)
		*len=wbh->recovery_dtbo_size;
	dtbo_p_len=(wbh->recovery_dtbo_size + boot_h.page_size - 1) / boot_h.page_size;
	return dtbo_block_lnk;
}

void *BootUtils::GetDtb(int *len){
	if(wbh==NULL)return NULL;
	if(len)
		*len=wbh->dtb_size;
	dtb_p_len=(wbh->dtb_size+boot_h.page_size-1)/boot_h.page_size;
	return dtb_block_lnk;
}

void *BootUtils::GetMakeweight(int *len){
	if (len)
		*len=makeweight_part.dlen;

	return makeweight_lnk;
}

onegen_max_bootheader *BootUtils::GetCurMainConfig() {
	return wbh;
}

void BootUtils::SetCurMainConfig(onegen_max_bootheader *conf) {
	uint32_t kernel=boot_h.kernel_size,
			 ramfs=boot_h.ramdisk_size,
			 secondfs=boot_h.second_size,
			 dtbo=boot_h.recovery_dtbo_size,
			 header=boot_h.header_size,
			 dtb=boot_h.dtb_size,
			 ver_dt=boot_h.header_version;

	if (conf) {
		wbh = &boot_h;
		memcpy(wbh, conf, sizeof (onegen_max_bootheader));
		wbh->header_size=header;
		wbh->kernel_size=kernel;
		wbh->ramdisk_size=ramfs;
		wbh->recovery_dtbo_size=dtbo;
		wbh->second_size=secondfs;
		wbh->dtb_size=dtb;
		if(ver_dt>hdr_ver_max)
			wbh->header_version=ver_dt;

	}
}
