// GeneHatchery.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include "../AgentGo/GoContest/Board.h"


#define MUTATION_RATE 5
#define MUTATION_PERCENTAGE 0.5

#define GaIntToChar(i)  ((char)((i<='H'-'A')?i+'A':i+'A'+1))
#define GaCharToInt(i)  ((int)((i<='h')?i-'a':i-'a'-1))
#define ExcReplyErr 1
#define ExcReplyInvalid 2

WCHAR* DebugeeExe;
WCHAR* ReferenceExe;

HANDLE  hStdInRead[2];         //子进程用的stdin的读入端  
HANDLE  hStdInWrite[2];        //主程序用的stdin的读入端  
  
//定义句柄: 构成stdout管道的两端句柄  
HANDLE  hStdOutRead[2];     ///主程序用的stdout的读入端  
HANDLE  hStdOutWrite[2];    ///子进程用的stdout的写入端  
  
//定义句柄: 构成stderr管道的句柄，由于stderr一般等于stdout,我们没有定义hStdErrRead，直接用hStdOutRead即可  
HANDLE  hStdErrWrite[2];       ///子进程用的stderr的写入端  




void CreatePipes()
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if   (!CreatePipe(&hStdInRead[0], &hStdInWrite[0],&sa, 0))  
		return ;  
	if  (!CreatePipe(&hStdOutRead[0], &hStdOutWrite[0],&sa, 0))  
		return ;  
	if (!DuplicateHandle(GetCurrentProcess(), hStdOutWrite[0], GetCurrentProcess(), &hStdErrWrite[0], 0, TRUE, DUPLICATE_SAME_ACCESS))  
		return ; 

	if  (!CreatePipe(&hStdInRead[1], &hStdInWrite[1],&sa, 0))  
		return ;  
	if  (!CreatePipe(&hStdOutRead[1], &hStdOutWrite[1],&sa, 0))  
		return ;  
	if (!DuplicateHandle(GetCurrentProcess(), hStdOutWrite[1], GetCurrentProcess(), &hStdErrWrite[1], 0, TRUE, DUPLICATE_SAME_ACCESS))  
		return ; 

}


HANDLE CreateRedirectedProcess(WCHAR* path,int index)
{
	WCHAR p[MAX_PATH];
	wcscpy_s(p,MAX_PATH,path);

	STARTUPINFO siStartInfo;  
    PROCESS_INFORMATION piProcInfo;  
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );  
	siStartInfo.cb = sizeof(STARTUPINFO);  
	siStartInfo.dwFlags  |= STARTF_USESTDHANDLES;  
	siStartInfo.hStdOutput = hStdOutWrite[index];     //意思是：子进程的stdout输出到hStdOutWrite  
	siStartInfo.hStdError  =  hStdErrWrite[index];        //意思是：子进程的stderr输出到hStdErrWrite  
	siStartInfo.hStdInput  = hStdInRead[index];  


	if(CreateProcess(NULL,  
		p,    // 子进程的命令行  
		NULL,                   // process security attributes  
		NULL,                   // primary thread security attributes  
		TRUE,                   // handles are inherited  
		NULL,                          // creation flags  
		NULL,                  // use parent's environment  
		NULL,                  // use parent's current directory  
		&siStartInfo,      // STARTUPINFO pointer  
		&piProcInfo))     // receives PROCESS_INFORMATION  
	{
		CloseHandle(piProcInfo.hThread);
		return piProcInfo.hProcess;

	}
	else
	{
		__asm int 3
	}

}

int MyPrintf(int index,char* str,...)
{
	va_list args;
	va_start( args, str );
	char buf[4*1024];
	vsprintf_s(buf,4*1024,str,args);
	DWORD dwWritten;
	WriteFile( hStdInWrite[index], buf, strlen(buf), &dwWritten, NULL);  
	FlushFileBuffers(hStdInWrite[index]);
	return 0;
}


long ReadFromPipe2(int index,char* out_buffer,size_t sz)  
{  
	ZeroMemory(out_buffer,sz);
	DWORD dwRead;    
	BOOL bSuccess = FALSE;  
	bSuccess = ReadFile( hStdOutRead[index], out_buffer, sz, &dwRead, NULL);  
	if ((bSuccess) && (dwRead!=0))  //如果成功了，且长度>0  
	{  
		return dwRead;
	}  
	return 0;  
}  

void WaitForResponse(int index,char* dbuf) //dbuf's size>200
{
	char buf[200];
	int len=0;
	long outlen;
	for(;;)
	{
		outlen=ReadFromPipe2(index,buf+len,200-len);
		if(outlen>0)
		{	
			len+=outlen;
			if(buf[len-3]==10 && buf[len-2]==13 && buf[len-1]==10)
			{
				buf[len-3]=0;
				strcpy_s(dbuf,200,buf);
				return ;
			}
			else if(buf[len-2]==10 && buf[len-1]==10)
			{
				buf[len-2]=0;
				strcpy_s(dbuf,200,buf);
				return ;
			}
		}
	}
}

char dbuf[200];
bool Genmove(int index,int isW , int & a, int& b,char& c1)
{
	MyPrintf(index,"genmove %c\n",isW?'w':'b');
	WaitForResponse(index,dbuf);
	if(sscanf(dbuf,"= %c%d",&c1,&b)==2)
	{
		b--;
		a=GaCharToInt(tolower(c1));
		c1=toupper(c1);
		return true;
	}
	else
	{
		for(int i=1;i<strlen(dbuf);i++)
		{
			if(dbuf[i]!=' ' && dbuf[i]!='\t' )
			{
				if(!strnicmp(dbuf+i,"pass",4))
				{
					return false;
				}
				else
				{
					throw ExcReplyInvalid;
				}
			}
		}
	}
	throw ExcReplyInvalid;
	return false;
}

bool IsReplyOK(int index)
{
	WaitForResponse(index,dbuf);
	if(dbuf[0]=='=')
		return true;
	else 
		throw ExcReplyErr;
	return false; //fix-me : check the result
}

#define SET_SCORE(ii,jj) 	{\
	if(bd.data[i+ii][j+jj]==GO_BLACK)\
	{\
		bd.data[i][j]=GO_BLACK;flg=1;\
		continue;\
	}\
	else if (bd.data[i+ii][j+jj]==GO_WHITE)\
	{\
		bd.data[i][j]=GO_WHITE;flg=1;\
		continue;\
	}\
}


int CalcResults(Board& bd)
{
	int dscore=0;
	//bd.print();
	int flg=1;
	while(flg)
	{
		flg=0;
		for(int i=0;i<13;i++)
		{
			for(int j=0;j<13;j++)
			{
				if(bd.data[i][j]==GO_NULL)
				{
					if(i>0)
					{
						SET_SCORE(-1,0);
					}
					if(i<12)
					{
						SET_SCORE(1,0);
					}
					if(j>0)
					{
						SET_SCORE(0,-1);
					}
					if(j<12)
					{
						SET_SCORE(0,1);
					}
				}
			}
		}
	}
	//bd.print();
	for(int i=0;i<13;i++)
	{
		for(int j=0;j<13;j++)
		{
			dscore+= (bd.data[i][j]==GO_BLACK)?1:-1;
		}
	}
	return  dscore;
}

int SimulateOneGame(double dna[],int ndna,int index[])
{
	WCHAR path[MAX_PATH];
	wcscpy_s(path,MAX_PATH,DebugeeExe);
	WCHAR fstr[20];
	for(int i=0;i<ndna;i++)
	{
		wsprintf(fstr,L" %f",dna[i]);
		wcscat_s(path,MAX_PATH,fstr);
	}
	HANDLE hDebugee=CreateRedirectedProcess(path,0);
	HANDLE hReference=CreateRedirectedProcess(ReferenceExe,1);
	
	//char rbuf[200];
	int dscore;
	Board bd;
	bool flg=false;
	//try
	//{

		MyPrintf(0,"clear_board\n");
		IsReplyOK(0);
		MyPrintf(0,"boardsize 13\n");
		IsReplyOK(0);
		MyPrintf(1,"clear_board\n");
		IsReplyOK(1);
		MyPrintf(1,"boardsize 13\n");
		IsReplyOK(1);

		bool lastwplay=1,bplay;
		Piece pc;
		for(;;)
		{
			int a,b;
			char col;
			bplay=Genmove(index[0],0,a,b,col);
			if(bplay)
			{
				bd.put(GO_BLACK,a,b);
				MyPrintf(index[1],"play b %c%d\n",col,b+1);
				IsReplyOK(index[1]);
			}
			else
			{
				pc=bd.getRandomPiece(GO_BLACK);
				if(pc.isEmpty())
				{
					if(!lastwplay)
					{

						dscore=CalcResults(bd);
						break;
					}
					else
					{
						bd.pass(GO_BLACK);
						MyPrintf(index[1],"play b pass\n");
						IsReplyOK(index[1]);
					}
				}
				else
				{
					MyPrintf(index[0],"play b %c%d\n",GaIntToChar(pc.row),pc.col+1);
					IsReplyOK(index[0]);
					MyPrintf(index[1],"play b %c%d\n",GaIntToChar(pc.row),pc.col+1);
					IsReplyOK(index[1]);
					//bd.print();
					bd.put(pc);
					//bd.print();
					bplay=true;
				}


			}
			

			lastwplay=Genmove(index[1],1,a,b,col);
			if(lastwplay)
			{
				bd.put(GO_WHITE,a,b);
				MyPrintf(index[0],"play w %c%d\n",col,b+1);
				IsReplyOK(index[0]);
			}
			else
			{
				pc=bd.getRandomPiece(GO_WHITE);
				if(pc.isEmpty())
				{
					if(!bplay)
					{
						//end
						dscore=CalcResults(bd);
						break;
					}
					else
					{
						bd.pass(GO_WHITE);
						MyPrintf(index[0],"play w pass\n");
						IsReplyOK(index[0]);
					}
				}
				else
				{
					MyPrintf(index[0],"play w %c%d\n",GaIntToChar(pc.row),pc.col+1);
					IsReplyOK(index[0]);
					MyPrintf(index[1],"play w %c%d\n",GaIntToChar(pc.row),pc.col+1);
					IsReplyOK(index[1]);
					//bd.print();
					bd.put(pc);
					//bd.print();
					lastwplay=true;
				}


			}
			//bd.print();
		}
	//}
	//catch (int& Ex)
	//{

	//}
	TerminateProcess(hDebugee,0);
	TerminateProcess(hReference,0);
	CloseHandle(hDebugee);
	CloseHandle(hReference);
	return dscore;
}

void print_gene(double g[],int c)
{
	printf("<");
	for(int i=0;i<c;i++)
	{
		printf("%f,",g[i]);
	}
	printf(">\n");
}


void bubble(int d[],int outindex[],int len)
{
	bool flg;
	int tmp;
	for(int i=0;i<len;i++)
	{
		outindex[i]=i;
	}
	do
	{
		flg=0;
		for(int i=0;i<len-1;i++)
		{
			if(d[outindex[i]]<d[outindex[i+1]])
			{
				tmp=d[outindex[i]];
				d[outindex[i]]=d[outindex[i+1]];
				d[outindex[i+1]]=tmp;
				flg=1;
			}
		}
	}while(flg);
}

void slave()
{

}

double mutation(double ori)
{
	int go=rand()%99+1;
	double d;
	if(go<=MUTATION_RATE)
	{
		go=(rand()%1001)-500;
		d=ori*go/500*MUTATION_PERCENTAGE;
		return ori*go/500*MUTATION_PERCENTAGE;
	}
	else 
		return 0;
}

void mate(double f[],double m[],double s[],int DNAs)
{
	int crosspoint;
	float t;
	for(int i=0;i<DNAs;i++)
	{
		crosspoint= rand() % 2 ;
		t=(crosspoint)?f[i]:m[i];
		s[i]=t+mutation(t);
	}
}

void master(int slaves,int DNAs,double initDNA[],int cnt,int rounds,double* olddata)
{
	printf("Slaves: %d, DNAs: %d\n",slaves,DNAs);
	if(olddata)
		printf("Continue Execution....\n");
	else
		printf("%f %f %f\n",initDNA[0],initDNA[1],initDNA[2]);

	double* data;
	if(olddata)
		data=olddata;
	else
		data=(double*)malloc(sizeof(double)*DNAs*cnt);

	double** hatchery=(double**)malloc(sizeof(double*)*cnt);
	int* scores=(int*)malloc(sizeof(int)*cnt);
	int* sort_index=(int*)malloc(sizeof(int)*cnt);
	double *tmp=data;
	for(int i=0;i<cnt;i++)
	{
		hatchery[i]=tmp;
		if(!olddata)
		{
			for(int j=0;j<DNAs;j++)
			{
				hatchery[i][j]=initDNA[j]*0.75 + initDNA[j]*0.5 / cnt * i;
			}
		}
		tmp+=DNAs;
	}

	CreatePipes();

	int s1,s2,j;
	for(int rnd=0;rnd<5;rnd++)
	{
		FILE* fp=fopen("progress.ghp","wb");
		fwrite(&slaves,sizeof(slaves),1,fp);
		fwrite(&DNAs,sizeof(DNAs),1,fp);
		fwrite(&cnt,sizeof(cnt),1,fp);
		fwrite(data,sizeof(double)*DNAs*cnt,1,fp);
		fclose(fp);

		printf("Round %d started.\n",rnd);
		for(int i=0;i<cnt;i++)
		{
			print_gene(hatchery[i],DNAs);
		}
		for(int i=0;i<cnt;i++)
		{
			int index[2]={0,1};
			printf("%d:Testing gene : ",rnd);print_gene(hatchery[i],DNAs);
			s1=SimulateOneGame(hatchery[i],DNAs,index);
			printf("score : %d\n",s1);

			//index[0]=1;index[1]=0;
			//printf("%d:Testing gene : ",rnd);print_gene(hatchery[i],DNAs);
			//s2= -SimulateOneGame(hatchery[i],DNAs,index);
			//printf("score : %d\n",s2);
			s2=s1;
			scores[i]= (s1+s2)/2;
		}
		bubble(scores,sort_index,cnt);

		for(int i=cnt/2;i<cnt;i++)
		{
			j=i-cnt/2;
			mate(hatchery[j],hatchery[j+1],hatchery[i],DNAs);
			
		}

	}
	free(sort_index);
	free(scores);
	free(data);
	free(hatchery);
}


void param_abort()
{
	printf("Bad parameters!\n");
	exit(-1);
}

int _tmain(int argc, _TCHAR* argv[])
{
	srand(time(NULL));
	if(argc>=3)
	{
		if(!wcscmp(argv[1],L"slave"))
		{
			DebugeeExe=argv[2];
			ReferenceExe=argv[3];
			slave();
		}
		else if (!wcscmp(argv[1],L"master"))
		{
			if(argc>2)
			{
				DebugeeExe=argv[2];
				ReferenceExe=argv[3];
				int slaves=0;
				double initDNA[10]={0};
				int DNAs=0;
				int cnt=4;
				int rounds=2;
				double* olddata=0;
				for(int i=4;i<argc;i++)
				{
					if(!wcscmp(argv[i],L"-s") && i<argc-1)
					{
						if(!olddata)  slaves=_wtoi(argv[i+1]);
						i++;
					}
					else if(!wcscmp(argv[i],L"-f") && i<argc-1)
					{
						FILE* fp=_wfopen(argv[i+1],L"r");
						fread(&slaves,sizeof(slaves),1,fp);
						fread(&DNAs,sizeof(DNAs),1,fp);
						fread(&cnt,sizeof(cnt),1,fp);
						olddata=(double*)malloc(sizeof(double)*cnt*DNAs);
						fread(olddata,sizeof(double)*cnt*DNAs,1,fp);
						fclose(fp);
						i++;
					}
					else if(!wcscmp(argv[i],L"-r") && i<argc-1)
					{
						rounds=_wtoi(argv[i+1]);
						i++;
					}
					else if(!wcscmp(argv[i],L"-c") && i<argc-1)
					{
						if(!olddata)  cnt=_wtoi(argv[i+1]);
						if(cnt%2)
						{
							printf("sample count must be even!\n");
							exit(-1);
						}
						i++;
					}
					else if (argv[i][0]==L'-' && argv[i][1]==L'd')
					{
						if(!olddata)
							DNAs=_wtoi(argv[i]+2);
						if(i+DNAs>argc-1)
							param_abort();
						for(int j=0;j<DNAs;j++)
						{
							i++;
							initDNA[j]=_wtof(argv[i]);
						}
					}
					else
						param_abort();
				}
				master(slaves,DNAs,initDNA,cnt,rounds,olddata);
			}
			else
				param_abort();
		}
	}
	else
	{
		param_abort();
	}
	system("pause");
	return 0;
}

