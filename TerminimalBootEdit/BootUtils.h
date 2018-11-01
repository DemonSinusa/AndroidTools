/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   BootUtils.h
 * Author: aveman
 *
 * Created on 11 марта 2016 г., 20:16
 */

#ifndef BOOTUTILS_H
#define BOOTUTILS_H

#define UfNtype char

#include <openssl/sha.h>

class BootUtils
{
public:
    BootUtils(const BootUtils&);
    BootUtils(UfNtype *fname, int offset);
    virtual ~BootUtils();
    char *FullBoot = NULL;
    unsigned int static_flen = 0;
    boot_img_hdr *GetCurMainConfig();
    void SetCurMainConfig(boot_img_hdr *conf);
    int InjKernel(void *dump, int len);
    int InjKernel(char *fname);
    int InjROOTFS(void *dump, int len);
    int InjROOTFS(char *fname);
    int InjXZ401(void *dump, int len);
    int InjXZ401(char *fname);
    void *GetCurKernel(int *len);
    void *GetCurROOTFS(int *len);
    void *GetCurXZ401(int *len);
private:
    UfNtype *Bimgname = NULL;

    boot_img_hdr boot_h, *wbh = NULL;
    SHA_CTX ctx;
    unsigned char hash[SHA_DIGEST_LENGTH];
    int sha = 0;

    FILE *boot = NULL;
    int PhysOS = 0;
    int kernel_p_len = 0x00, rootfs_p_len = 0x00, unk_xzblk_p_len = 0x00;
    char *kernel_block_lnk = NULL,\
                             *rootfs_block_lnk = NULL, *unk_xzblk_lnk = NULL;

    bool OpenBFGrab(UfNtype *fname, int offset);
    void CloseBFile();

};

#endif /* BOOTUTILS_H */

