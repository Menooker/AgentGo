// GeneHatchery.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include "../AgentGo/GoContest/Board.h"

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
	WriteFile( hStdInWrite[index], buf, strlen(buf)+1, &dwWritten, NULL);  
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
				if(!strcmp(dbuf+i+1,"pass"))
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
	if(dbuf[0]==' ')
		return true;
	else 
		throw ExcReplyErr;
	return false; //fix-me : check the result
}

void SimulateOneGame()
{
	HANDLE hDebugee=CreateRedirectedProcess(DebugeeExe,0);
	HANDLE hReference=CreateRedirectedProcess(ReferenceExe,1);
	
	//char rbuf[200];
	Board bd;
	try
	{

		MyPrintf(0,"clear_board\n");
		IsReplyOK(0);
		MyPrintf(0,"board_size 13\n");
		IsReplyOK(0);
		MyPrintf(1,"clear_board\n");
		IsReplyOK(1);
		MyPrintf(1,"board_size 13\n");
		IsReplyOK(1);

		bool lastwplay=1,bplay;
		for(;;)
		{
			int a,b;
			char col;
			bplay=Genmove(0,0,a,b,col);
			if(bplay)
			{
				bd.put(GO_BLACK,a,b);
				MyPrintf(1,"play b %c%d\n",col,b);
				IsReplyOK(1);
			}
			else
			{
				if(!lastwplay)
				{
					//end
				}
				else
				{
					MyPrintf(1,"play b pass\n");
					IsReplyOK(1);
				}
			}
			

			lastwplay=Genmove(1,1,a,b,col);
			if(lastwplay)
			{
				bd.put(GO_WHITE,a,b);
				MyPrintf(0,"play w %c%d\n",col,b);
				IsReplyOK(0);
			}
			else
			{
				if(!bplay)
				{
					//end
				}
				else
				{
					MyPrintf(0,"play w pass\n");
					IsReplyOK(0);
				}
			}
		}
	}
	catch (int& Ex)
	{

	}
	TerminateProcess(hDebugee,0);
	TerminateProcess(hReference,0);
	CloseHandle(hDebugee);
	CloseHandle(hReference);
}

void slave()
{

}

void master(int slaves,int DNAs,double initDNA[])
{
	printf("Slaves: %d, DNAs: %d\n",slaves,DNAs);
	printf("%f %f %f\n",initDNA[0],initDNA[1],initDNA[2]);
	CreatePipes();
	SimulateOneGame();
}


void param_abort()
{
	printf("Bad parameters!\n");
	exit(-1);
}

int _tmain(int argc, _TCHAR* argv[])
{
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
				for(int i=4;i<argc;i++)
				{
					if(!wcscmp(argv[i],L"-s") && i<argc-1)
					{
						slaves=_wtoi(argv[i+1]);
						i++;
					}
					else if (argv[i][0]==L'-' && argv[i][1]==L'd')
					{
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
				master(slaves,DNAs,initDNA);
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

