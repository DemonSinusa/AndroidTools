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
#include "FRulezConfigs.h"

#include "mkbootfs.h"
#include "BootUtils.h"
#include "LowLvlPck.h"
#include "Config.h"

unsigned int next_inode = 300000;

//extern char *maindir;

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
			   "5.dtbo приобщенное к *.img не встречал но если в config.conf прострелит параметр DBO_ADDRESS && DBO_HEADER_SIZE то пздравляю у Вас бут №1-(рас)\"могёт и умеет так Ж №2-(второй)\"\r\n"
			   "6.корявый boot_version принемает и DeviceTree.dat приклеит к ядру и как надо,а в прочем хз,надо ли так... \r\n"
			   "7.boot_version стал полноценным параметром конфиуряецца и работает вродебы. Воот... \"Но если карявый то это размер &ниипет&.\"\r\n"
			   "а если и есть что-то иное, то должно подходить и каждое \"это\" под описание, которое ниже и общее для понимания:\r\n"
			   "-Нулевой размер не изменяет содержимое в boot.img оригинальном иначе-иначе -^\r\n"
			   "-Отсутствие нужных онных требует наличие работоспособного бута для их появления без лишних пустышек..\r\n"
			   "-DownGrяdish boot:-продумай куда собираешься вклеитвать лишний loot..\r\n"
			   "Что умеет: \r\n"
			   "1.Подгонять,подчищать,обновлять руководствуясь структурным описанием из (bootimg.h) который всплывает из 2-х мест:\r\n"
			   "\t1.1-config.conf \r\n"
			   "\t1.2-из *.img тот что был взят..\r\n"
			   "2.Все,что выше в WELLDONE..\r\n"
			   "3.Ну а все остальное Вы сделаете сами своими руками)))\r\n"
			   "Без имен и подписей т.к. OpenSource(к)..\r\n"
			   "Полезности vs Ненужности:\r\n"
			   "$ cd ramdisk/\r\n"
			   "$ find * | cpio --owner=0:0 --quiet -o -H newc | gzip > ../ramdisk.cpio.gz\r\n"
			   "1.МОД-droid_fs_repacker:при наличии в WORKDIR каталога \"ramdisk/\" запаковывает его в читабельный дройду вид.\r\n"
			   "2.МОД-zImage_builder_helper:при наличии годной, необработаной, ядерно-кернеловой вытяжки приложить как zImage.pck и ликовать, после удалить лишнее\r\n\r\n"
			   "Эксперементальные параметры:\r\n"
			   "\"CPIO_PACK_BLOCK_ALIGN=0xHEX\"-использовать выравнивание размера пакуемого из каталога cpio на указанное значение.\r\n";


void InitTargets() {
	EnvDir[IT_KERNEL_PF]=(char *) "zImage.pck";
	EnvDir[IT_KERNEL_LF]=(char *)"zImageLoader.bin";
	EnvDir[IT_KERNEL_GF]=(char *)"zImage.gz";
	EnvDir[IT_KERNEL_DTBF]=(char *)"zImageDTB.bin";
	EnvDir[IT_ROOTFS_LF]=(char *)"ramdiskLoader.bin";
	EnvDir[IT_ROOTFS_PF]=(char *) "ramdisk.cpio.gz";
	EnvDir[IT_SECONDFS_LF]=(char *)"secondpartLoader.bin";
	EnvDir[IT_SECONDFS_PF]=(char *) "secondpart.dat";
	EnvDir[IT_DEVTREE]=(char *)"DevTree.dat";
	EnvDir[IT_DTBO_PF]=(char *)"dtbo.dat";
	EnvDir[IT_DTB_PF]=(char *)"dtb.dat";
	EnvDir[IT_CONFIG]=(char *) "config.conf";
	EnvDir[IT_ORIGFSIGN]=(char *)"Orig&FakenowSign.sigh";



	EnvDir[IT_READ_ME]=(char *) "README.txt";
	EnvDir[IT_RAMFS_DIR]=(char *) "ramdisk";
	EnvDir[IT_KERNEL_F]=(char *)"zImage";
}


void UpperCase(char *s) {
	int i = 0;

	while (s[i]!='\0') {
		if (s[i]>='a' && s[i]<='z') {
			s[i]=s[i]-32;
		}
		++i;
	}
}

int InOutPorting(char *selfname) {
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
	if (!stat(EnvPath[IT_CONFIG], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				cfg->EatTxtConfig(EnvPath[IT_CONFIG]);
				bu->SetCurMainConfig(cfg->GetHeader());
				cfg->WriteCfg(EnvPath[IT_CONFIG]);
			}
		}
	} else {
		tpoint = (char *) bu->GetCurMainConfig();
		if (tpoint) {
			if (((boot_img_hdr *)tpoint)->header_version==0)
				cfg->EatBinConfig(tpoint, sizeof (boot_img_hdr));
			else if(((boot_img_hdr *)tpoint)->header_version==1)
				cfg->EatBinConfig(tpoint, sizeof (struct boot_img_hdr_v1));
			else cfg->EatBinConfig(tpoint, sizeof (struct boot_img_hdr_v2));

			cfg->WriteCfg(EnvPath[IT_CONFIG]);
		}
	}
	//===========================Ядро===================================*.pck
	if (!stat(EnvPath[IT_KERNEL_PF], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				if (bu->InjKernel(EnvPath[IT_KERNEL_PF])!=1) {
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_KERNEL_PF]);
				}
			}
		}
	} else {
		if (!stat(EnvPath[IT_KERNEL_F], &tistic)) {				//WO_*.ext
			if (tistic.st_mode & (S_IFMT | S_IFREG)) {
				char *gzcmd=(char *)"gzip --no-name ";
				char *fullcmd = new char[strlen(EnvPath[IT_KERNEL_F]) + strlen(gzcmd)+1];
				strcpy(fullcmd, gzcmd);
				strcat(fullcmd, EnvPath[IT_KERNEL_F]);
				system(fullcmd);
				delete[] fullcmd;
			}
		}
		//!Spiliting...
		//Kernel gzipped
		if (!stat(EnvPath[IT_KERNEL_GF], &tistic)) {
			if (tistic.st_mode & (S_IFMT | S_IFREG)) {
				if (tistic.st_size != 0) {
					if (bu->SetKernelGz(EnvPath[IT_KERNEL_GF])!=1) {
						fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_KERNEL_GF]);
					}
				}
			}
		} else {
			tpoint = (char *) bu->GetKernelGz(&envlen);
			if (envlen > 0) {
				if ((fh = fopen(EnvPath[IT_KERNEL_GF], "wb")) != NULL) {
					fwrite(tpoint, envlen, 1, fh);
					fclose(fh);
				}
			}
		}
		//Kernel DTB table
		if (!stat(EnvPath[IT_KERNEL_DTBF], &tistic)) {
			if (tistic.st_mode & (S_IFMT | S_IFREG)) {
				if (tistic.st_size != 0) {
					if (bu->SetDTBList(EnvPath[IT_KERNEL_DTBF])!=1) {
						fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_KERNEL_DTBF]);
					}
				}
			}
		} else {
			tpoint = (char *) bu->GetDTBList(&envlen);
			if (envlen > 0) {
				if ((fh = fopen(EnvPath[IT_KERNEL_DTBF], "wb")) != NULL) {
					fwrite(tpoint, envlen, 1, fh);
					fclose(fh);
				}
			}
		}
	}
	//BootLoader MTK kernel
	char krnl[]="KERNEL";
	if (!stat(EnvPath[IT_KERNEL_LF], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				bu->SetPageLoader(KERNEL_BLK,bu->kernel_data.dtbsz+bu->kernel_data.gzsz,krnl,0x200);
			}
		}
	} else {
		tpoint = (char *) bu->GetPageLoader(KERNEL_BLK,&envlen);
		if (envlen > 0) {
			if ((fh = fopen(EnvPath[IT_KERNEL_LF], "wb")) != NULL) {
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}
	//!Spilited!
	//===========================Ядро===================================
	//-----------------------------------РутФС----------------------
	if (!stat(EnvPath[IT_RAMFS_DIR], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFDIR)) {
			PCK *pack = NULL;
			int count = 0;
			char *thecpio = (char *) "ramdisk.cpio", *fullcmd = NULL;
			char *fullpath = new char[strlen(thecpio) + strlen(EnvPath[IT_WORKDIR])+1];
			sprintf(fullpath, "%s%s", EnvPath[IT_WORKDIR], thecpio);
			pack = InitPacker(EnvPath[IT_RAMFS_DIR], fullpath, next_inode);
			count = CreateList(EnvPath[IT_RAMFS_DIR], pack);
			FInitPacker(pack);

			fprintf(stdout, "%s-%d вхождений\r\n", "Припаковано", count);

			char gzcmd[32];
			sprintf(gzcmd,"gzip %s %s ",cfg->GetProp(PROP_RAMGZ_LVL),"--no-name");
			fullcmd = new char[strlen(fullpath) + strlen(gzcmd)+1];
			strcpy(fullcmd, gzcmd);
			strcat(fullcmd, fullpath);
			system(fullcmd);
			delete[] fullcmd;
			delete[] fullpath;
		}
	}

	if (!stat(EnvPath[IT_ROOTFS_PF], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				if (bu->SetPageData(RAMFS_BLK,EnvPath[IT_ROOTFS_PF])!=1) {
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_ROOTFS_PF]);
				}
			}
		}
	} else {
		tpoint = (char *) bu->GetPageData(RAMFS_BLK,&envlen);
		if (envlen > 0) {
			if ((fh = fopen(EnvPath[IT_ROOTFS_PF], "wb")) != NULL) {
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}
	//Bootloader MTK
	char ramfsbl[]="ROOTFS";
	if (!stat(EnvPath[IT_ROOTFS_LF], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				bu->SetPageLoader(RAMFS_BLK,bu->root_fs.dlen,ramfsbl,0x200);
			}
		}
	} else {
		tpoint = (char *) bu->GetPageLoader(RAMFS_BLK,&envlen);
		if (envlen > 0) {
			if ((fh = fopen(EnvPath[IT_ROOTFS_LF], "wb")) != NULL) {
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}
	//-------------------------------RootFs----------------

	//Доп ПО

	if (!stat(EnvPath[IT_SECONDFS_PF], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				if (bu->SetPageData(SECONDFS_BLK,EnvPath[IT_SECONDFS_PF])!=1) {
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_SECONDFS_PF]);
				}
			}
		}
	} else {
		tpoint = (char *) bu->GetPageData(SECONDFS_BLK,&envlen);
		if (envlen > 0) {
			if ((fh = fopen(EnvPath[IT_SECONDFS_PF], "wb")) != NULL) {
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}

//BootLoadef MTK
	char scndfs[]="SECONDFS";
	if (!stat(EnvPath[IT_SECONDFS_LF], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				bu->SetPageLoader(SECONDFS_BLK,bu->second_part.dlen,scndfs,0x200);
			}
		}
	} else {
		tpoint = (char *) bu->GetPageLoader(SECONDFS_BLK,&envlen);
		if (envlen > 0) {
			if ((fh = fopen(EnvPath[IT_SECONDFS_LF], "wb")) != NULL) {
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}
	//Bootloader MTK
		//---------------------------DeviceTree---------------------------------
	if (!stat(EnvPath[IT_DEVTREE], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				if (bu->InjDTree(EnvPath[IT_DEVTREE])!=1) {
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_DEVTREE]);
				}
			}
		}
	} else {
		tpoint = (char *) bu->GetDTree(&envlen);
		if (envlen > 0) {
			if ((fh = fopen(EnvPath[IT_DEVTREE], "wb")) != NULL) {
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}
	//---------------------------Dtbo---------------------------------
	if (!stat(EnvPath[IT_DTBO_PF], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				if (bu->InjDtbo(EnvPath[IT_DTBO_PF])!=1) {
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_DTBO_PF]);
				}
			}
		}
	} else {
		tpoint = (char *) bu->GetDtbo(&envlen);
		if (envlen > 0) {
			if ((fh = fopen(EnvPath[IT_DTBO_PF], "wb")) != NULL) {
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}
//---------------------------Dtb---------------------------------
	if (!stat(EnvPath[IT_DTB_PF], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				if (bu->InjDtb(EnvPath[IT_DTB_PF])!=1) {
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_DTB_PF]);
				}
			}
		}
	} else {
		tpoint = (char *) bu->GetDtb(&envlen);
		if (envlen > 0) {
			if ((fh = fopen(EnvPath[IT_DTB_PF], "wb")) != NULL) {
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}
//----------------------------AnyWeight--------------------------------
	if (!stat(EnvPath[IT_ORIGFSIGN], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFREG)) {
			if (tistic.st_size != 0) {
				if (bu->InjAnythink(EnvPath[IT_ORIGFSIGN])!=1) {
					fprintf(stderr, "*File:%s\r\n--File KARUPPIT&have>0 and readn't and getn'ttoo(\r\n", EnvPath[IT_ORIGFSIGN]);
				}
			}
		}
	} else {
		tpoint = (char *) bu->GetMakeweight(&envlen);
		if (envlen > 0) {
			if ((fh = fopen(EnvPath[IT_ORIGFSIGN], "wb")) != NULL) {
				fwrite(tpoint, envlen, 1, fh);
				fclose(fh);
			}
		}
	}
//---------------------------===---------------------------------

	delete cfg;
	delete bu;
	delete[] EnvPath[IT_IMAGE];
	return 1;
}

/*
 *
 */
int ReMakeANDCatalogs(char *work_dir, char *selfname) {
	char *tpoint = work_dir, *wfile = NULL;
	struct stat tistic;
	int envlen = 0, tenvlen = 0, retval = 0;


	envlen = strlen(selfname);

	wfile = new UfNtype[envlen + 1];
	strcpy(wfile, selfname);


	InitTargets();

	EnvPath[IT_WORKDIR]=new UfNtype[strlen(tpoint)+1];
	strcpy(EnvPath[IT_WORKDIR], tpoint);

	if (!IsDir(EnvPath[IT_WORKDIR]))
		mkdir(EnvPath[IT_WORKDIR], 0775);


	if (!stat(EnvPath[IT_WORKDIR], &tistic)) {
		if (tistic.st_mode & (S_IFMT | S_IFDIR)) {

			tpoint = EnvDir[IT_READ_ME];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_READ_ME] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_READ_ME], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_READ_ME], tpoint);

			tpoint = EnvDir[IT_ROOTFS_PF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_ROOTFS_PF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_ROOTFS_PF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_ROOTFS_PF], tpoint);

			tpoint = EnvDir[IT_ROOTFS_LF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_ROOTFS_LF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_ROOTFS_LF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_ROOTFS_LF], tpoint);

			tpoint = EnvDir[IT_SECONDFS_PF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_SECONDFS_PF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_SECONDFS_PF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_SECONDFS_PF], tpoint);

			tpoint = EnvDir[IT_SECONDFS_LF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_SECONDFS_LF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_SECONDFS_LF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_SECONDFS_LF], tpoint);

			tpoint = EnvDir[IT_CONFIG];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_CONFIG] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_CONFIG], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_CONFIG], tpoint);

			tpoint = EnvDir[IT_ORIGFSIGN];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_ORIGFSIGN] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_ORIGFSIGN], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_ORIGFSIGN], tpoint);

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

			tpoint = EnvDir[IT_KERNEL_PF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_KERNEL_PF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_KERNEL_PF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_KERNEL_PF], tpoint);

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

			tpoint = EnvDir[IT_DEVTREE];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_DEVTREE] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_DEVTREE], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_DEVTREE], tpoint);

			tpoint=EnvDir[IT_DTBO_PF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_DTBO_PF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_DTBO_PF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_DTBO_PF], tpoint);

			tpoint=EnvDir[IT_DTB_PF];
			tenvlen = strlen(EnvPath[IT_WORKDIR]) + strlen(tpoint);
			EnvPath[IT_DTB_PF] = new UfNtype[tenvlen + 1];
			strcpy(EnvPath[IT_DTB_PF], EnvPath[IT_WORKDIR]);
			strcat(EnvPath[IT_DTB_PF], tpoint);


			retval = 1;

		} else
			retval = 0;
	}
	if (retval)
		InOutPorting(wfile);
	delete[] wfile;
	delete[] EnvPath[IT_ROOTFS_LF];
	delete[] EnvPath[IT_ROOTFS_PF];
	delete[] EnvPath[IT_READ_ME];
	delete[] EnvPath[IT_SECONDFS_LF];
	delete[] EnvPath[IT_SECONDFS_PF];
	delete[] EnvPath[IT_CONFIG];
	delete[] EnvPath[IT_ORIGFSIGN];
	delete[] EnvPath[IT_WORKDIR];
	delete[] EnvPath[IT_RAMFS_DIR];
	delete[] EnvPath[IT_KERNEL_F];
	delete[] EnvPath[IT_KERNEL_PF];
	delete[] EnvPath[IT_KERNEL_LF];
	delete[] EnvPath[IT_KERNEL_GF];
	delete[] EnvPath[IT_KERNEL_DTBF];
	delete[] EnvPath[IT_DEVTREE];
	delete[] EnvPath[IT_DTBO_PF];
	delete[] EnvPath[IT_DTB_PF];
	return retval;
}



