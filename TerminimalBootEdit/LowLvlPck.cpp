/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <cstdlib>

using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <bootimg.h>

#include "mkbootfs.h"
#include "BootUtils.h"
#include "LowLvlPck.h"
#include "Config.h"

unsigned int next_inode = 300000;

extern char *maindir;

char *EnvDir[NUM_ITS];
char *EnvPath[NUM_ITS];


char *readme = (char *) "Структура каталогов и принцип работы:\r\n" \
               "Самblй оригинальный boot.img|&|recovery.img кладем рядом с запускаемой программой(при отсутствии будет собран из WORK_DIR)\r\n"
               "Все остальное интуитивно понятно,со структурой каталогов програмулька разберется сама...\r\n"
               "\t\tесли Вы это видите то возможность была ей дана :)\r\n"
               "Проверяет есть ли тут:\r\n"
				"1.config.conf, в нем будут хрантиться основные параметры конкретного образа для Android boot, при отсутствии будет вывернут из рабочего образа.\r\n"
				"2.zImage не сжатый в *.gz сожмет, к сжатому zImageLoader.bin|&|zImageDTB.bin при наличии приплетёт, едем дальше..\r\n"
				"3.ramdisk/ каталог в ramdisk.cpio перепишет и в *.gz пережмет добавив к прожекту и дальше попрет...\r\n"
				"4.secondpart не встречал и что за зверь я не знаю потому secondpart.dat при наличии ессно добавит и перейдем к boot -v1\r\n"
				"5.dtbo приобщенное к *.img не встречал но если в config.conf прострелит параметр DBO_ADDRESS && DBO_HEADER_SIZE то пздравляю у Вас бут №1-(рас)\r\n"
               "а если и есть то подходит ли каждый из них под описание, которое ниже и общее для понимания:\r\n"
               "-Нулевой размер не изменяет содержимое в boot.img оригинальном иначе-иначе -^\r\n"
               "-Отсутствие нужных онных требует наличие работоспособного бута для их появления без лишних пустышек..\r\n"
               "Что умеет: \r\n"
               "1.Подгонять,подчищать,обновлять руководствуясь структурным описанием из (bootimg.h) который всплывает из 2-х мест:\r\n"
               "\t1.1-config.conf \r\n"
               "\t1.2-из *.img тот что был взят..\r\n"
               "2.Все,что выше в WELLDONE..\r\n"
               "3.Ну а все остальное Вы сделаете сами своими руками)))\r\n"
               "Без имен и подписей т.к. OpenSource(к)..\r\n"
               "Полезности vs Ненужности:\r\n"
               "$ cd ramdisk/\r\n"
               "$ find . | cpio --quiet -o -H newc | gzip > ../ramdisk.cpio.gz\r\n"
               "1.МОД-droid_fs_repacker:при наличии в WORKDIR каталога \"ramdisk/\" запаковывает его в читабельный дройду вид.\r\n"
               "2.МОД-zImage_builder_helper:при наличии годной, необработаной, ядерно-кернеловой вытяжки приложить как zImage.pck и ликовать, после удалить лишнее\r\n";


void InitTargets(){
EnvDir[IT_KERNEL_PF]=(char *) "zImage.pck";
EnvDir[IT_KERNEL_LF]=(char *)"zImageLoader.bin";
EnvDir[IT_KERNEL_GF]=(char *)"zImage.gz";
EnvDir[IT_KERNEL_DTBF]=(char *)"zImageDTB.bin";
EnvDir[IT_ROOTFS_PF]=(char *) "ramdisk.cpio.gz";
EnvDir[IT_SECONDFS_PF]=(char *) "secondpart.dat";
EnvDir[IT_DBO_PF]=(char *)"dtbo.dat";
EnvDir[IT_CONFIG]=(char *) "config.conf";


EnvDir[IT_READ_ME]=(char *) "README.txt";
EnvDir[IT_RAMFS_DIR]=(char *) "ramdisk";
EnvDir[IT_KERNEL_F]=(char *)"zImage";
}

int InOutPorting(char *selfname)
{
	char *tpoint = selfname;
	struct stat tistic;
	FILE *fh = NULL;
	int envlen = 0;

	EnvPath[IT_IMAGE] = new UfNtype[strlen(tpoint) + 1];
	strcpy(EnvPath[IT_IMAGE], tpoint);

	//Пути разложены
	BootUtils *bu = new BootUtils(EnvPath[IT_IMAGE], 0);
	Config *cfg = new Config();
	//--->

	fh = fopen(EnvPath[IT_READ_ME], "wt");
	fwrite(readme, strlen(readme), 1, fh);
	fclose(fh);

	//Конфиг
	if (!stat(EnvPath[IT_CONFIG], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFREG))
		{
			if (tistic.st_size != 0)
			{
				cfg->EatTxtConfig(EnvPath[IT_CONFIG]);
				bu->SetCurMainConfig(cfg->GetHeader());
			}
		}
	}
	else
	{
		tpoint = (char *) bu->GetCurMainConfig();
		if (tpoint)
		{
            if(((boot_img_hdr *)tpoint)->header_version==0)
			cfg->EatBinConfig(tpoint, sizeof (boot_img_hdr));
			else cfg->EatBinConfig(tpoint, sizeof (droid_boot_header));
			cfg->WriteCfg(EnvPath[IT_CONFIG]);
		}
	}
	//===========================Ядро===================================
	if (!stat(EnvPath[IT_KERNEL_PF], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFREG))
		{
			if (tistic.st_size != 0)
			{
				if(bu->InjKernel(EnvPath[IT_KERNEL_PF])!=1)
				{
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_KERNEL_PF]);
				}
			}
		}
	}
	else
	{
	if (!stat(EnvPath[IT_KERNEL_F], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFREG))
		{
			char *gzcmd=(char *)"gzip --no-name -9 ";
			char *fullcmd = new char[strlen(EnvPath[IT_KERNEL_F]) + strlen(gzcmd)+1];
			strcpy(fullcmd, gzcmd);
			strcat(fullcmd, EnvPath[IT_KERNEL_F]);
			system(fullcmd);
			delete fullcmd;
		}
	}
	//BootLoader
	if (!stat(EnvPath[IT_KERNEL_LF], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFREG))
		{
			if (tistic.st_size != 0)
			{
				if(bu->SetBootLoader(EnvPath[IT_KERNEL_LF])!=1)
				{
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_KERNEL_LF]);
				}
			}
		}
	}else{
		tpoint = (char *) bu->GetBootLoader(&envlen);
		if (envlen > 0)
		{
			if ((fh = fopen(EnvPath[IT_KERNEL_LF], "wb")) != NULL)
			{
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
		}
		//Kernel gzipped
		if (!stat(EnvPath[IT_KERNEL_GF], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFREG))
		{
			if (tistic.st_size != 0)
			{
				if(bu->SetKernelGz(EnvPath[IT_KERNEL_GF])!=1)
				{
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_KERNEL_GF]);
				}
			}
		}
	}else{
		tpoint = (char *) bu->GetKernelGz(&envlen);
		if (envlen > 0)
		{
			if ((fh = fopen(EnvPath[IT_KERNEL_GF], "wb")) != NULL)
			{
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
		}
		//Kernel DTB table
	if (!stat(EnvPath[IT_KERNEL_DTBF], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFREG))
		{
			if (tistic.st_size != 0)
			{
				if(bu->SetDTBList(EnvPath[IT_KERNEL_DTBF])!=1)
				{
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_KERNEL_DTBF]);
				}
			}
		}
	}else{
		tpoint = (char *) bu->GetDTBList(&envlen);
		if (envlen > 0)
		{
			if ((fh = fopen(EnvPath[IT_KERNEL_DTBF], "wb")) != NULL)
			{
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
		}
	}
	//===========================Ядро===================================
	//-----------------------------------РутФС----------------------
	if (!stat(EnvPath[IT_RAMFS_DIR], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFDIR))
		{
			PCK *pack = NULL;
			int count = 0;
			char *thecpio = (char *) "ramdisk.cpio", *fullcmd = NULL;
			char *fullpath = new char[strlen(thecpio) + strlen(EnvPath[IT_WORKDIR])];
			sprintf(fullpath, "%s%s", EnvPath[IT_WORKDIR], thecpio);
			pack = InitPacker(EnvPath[IT_RAMFS_DIR], fullpath, next_inode);
			count = CreateList(EnvPath[IT_RAMFS_DIR], pack);
			FInitPacker(pack);

			fprintf(stdout, "%s-%d вхождений\r\n", "Припаковано", count);

			fullcmd = new char[strlen(fullpath) + 7];
			strcpy(fullcmd, "gzip ");
			strcat(fullcmd, fullpath);
			system(fullcmd);
			delete fullcmd;
			delete fullpath;
		}
	}

	if (!stat(EnvPath[IT_ROOTFS_PF], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFREG))
		{
			if (tistic.st_size != 0)
			{
				if(bu->InjROOTFS(EnvPath[IT_ROOTFS_PF])!=1)
				{
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_ROOTFS_PF]);
				}
			}
		}
	}
	else
	{
		tpoint = (char *) bu->GetCurROOTFS(&envlen);
		if (envlen > 0)
		{
			if ((fh = fopen(EnvPath[IT_ROOTFS_PF], "wb")) != NULL)
			{
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}
	//-------------------------------RootFs----------------

	//Доп ПО
	if (!stat(EnvPath[IT_SECONDFS_PF], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFREG))
		{
			if (tistic.st_size != 0)
			{
				if(bu->InjXZ401(EnvPath[IT_SECONDFS_PF])!=1)
				{
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_SECONDFS_PF]);
				}
			}
		}
	}
	else
	{
		tpoint = (char *) bu->GetCurXZ401(&envlen);
		if (envlen > 0)
		{
			if ((fh = fopen(EnvPath[IT_SECONDFS_PF], "wb")) != NULL)
			{
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}

	//Dbo
	if (!stat(EnvPath[IT_DBO_PF], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFREG))
		{
			if (tistic.st_size != 0)
			{
				if(bu->InjDbo(EnvPath[IT_DBO_PF])!=1)
				{
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_DBO_PF]);
				}
			}
		}
	}
	else
	{
		tpoint = (char *) bu->GetDbo(&envlen);
		if (envlen > 0)
		{
			if ((fh = fopen(EnvPath[IT_DBO_PF], "wb")) != NULL)
			{
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}


	delete cfg;
	delete bu;
	delete EnvPath[IT_IMAGE];
	return 1;
}

/*
 *
 */
int ReMakeANDCatalogs(char *w_name, char *selfname)
{
	char *tpoint = w_name, *wfile = NULL;
	struct stat tistic;
	int envlen = 0, tenvlen = 0, retval = 0;


	envlen = strlen(maindir);

	wfile = new UfNtype[envlen + strlen(selfname) + 1];
	strcpy(wfile, maindir);
	strcat(wfile, selfname);

	InitTargets();

	EnvPath[IT_WORKDIR] = new UfNtype[envlen + strlen(tpoint) + 2];
	strcpy(EnvPath[IT_WORKDIR], maindir);
	strcat(EnvPath[IT_WORKDIR], tpoint);
	envlen = strlen(EnvPath[IT_WORKDIR]);
	EnvPath[IT_WORKDIR][envlen] = TRUE_SLASH;
	EnvPath[IT_WORKDIR][envlen + 1] = '\0';

	mkdir(EnvPath[IT_WORKDIR], 0755);


	if (!stat(EnvPath[IT_WORKDIR], &tistic))
	{
		if (tistic.st_mode & (S_IFMT | S_IFDIR))
		{

			tpoint = EnvDir[IT_READ_ME];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_READ_ME] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_READ_ME], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_READ_ME], tpoint);
			tpoint = EnvDir[IT_KERNEL_PF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_KERNEL_PF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_KERNEL_PF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_KERNEL_PF], tpoint);
			tpoint = EnvDir[IT_ROOTFS_PF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_ROOTFS_PF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_ROOTFS_PF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_ROOTFS_PF], tpoint);
			tpoint = EnvDir[IT_SECONDFS_PF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_SECONDFS_PF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_SECONDFS_PF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_SECONDFS_PF], tpoint);
			tpoint = EnvDir[IT_CONFIG];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_CONFIG] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_CONFIG], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_CONFIG], tpoint);
			tpoint=EnvDir[IT_DBO_PF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_DBO_PF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_DBO_PF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_DBO_PF], tpoint);

			tpoint=EnvDir[IT_RAMFS_DIR];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_RAMFS_DIR] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_RAMFS_DIR], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_RAMFS_DIR], tpoint);

			tpoint=EnvDir[IT_KERNEL_F];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_KERNEL_F] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_KERNEL_F], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_KERNEL_F], tpoint);

			tpoint=EnvDir[IT_KERNEL_LF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_KERNEL_LF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_KERNEL_LF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_KERNEL_LF], tpoint);

			tpoint=EnvDir[IT_KERNEL_GF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_KERNEL_GF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_KERNEL_GF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_KERNEL_GF], tpoint);

			tpoint=EnvDir[IT_KERNEL_DTBF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_KERNEL_DTBF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_KERNEL_DTBF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_KERNEL_DTBF], tpoint);

			retval = 1;

		}
		else
			retval = 0;
	}
	if (retval)
		InOutPorting(wfile);
	delete wfile;
	delete EnvPath[IT_KERNEL_PF];
	delete EnvPath[IT_ROOTFS_PF];
	delete EnvPath[IT_READ_ME];
	delete EnvPath[IT_SECONDFS_PF];
	delete EnvPath[IT_CONFIG];
	delete EnvPath[IT_WORKDIR];
	delete EnvPath[IT_RAMFS_DIR];
	delete EnvPath[IT_DBO_PF];
	delete EnvPath[IT_KERNEL_F];
	delete EnvPath[IT_KERNEL_LF];
	delete EnvPath[IT_KERNEL_GF];
	delete EnvPath[IT_KERNEL_DTBF];
	return retval;
}



