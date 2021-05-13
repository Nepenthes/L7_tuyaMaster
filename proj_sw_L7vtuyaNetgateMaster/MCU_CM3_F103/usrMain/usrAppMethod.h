#ifndef __USRAPPMETHOD_H__
#define __USRAPPMETHOD_H__

#include "dataManage.h"

/*¹¦ÄÜº¯Êý*/
void *memmem(void *start, unsigned int s_len, void *find,unsigned int f_len);
int memloc(u8 str2[],u8 num_s2,u8 str1[],u8 num_s1);
int strloc(char *str2,char *str1);

#endif
