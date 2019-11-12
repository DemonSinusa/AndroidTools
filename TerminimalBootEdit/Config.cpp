/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Config.cpp
 * Author: senjord
 *
 * Created on 20 мая 2018 г., 13:56
 */
//#include <cstdlib>

//using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <bootimg.h>
#include "BootUtils.h"
#include "Config.h"

const char *names[] =
{
	L_PROP_KERNEL_ADDR,
	L_PROP_KERNELTAG_ADDR,
	L_PROP_ROOTFS_ADDR,
	L_PROP_SECONDFS_ADDR,
	L_PROP_DBO_ADDR,
	L_PROP_PAGE_SIZE,
	L_PROP_DBOHEADER_SIZE,
	L_MIX_OS_VERSION,
	L_PROP_NAME,
	L_PROP_CMDLINE,
	L_PROP_CMDLINE_EX,
	L_PROP_RAMDISK_GZ_LVL,

	L_ID0,
	L_ID1,
	L_ID2,
	L_ID3,
	L_ID4,
	L_ID5,
	L_ID6,
	L_ID7
};

OPT options[NUM_PROPS];
//return numder in DB or -1;

void Config::SetType(OPT *item, int i)
{
	if (i == PROP_KERNEL_ADDR || i == PROP_KERNELTAG_ADDR ||
	        i == PROP_ROOTFS_ADDR || i == PROP_SECONDFS_ADDR || i==PROP_DBO_ADDR)
		item->datatype = TYPE_HEX;
	else if (i == MIX_OS_VERSION || i == PROP_NAME || i == PROP_CMDLINE || i == PROP_CMDLINE_EX)
		item->datatype = TYPE_TXT;
	else
		item->datatype = TYPE_INT;
}

short Config::GetType(int id){
	OPT titem;
	if(id<NUM_PROPS){
	SetType(&titem,id);
	}else titem.datatype=-1;
	return titem.datatype;
}

int Config::SetParm(UfNtype *str, const char *list[])
{
	UfNtype *lexem = NULL, *var = NULL;
	int len = 0, i = 0;
	var = strchr(str, '=');
	len = var - str;
	lexem = new UfNtype[len + 1];
	strncpy(lexem, str, len);
	lexem[len] = '\0';
	for (i = 0; i < NUM_PROPS; i++)
	{
		if (strstr(lexem, list[i]))
			break;
	}
	delete[] lexem;
	if (i != NUM_PROPS)
	{
		if (len > 0)
		{
			options[i].len = &str[strlen(str)]-(var + 1);
			strncpy(options[i].txtname, var + 1, MAX_PARAMETR_LEN);
			SetType(&options[i], i);
		}
		return i;
	}
	else
		return -1;
}

int Config::PackOSVersion(OSV *version)
{
	int ver=0;
	ver &= ((1 << 11) - 1);
	ver |= (((version->major & 0x7f) << 25) | ((version->minor & 0x7f) << 18) | ((version->patch & 0x7f) << 11));

	ver &= ~((1 << 11) - 1);
	ver |= (((version->year - 2000) & 0x7f) << 4) | ((version->month & 0xf) << 0);
	return ver;
}

void Config::UnpackOSVersion(OSV *version, int packed)
{
	memset(version, 0x00, sizeof (OSV));
	if (packed != 0)
	{
		int os_version, os_patch_level;
		os_version = packed >> 11;
		os_patch_level = packed & 0x7ff;

		version->major = (os_version >> 14)&0x7f;
		version->minor = (os_version >> 7)&0x7f;
		version->patch = os_version & 0x7f;

		version->year = (os_patch_level >> 4) + 2000;
		version->month = os_patch_level & 0xf;

		if ((version->major < 128) && (version->minor < 128) && (version->patch < 128) &&
		        (version->year >= 2000) && (version->year < 2128) && (version->month > 0) && (version->month <= 12))
		{
			printf("BOARD_OS_VERSION %d.%d.%d\n", version->major, version->minor, version->patch);
			printf("BOARD_OS_PATCH_LEVEL %d-%02d\n", version->year, version->month);
		}
		else
		{
			packed = 0;
		}
	}
}

Config::Config()
{
	memset(&ver, 0x00, sizeof (OSV));
	memset(&it_cfg, 0x00, sizeof (droid_boot_header));
}

Config::~Config()
{
	if (txtblk)
		free(txtblk);
	if (block)
		free(block);
}

int Config::EatBinConfig(UfNtype *file)
{
	if ((fc = fopen(file, "rb")) != NULL)
	{
		fseek(fc, 0x00, SEEK_END);
		blklen = ftell(fc);
		fseek(fc, 0x00, SEEK_SET);
		if (block)
			free(block);
		block = (char *) malloc(blklen);
		fread(block, blklen, 1, fc);
		fclose(fc);
		return EatBinConfig((void *) block, blklen);
	}
	else
		return 0;
}

int Config::EatTxtConfig(UfNtype *file)
{
	if ((fc = fopen(file, "rb")) != NULL)
	{
		fseek(fc, 0x00, SEEK_END);
		tblklen = ftell(fc);
		fseek(fc, 0x00, SEEK_SET);
		if (txtblk)
			free(txtblk);
		txtblk = (char *) malloc(tblklen);
		fread(txtblk, tblklen, 1, fc);
		fclose(fc);
		return EatTxtConfig((void *) txtblk);
	}
	else
		return 0;
}

int Config::EatBinConfig(void *data, unsigned int len)
{
	int parlen = 0;
	droid_boot_header *curboot = NULL;
	if (len >= sizeof (boot_img_hdr))
		curboot = (droid_boot_header *) data;
	else
		return 0;
	//Ручками перетаскиваем каждый параметр
	memset(options, 0x00, sizeof (OPT) * NUM_PROPS);

	sprintf(options[PROP_RAMGZ_LVL].txtname, "%d", 9);
	options[PROP_RAMGZ_LVL].len = sizeof (uint32_t);
	SetType(&options[PROP_RAMGZ_LVL], PROP_RAMGZ_LVL);

	if ((parlen = strlen((char *) curboot->cmdline)) != 0)
	{
		strncpy(options[PROP_CMDLINE].txtname, (char *) curboot->cmdline, MAX_PARAMETR_LEN);
		options[PROP_CMDLINE].len = parlen;
		SetType(&options[PROP_CMDLINE], PROP_CMDLINE);
	}
	if ((parlen = strlen((char *) curboot->extra_cmdline)) != 0)
	{
		strncpy(options[PROP_CMDLINE_EX].txtname, (char *) curboot->extra_cmdline, MAX_PARAMETR_LEN);
		options[PROP_CMDLINE_EX].len = parlen;
		SetType(&options[PROP_CMDLINE_EX], PROP_CMDLINE_EX);
	}
	if (curboot->kernel_addr > 0)
	{
		sprintf(options[PROP_KERNEL_ADDR].txtname, "0x%08x", curboot->kernel_addr);
		options[PROP_KERNEL_ADDR].len = sizeof (uint32_t);
		SetType(&options[PROP_KERNEL_ADDR], PROP_KERNEL_ADDR);
	}

	if (curboot->tags_addr > 0)
	{
		sprintf(options[PROP_KERNELTAG_ADDR].txtname, "0x%08x", curboot->tags_addr);
		options[PROP_KERNELTAG_ADDR].len = sizeof (uint32_t);
		SetType(&options[PROP_KERNELTAG_ADDR], PROP_KERNELTAG_ADDR);
	}

	if (curboot->ramdisk_addr > 0)
	{
		sprintf(options[PROP_ROOTFS_ADDR].txtname, "0x%08x", curboot->ramdisk_addr);
		options[PROP_ROOTFS_ADDR].len = sizeof (uint32_t);
		SetType(&options[PROP_ROOTFS_ADDR], PROP_ROOTFS_ADDR);
	}
	if (curboot->second_addr > 0)
	{
		sprintf(options[PROP_SECONDFS_ADDR].txtname, "0x%08x", curboot->second_addr);
		options[PROP_SECONDFS_ADDR].len = sizeof (uint32_t);
		SetType(&options[PROP_SECONDFS_ADDR], PROP_SECONDFS_ADDR);
	}
	if (curboot->page_size > 0)
	{
		sprintf(options[PROP_PAGE_SIZE].txtname, "%d", curboot->page_size);
		options[PROP_PAGE_SIZE].len = sizeof (uint32_t);
		SetType(&options[PROP_PAGE_SIZE], PROP_PAGE_SIZE);
	}
	else
		return 0;

	if (curboot->os_version > 0)
	{
		UnpackOSVersion(&ver, curboot->os_version);
		sprintf(options[MIX_OS_VERSION].txtname, "%d.%d.%d %02d/%d", ver.major, ver.minor, ver.patch, ver.month, ver.year);
		options[MIX_OS_VERSION].len = strlen(options[MIX_OS_VERSION].txtname);
		SetType(&options[MIX_OS_VERSION], MIX_OS_VERSION);
	}
	if (*((char *)curboot->name) != '\0')
	{
		strncpy(options[PROP_NAME].txtname, (char *) curboot->name, BOOT_NAME_SIZE);
		//options[PROP_NAME].len = wcstombs(options[PROP_NAME].txtname,(wchar_t *)curboot->name,BOOT_NAME_SIZE);
		options[PROP_NAME].len=strlen((char *) curboot->name);
		SetType(&options[PROP_NAME], PROP_NAME);
	}

	if(len==sizeof(droid_boot_header)){
		if(curboot->header_size>0){
			sprintf(options[PROP_DBOHADER_SIZE].txtname, "%d", curboot->header_size);
			options[PROP_DBOHADER_SIZE].len=sizeof(uint32_t);
			SetType(&options[PROP_DBOHADER_SIZE],PROP_DBOHADER_SIZE);
		}
        if(curboot->recovery_dtbo_offset>0){
			sprintf(options[PROP_DBO_ADDR].txtname, "0x%16lx", curboot->recovery_dtbo_offset);
		options[PROP_DBO_ADDR].len = sizeof (uint32_t);
		SetType(&options[PROP_DBO_ADDR], PROP_DBO_ADDR);
        }
	}

	int i, counto = 0;

	for (i = 0; i < 8; i++)
	{
		sprintf(&options[ID0].txtname[i * 8], "%08x", curboot->id[i]);
		//	    sprintf(options[ID0 + i].txtname, "%08x", curboot->id[i]);
		//	    SetType(&options[ID0], ID0);
		//	    options[ID0].len = sizeof (uint32_t);
		if (curboot->id[i] > 0)
			counto++;
	}
	SetType(&options[ID0], ID0);
	options[ID0].len = counto * sizeof (uint32_t);

	return 1;
}

int Config::EatTxtConfig(void *data)
{
	int len = 0;
	UfNtype *str = (UfNtype *) data, *src = (UfNtype *) data;
	SetParm((char *)"RAMDISK_GZ_LVL=9", names);		//Предустановленые невычисляемые параметры
	while (len < tblklen)
	{
		if (src[len] == '\r' || src[len] == '\n')
		{
			src[len] = '\0';
			if (str)
			{
				SetParm(str, names);
				if (src[len + 1] == '\r' || src[len + 1] == '\n')
					str = NULL;

			}
			else
				str = &src[len + 1];
		}
		len++;
	}
	return 1;
}

droid_boot_header *Config::GetHeader()
{
	memset(&it_cfg, 0x00, sizeof (droid_boot_header));

	if (options[PROP_KERNEL_ADDR].len > 0)
		sscanf(options[PROP_KERNEL_ADDR].txtname, "0x%08x", &it_cfg.kernel_addr);

	if (options[PROP_KERNELTAG_ADDR].len > 0)
		sscanf(options[PROP_KERNELTAG_ADDR].txtname, "0x%08x", &it_cfg.tags_addr);

	if (options[PROP_ROOTFS_ADDR].len > 0)
		sscanf(options[PROP_ROOTFS_ADDR].txtname, "0x%08x", &it_cfg.ramdisk_addr);

	if (options[PROP_SECONDFS_ADDR].len > 0)
		sscanf(options[PROP_SECONDFS_ADDR].txtname, "0x%08x", &it_cfg.second_addr);

	if (options[PROP_PAGE_SIZE].len > 0)
		sscanf(options[PROP_PAGE_SIZE].txtname, "%d", &it_cfg.page_size);
	else
		it_cfg.page_size = 0x800;

	if (options[MIX_OS_VERSION].len > 0)
	{
		sscanf(options[MIX_OS_VERSION].txtname, "%d.%d.%d %02d/%d", &ver.major, &ver.minor, &ver.patch, &ver.month, &ver.year);
		it_cfg.os_version = PackOSVersion(&ver);
	}

	if (options[PROP_NAME].len > 0){
	//mbtowc((wchar_t *)it_cfg.name,options[PROP_NAME].txtname,BOOT_NAME_SIZE);
	strncpy((char *)it_cfg.name,options[PROP_NAME].txtname,BOOT_NAME_SIZE);
	}


	if (options[PROP_CMDLINE].len > 0)
		strncpy((char *)it_cfg.cmdline,options[PROP_CMDLINE].txtname,BOOT_ARGS_SIZE);

	if (options[PROP_CMDLINE_EX].len > 0)
		strncpy((char *)it_cfg.extra_cmdline,options[PROP_CMDLINE_EX].txtname,BOOT_EXTRA_ARGS_SIZE);

	if(options[PROP_DBOHADER_SIZE].len > 0)
		sscanf(options[PROP_DBOHADER_SIZE].txtname, "%d", &it_cfg.header_size);

	if(options[PROP_DBO_ADDR].len > 0)
		sscanf(options[PROP_DBO_ADDR].txtname, "0x%16lx", &it_cfg.recovery_dtbo_offset);

	if (options[ID0].len > 0)
		for (int i = 0; i < 8; i++)
		{
			sscanf(&options[ID0].txtname[i * 8], "%08x", &it_cfg.id[i]);
		}


	return &it_cfg;
}

int Config::WriteCfg(UfNtype *file)
{
	//Выплевывает текстовый конфиг из загруженного ранее
	FILE *fh = NULL;
	char *str=NULL;
	int counter = 0;
	if ((fh = fopen(file, "wt")) == NULL)
		return counter;

	for (int i = 0; i < NUM_PROPS; i++)
	{
		if (options[i].len > 0)
		{
			str =new char[strlen(names[i])+strlen(options[i].txtname)+4];
			sprintf(str, "%s=%s\r\n", names[i], options[i].txtname);
			fwrite(str, 1, strlen(str), fh);
			delete str;
			counter++;
		}
	}
	fclose(fh);
	return counter;
}

char *Config::GetProp(int id){
	if(block||txtblk){
			if(id<NUM_PROPS&&options[id].len>0)return options[id].txtname;
	}
	return NULL;
}
