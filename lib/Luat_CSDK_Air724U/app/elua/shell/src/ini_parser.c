/*elbowµâ µug²»ughnâchâchâxâ ughâ ²»Windows BEWICIS. RedString”oGetPrivateInt »±±¶üÀòèÀÃÀÃÀïÀÃÀïÁeÚnÀÚÚÚnèÚÕ
µ ‘BELERYughâ ³BynèÕÕ´èèà´àugh«uggâ ¾¾µ²µ ESS´´üughüug´´´´ug´´´¯²¨²µµ*/
#include <string.h>
#ifdef WIN32
#include <Windows.h>
#include <stdio.h>
#else
#define  MAX_PATH 260
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#endif
char g_szConfigPath[MAX_PATH];

#include "am_openat.h"

#define fgetc getc

//´ÓINIÎÄ¼þ¶ÁÈ¡×Ö·û´®ÀàÐÍÊý¾Ý
char *GetIniKeyString(char *title,char *key,char *filename) 
{ 
	FILE *fp; 
	char szLine[256];
	static char tmpstr[256];
	int rtnval;
	int i = 0; 
	int flag = 0; 
	char *tmp;
	char* strend;
 
	if((fp = fopen(filename, "r")) == NULL) 
	{ 
		printf("have   no   such   file \n");
		return ""; 
	}
	while(!feof(fp)) 
	{ 
		rtnval = fgetc(fp); 
		if(rtnval == EOF) 
		{ 
			break; 
		} 
		else 
		{ 
			szLine[i++] = rtnval; 
		} 
		if(rtnval == '\n') 
		{ 
#ifndef WIN32
			i--;
#endif	
			szLine[--i] = '\0';
			i = 0; 
			tmp = strchr(szLine, '='); 

 
			if(( tmp != NULL )&&(flag == 1)) 
			{ 
				if(strstr(szLine,key)!=NULL) 
				{ 
					// × ¢ ênðð
					if ('#' == szLine[0])
					{
					}
					else if ( '/' == szLine[0] && '/' == szLine[1] )
					{
						
					}
					else
					{
						//ÕÒ´òkey¶ÔÓ¦±äÁ¿
						tmp++;
						while(*tmp && (*tmp == ' ' || *tmp == '\t')) tmp++;
						strend = tmp;
						while(*strend && *strend != ' ' && *strend != '\t') strend++;
						strncpy(tmpstr,tmp, strend - tmp); 
						tmpstr[strend - tmp] = 0;
						fclose(fp);
						return tmpstr; 
					}
				} 
			}
			else 
			{ 
				strcpy(tmpstr,"["); 
				strcat(tmpstr,title); 
				strcat(tmpstr,"]");
				if( strncmp(tmpstr,szLine,strlen(tmpstr)) == 0 ) 
				{
					//ÕÒµ½title
					flag = 1; 
				}
			}
		}
	}
	fclose(fp); 
	return ""; 
}
 
//´ÓINIÎÄ¼þ¶ÁÈ¡ÕûÀàÐÍÊý¾Ý
int GetIniKeyInt(char *title,char *key,char *filename)
{
	return atoi(GetIniKeyString(title,key,filename));
}
 


