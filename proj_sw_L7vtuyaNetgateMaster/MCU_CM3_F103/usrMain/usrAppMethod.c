#include "usrAppMethod.h"

void *memmem(void *start, unsigned int s_len, void *find,unsigned int f_len){
	
	char *p, *q;
	unsigned int len;
	p = start, q = find;
	len = 0;
	while((p - (char *)start + f_len) <= s_len){
			while(*p++ == *q++){
					len++;
					if(len == f_len)
							return(p - f_len);
			};
			q = find;
			len = 0;
	};
	return(NULL);
}

int memloc(u8 str2[],u8 num_s2,u8 str1[],u8 num_s1)
{
	int la = num_s1;
	int i, j;
	int lb = num_s2;
	for(i = 0; i < lb; i ++)
	{
		for(j = 0; j < la && i + j < lb && str1[j] == str2[i + j]; j ++);
		if(j == la)return i;
	}
	return -1;
}

int strloc(char *str2,char *str1)
{
	int la = strlen(str1);
	int i, j;
	int lb = strlen(str2);
	for(i = 0; i < lb; i ++)
	{
		for(j = 0; j < la && i + j < lb && str1[j] == str2[i + j]; j ++);
		if(j == la)return i;
	}
	return -1;
}

//void printf_datsHtoA(const u8 *TipsHead, u8 *dats , u8 datsLen){

//	u8 dats_Log[DEBUG_LOGLEN] = {0};
//	u8 loop = 0;

//	memset(&dats_Log[0], 0, DEBUG_LOGLEN * sizeof(u8));
//	for(loop = 0; loop < datsLen; loop ++){

//		sprintf((char *)&dats_Log[loop * 3], "%02X ", *dats ++);
//	}
//	os_printf("%s<datsLen: %d> %s.\n", TipsHead, datsLen, dats_Log);
//}

