/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   LowLvlPck.h
 * Author: senjord
 *
 * Created on 20 мая 2018 г., 13:49
 */

#ifndef LOWLVLPCK_H
#define LOWLVLPCK_H

#ifdef WIN
#define TRUE_SLASH  '\\'
#else
#define TRUE_SLASH  '/'
#endif

enum _its_filez {
    IT_KERNEL_PF=0,				//Необработаное ядро
    IT_KERNEL_LF,				//Загрузчик ядра
    IT_KERNEL_GF,				//Сжатое ядро
    IT_KERNEL_DTBF,				//Таблица смещений ядра
    IT_KERNEL_F,				//Распакованое ядро
    IT_ROOTFS_LF,				//Загрузчик рутфс
    IT_ROOTFS_PF,				//Пакованый рутфс
    IT_RAMFS_DIR,				//Распакованая РутФС
    IT_SECONDFS_LF,				//Загрузчик 3-й части
    IT_SECONDFS_PF,				//Пакованый секондфс
    IT_DBO_PF,					//?Пакованый dtbo boot v1

    IT_READ_ME,
    IT_CONFIG,					//Текстовый конфиг

    IT_WORKDIR,					//Рабочий каталог
    IT_IMAGE,					//Путь до *.img


	NUM_ITS
};

typedef struct _dolbbdtwosa_{
	char *str;
	short len,_isdef;
}HRGA;


void UpperCase(char *s);
int ReMakeANDCatalogs(char *w_name, char *selfname);


#endif /* LOWLVLPCK_H */

