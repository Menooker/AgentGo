#include <stdio.h>
int main(void)
{
    FILE *in, *out;
     if ((out = fopen("hash.txt", "wt")) == NULL)
     {
     fprintf(stderr, "�����ļ�ʧ��.\n");
     return 1;
     }
     else
     {
      fprintf(out,"%s","���Ŵ�ѧ\n");
      fprintf(out,"%s","2010311854\n");
      fprintf(out,"%s","������\n");
     }
     fclose(out);
     return 0;
}
