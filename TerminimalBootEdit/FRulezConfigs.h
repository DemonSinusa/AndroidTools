#ifndef FRULEZCONFIGS_H
#define FRULEZCONFIGS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define NONE_COMMENT            0
#define MULTILINE_COMMENT       1
#define ONELINE_COMMENT         2

typedef struct _str_in_mem {
    char *str;
    int len;
} SIM;

typedef struct _strs_in_mem {
    SIM str;
    char comtype;
    struct _strs_in_mem *next,*prev;
} SSIM;

typedef struct android_uid_info {
    char *name;
    unsigned aid;
    struct android_uid_info *next;
}AUI;

typedef struct android_fs_path_config {
    unsigned mode;
    AUI *u_uid,*u_gid;
    char *prefix;
	struct android_fs_path_config *next;
}AFPC;





SSIM *DoParse(char *mem);
void ParseClear(SSIM *root);

AUI *ParseIDs(SSIM *ids,int *count);
void FreeAIDs(AUI *root);

AFPC *ParseFS(SSIM *fs,AUI *all,int *count);
void FreeAFPCs(AFPC *root);

bool IsDir(char *path);
void fs_config(const char *path, int dir,unsigned *uid, unsigned *gid, unsigned *mode);


#ifdef	__cplusplus
}
#endif

#endif
