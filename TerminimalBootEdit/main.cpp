/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   main.cpp
 * Author: aveman
 *
 * Created on 11 марта 2016 г., 20:14
 */

#include <cstdlib>

using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#include <bootimg.h>

#include "BootUtils.h"
#include "LowLvlPck.h"


char *EnvDir[] = {NULL,
                  (char *) "zImage", (char *) "ramdisk.cpio.gz",
                  (char *) "README.txt", (char *) "secondpart.dat",
                  (char *) "config.conf", (char *) "ramdisk"
                 };

int main(int argc, char** argv)
{
    char WDR = 0;
    struct stat tistic;
    char *bootimgname = (char *) "boot.img";
    char *recimgname = (char *) "recovery.img";
    EnvDir[0] = new char[strlen(argv[0])];
    strcpy(EnvDir[0], argv[0]);
    char *poz = strrchr(EnvDir[0], '/');
    poz[1] = '\0';
    //--->
    WDR += ReMakeANDCatalogs((char *) "WORK_DIR_BOOT", bootimgname);
    WDR += ReMakeANDCatalogs((char *) "WORK_DIR_RECOVERY", recimgname);
    //---<
    delete EnvDir[0];


    return 0;
}

