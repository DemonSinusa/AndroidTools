/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Config.h
 * Author: senjord
 *
 * Created on 20 мая 2018 г., 13:56
 */

#ifndef CONFIG_H
#define CONFIG_H

#define MAX_PARAMETR_LEN    512


#define TYPE_TXT    0
#define TYPE_INT    1
#define TYPE_HEX    2
#define TYPE_DTIME  3

typedef struct item
{
	char txtname[MAX_PARAMETR_LEN];
	short datatype, len;
} OPT;

typedef struct _osversion_
{
	int major,minor,patch;
	int year,month;
} OSV;

#define    L_PROP_KERNEL_ADDR "KERNEL_ADDRESS"
#define    L_PROP_KERNELTAG_ADDR    "KERNEL_TAG_ADDRESS"
#define    L_PROP_ROOTFS_ADDR "ROOTFS_ADDRESS"
#define    L_PROP_SECONDFS_ADDR "SECONDFS_ADDRESS"
#define    L_PROP_PAGE_SIZE "PAGE_SIZE"

#define    L_MIX_OS_VERSION "OS_VERSION"

#define    L_PROP_NAME "BOOT_NAME"
#define    L_PROP_CMDLINE   "KERNEL_CMDLINE"
#define    L_PROP_CMDLINE_EX "KERNEL_CMDLINE_EX"

#define     L_ID0   "INT_ID_0"
#define     L_ID1   "INT_ID_1"
#define     L_ID2   "INT_ID_2"
#define     L_ID3   "INT_ID_3"
#define     L_ID4   "INT_ID_4"
#define     L_ID5   "INT_ID_5"
#define     L_ID6   "INT_ID_6"
#define     L_ID7   "INT_ID_7"

enum RW_var
{
	PROP_KERNEL_ADDR = 0,
	PROP_KERNELTAG_ADDR,
	PROP_ROOTFS_ADDR,
	PROP_SECONDFS_ADDR,
	PROP_PAGE_SIZE,

	MIX_OS_VERSION,

	PROP_NAME,
	PROP_CMDLINE,
	PROP_CMDLINE_EX,

	ID0,
	ID1,
	ID2,
	ID3,
	ID4,
	ID5,
	ID6,
	ID7,

	NUM_PROPS
};

class Config
{
public:
	Config();
	~Config();

	int EatBinConfig(void *data,unsigned int len);
	int EatBinConfig(UfNtype *file);

	int EatTxtConfig(void *data);
	int EatTxtConfig(UfNtype *file);

	int WriteCfg(UfNtype *file);
	boot_img_hdr *GetHeader();

private:
	FILE *fc = NULL;
	char *block = NULL, *txtblk = NULL;
	int blklen = 0, tblklen = 0;
	boot_img_hdr it_cfg;

	OSV ver;
	int PackOSVersion(OSV *version);
	void UnpackOSVersion(OSV *version,int packed);

	void SetType(OPT *item, int i);
	int SetParm(UfNtype *str, const char *list[]);

};

#endif /* CONFIG_H */

