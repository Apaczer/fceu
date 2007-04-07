#include <stdlib.h>
void GetBaseDirectory(char *BaseDirectory)
{
 char *ol;

 ol=getenv("HOME");
 BaseDirectory[0]=0;
 if(ol)
 {
  strncpy(BaseDirectory,ol,2047);
  BaseDirectory[2047]=0;
  strcat(BaseDirectory,"/.fceultra");
 }
}

