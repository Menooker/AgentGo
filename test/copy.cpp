#include <stdio.h>
int main(void)
{
    FILE *in, *out;
     if ((out = fopen("hash.txt", "wt")) == NULL)
     {
     fprintf(stderr, "创建文件失败.\n");
     return 1;
     }
     else
     {
      fprintf(out,"%s","剑桥大学\n");
      fprintf(out,"%s","2010311854\n");
      fprintf(out,"%s","崔升东\n");
     }
     fclose(out);
     return 0;
}
