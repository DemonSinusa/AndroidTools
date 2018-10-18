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

extern char *EnvDir[];

char *EnvPath[] = {
    NULL, //Ядро
    NULL, //РутФС
    NULL, //Прочти.тхт
    NULL, //Доп.ПО
    NULL, //Конфиг
    NULL, //Рабочий каталог
    NULL, //Путь до *.img
    NULL //Каталог ramdisk/
};


char *readme = (char *) "Структура каталогов и принцип работы:\r\n" \
               "Самblй оригинальный boot.img|&|recovery.img кладем рядом с запускаемой программой(при отсутствии будет собран из WORK_DIR)\r\n"
"Все остальное интуитивно понятно,со структурой каталогов програмулька разберется сама...\r\n"
"\t\tесли Вы это видите то возможность была ей дана :)\r\n"
"Проверяет есть ли тут zImage|&|ramdisk.cpio.gz|&|secondpart.dat|&|config.conf, \r\n"
"а если и есть то подходит ли каждый из них под описание, которое ниже и общее для понимания:\r\n"
"-Нулевой размер не изменяет содержимое в boot.img оригинальном иначе-иначе -^\r\n"
"-Отсутствие нужных онных требует наличие работоспособного бута для их появления без лишних пустышек..\r\n"
"Что умеет: \r\n"
"1.Подгонять,подчищать,обновлять руководствуясь структурным описанием из (bootimg.h) который всплывает из 2-х мест:\r\n"
"\t1.1-config.conf \r\n"
"\t1.2-из boot.img тот что был взят..\r\n"
"2.Все,что выше в WELLDONE..\r\n"
"3.Ну а все остальное Вы сделаете сами своими руками)))\r\n"
"Без имен и подписей т.к. OpenSource(к)..\r\n"
"Полезности:\r\n"
"$ cd ramdisk/\r\n"
"$ find . | cpio --quiet -o -H newc | gzip > ../ramdisk.cpio.gz\r\n"
"1.МОД-droid_fs_repacker:при наличии в WORKDIR каталога \"ramdisk/\" запаковывает его в читабельный дройду вид."
"1.1 полученый ramdisk.cpio остается только преобразовать $ gzip ramdisk.cpio и перезапустить тулзу для добавления в *.img\r\n";

int InOutPorting(char *selfname)
{
    char *tpoint = selfname;
    struct stat tistic;
    FILE *fh = NULL;
    int envlen = 0;
    int datalen = 0;
    char *data = NULL;

    EnvPath[6] = new UfNtype[strlen(tpoint) + 1];
    strcpy(EnvPath[6], tpoint);

    //Пути разложены
    BootUtils *bu = new BootUtils(EnvPath[6], 0);
    Config *cfg = new Config();
    //--->

    fh = fopen(EnvPath[2], "wt");
    fwrite(readme, strlen(readme), 1, fh);
    fclose(fh);

    //Конфиг
    if (!stat(EnvPath[4], &tistic)) {
	if (tistic.st_mode & (S_IFMT | S_IFREG)) {
	    if (tistic.st_size != 0) {
		cfg->EatTxtConfig(EnvPath[4]);
		bu->SetCurMainConfig(cfg->GetHeader());
	    }
	}
    } else {
	tpoint = (char *) bu->GetCurMainConfig();
	if (tpoint) {
	    cfg->EatBinConfig(tpoint, sizeof (boot_img_hdr));
	    cfg->WriteCfg(EnvPath[4]);
	}
    }
    //Ядро
    if (!stat(EnvPath[0], &tistic)) {
	if (tistic.st_mode & (S_IFMT | S_IFREG)) {
	    if (tistic.st_size != 0) {
		if ((fh = fopen(EnvPath[0], "rb")) != NULL) {
		    fseek(fh, 0, SEEK_END);
		    datalen = ftell(fh);
		    data = new char[datalen];
		    fseek(fh, 0, SEEK_SET);
		    fread(data, datalen, 1, fh);
		    bu->InjKernel(data, datalen);
		    delete data;
		    fclose(fh);
		}
	    }
	}
    } else {
	tpoint = (char *) bu->GetCurKernel(&envlen);
	if (envlen > 0) {
	    if ((fh = fopen(EnvPath[0], "wb")) != NULL) {
		fwrite(tpoint, envlen, 1, fh);
		fclose(fh);
	    }
	}
    }
    //РутФС
    if (!stat(EnvPath[1], &tistic)) {
	if (tistic.st_mode & (S_IFMT | S_IFREG)) {
	    if (tistic.st_size != 0) {
		if ((fh = fopen(EnvPath[1], "rb")) != NULL) {
		    fseek(fh, 0, SEEK_END);
		    datalen = ftell(fh);
		    data = new char[datalen];
		    fseek(fh, 0, SEEK_SET);
		    fread(data, datalen, 1, fh);
		    bu->InjROOTFS(data, datalen);
		    delete data;
		    fclose(fh);
		}
	    }
	}
    } else {
	tpoint = (char *) bu->GetCurROOTFS(&envlen);
	if (envlen > 0) {
	    if ((fh = fopen(EnvPath[1], "wb")) != NULL) {
		fwrite(tpoint, envlen, 1, fh);
		fclose(fh);
	    }
	}
    }

    //Доп ПО
    if (!stat(EnvPath[3], &tistic)) {
	if (tistic.st_mode & (S_IFMT | S_IFREG)) {
	    if (tistic.st_size != 0) {
		if ((fh = fopen(EnvPath[3], "rb")) != NULL) {
		    fseek(fh, 0, SEEK_END);
		    datalen = ftell(fh);
		    data = new char[datalen];
		    fseek(fh, 0, SEEK_SET);
		    fread(data, datalen, 1, fh);
		    bu->InjXZ401(data, datalen);
		    delete data;
		    fclose(fh);
		}
	    }
	}
    } else {
	tpoint = (char *) bu->GetCurXZ401(&envlen);
	if (envlen > 0) {
	    if ((fh = fopen(EnvPath[3], "wb")) != NULL) {
		fwrite(tpoint, envlen, 1, fh);
		fclose(fh);
	    }
	}
    }

    if (!stat(EnvPath[7], &tistic)) {
	if (tistic.st_mode & (S_IFMT | S_IFDIR)) {
	    PCK *pack = NULL;
	    int count = 0;
	    char *thecpio = (char *) "ramdisk.cpio", *fullcmd = NULL;
	    char *fullpath = new char[strlen(thecpio) + strlen(EnvPath[5])];
	    sprintf(fullpath, "%s%s", EnvPath[5], thecpio);
	    pack = InitPacker(EnvPath[7], fullpath, next_inode);
	    count = CreateList(EnvPath[7], pack);
	    FInitPacker(pack);
#ifdef DEBUG
	    fprintf(stdout, "%s-%d вхождений\r\n", "Припаковано", count);
#endif
	    fullcmd = new char[strlen(fullpath) + 7];
	    strcpy(fullcmd, "gzip ");
	    strcat(fullcmd, fullpath);
	    system(fullcmd);
	    delete fullcmd;
	    delete fullpath;
	}
    }


    delete cfg;
    delete bu;
    delete EnvPath[6];
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


    envlen = strlen(EnvDir[0]);

    wfile = new UfNtype[envlen + strlen(selfname) + 1];
    strcpy(wfile, EnvDir[0]);
    strcat(wfile, selfname);
    //WD=E5
    EnvPath[5] = new UfNtype[envlen + strlen(tpoint) + 2];
    strcpy(EnvPath[5], EnvDir[0]);
    strcat(EnvPath[5], tpoint);
    envlen = strlen(EnvPath[5]);
    EnvPath[5][envlen] = TRUE_SLASH;
    EnvPath[5][envlen + 1] = '\0';


    EnvPath[7] = new UfNtype[envlen + strlen(EnvDir[6])];
    strcpy(EnvPath[7], EnvPath[5]);
    strcat(EnvPath[7], EnvDir[6]);

    mkdir(EnvPath[5], 0755);


    if (!stat(EnvPath[5], &tistic)) {
	if (tistic.st_mode & (S_IFMT | S_IFDIR)) {

	    tpoint = EnvDir[3];
	    tenvlen = strlen(EnvPath[5]) + strlen(tpoint);
	    EnvPath[2] = new UfNtype[tenvlen + 1];
	    strcpy(EnvPath[2], EnvPath[5]);
	    strcat(EnvPath[2], tpoint);
	    tpoint = EnvDir[1];
	    tenvlen = strlen(EnvPath[5]) + strlen(tpoint);
	    EnvPath[0] = new UfNtype[tenvlen + 1];
	    strcpy(EnvPath[0], EnvPath[5]);
	    strcat(EnvPath[0], tpoint);
	    tpoint = EnvDir[2];
	    tenvlen = strlen(EnvPath[5]) + strlen(tpoint);
	    EnvPath[1] = new UfNtype[tenvlen + 1];
	    strcpy(EnvPath[1], EnvPath[5]);
	    strcat(EnvPath[1], tpoint);
	    tpoint = EnvDir[4];
	    tenvlen = strlen(EnvPath[5]) + strlen(tpoint);
	    EnvPath[3] = new UfNtype[tenvlen + 1];
	    strcpy(EnvPath[3], EnvPath[5]);
	    strcat(EnvPath[3], tpoint);
	    tpoint = EnvDir[5];
	    tenvlen = strlen(EnvPath[5]) + strlen(tpoint);
	    EnvPath[4] = new UfNtype[tenvlen + 1];
	    strcpy(EnvPath[4], EnvPath[5]);
	    strcat(EnvPath[4], tpoint);


	    retval = 1;

	} else
	    retval = 0;
    }
    if (retval)
	InOutPorting(wfile);
    delete wfile;
    delete EnvPath[0];
    delete EnvPath[1];
    delete EnvPath[2];
    delete EnvPath[3];
    delete EnvPath[4];
    delete EnvPath[5];
    delete EnvPath[7];
    return retval;
}



