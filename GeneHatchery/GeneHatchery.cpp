// GeneHatchery.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stack>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include "../AgentGo/GoContest/Board.h"
#include <WinSock.h>
#pragma comment(lib, "WS2_32") 

/*
Network Mode Limits:
50 genes with 10 DNA positions

*/


#define MUTATION_RATE 2
#define MUTATION_PERCENTAGE 0.3

#define GaIntToChar(i)  ((char)((i<='H'-'A')?i+'A':i+'A'+1))
#define GaCharToInt(i)  ((int)((i<='h')?i-'a':i-'a'-1))
#define ExcReplyErr 1
#define ExcReplyInvalid 2
#define ExcBindErr 3

WCHAR* DebugeeExe;
WCHAR* ReferenceExe;

HANDLE  hStdInRead[2];         //子进程用的stdin的读入端  
HANDLE  hStdInWrite[2];        //主程序用的stdin的读入端  
  
//定义句柄: 构成stdout管道的两端句柄  
HANDLE  hStdOutRead[2];     ///主程序用的stdout的读入端  
HANDLE  hStdOutWrite[2];    ///子进程用的stdout的写入端  
  
//定义句柄: 构成stderr管道的句柄，由于stderr一般等于stdout,我们没有定义hStdErrRead，直接用hStdOutRead即可  
HANDLE  hStdErrWrite[2];       ///子进程用的stderr的写入端  



struct ServerParam
{
	int port;
	int* pscore;
	SOCKET slisten;
	SOCKET sClient;
};

struct ServerConfig
{
	int Magic;
	int id;
	int ndna;
	double dnas[15];
	int Magic2;
};

struct ClientReply
{
	
	int Magic;
	int cnt;
	struct{
		int id;
		int sc;
	}data[50];
	int Magic2;
};

long ServerPendingCnt;
HANDLE hEvent;

DWORD __stdcall ServerRecvProc(ServerParam* p)
{
	
	for(;;)
	{
		char buf[512];
		ClientReply* cr=(ClientReply*)buf;
		memset(buf,0,512);
		int ret = recv(p->sClient, (char*)cr, 512, 0);   
		if(ret > 0)
		{
			if(cr->Magic==0xFEFEFEFE && cr->Magic2==0xAAAABBBB)
			{
				for(int i=0;i<cr->cnt;i++)
				{
					p->pscore[cr->data[i].id]=cr->data[i].sc;
					printf("Remote Gene %d : %d\n",cr->data[i].id,cr->data[i].sc);
				}
				InterlockedExchangeAdd((LONG volatile *)&ServerPendingCnt,-cr->cnt);
				if(ServerPendingCnt<=0)
				{
					SetEvent(hEvent);
				}
			}
			else
				printf("A package disposed...\n");
		}
		else 
		{
			int err=WSAGetLastError();
			switch(err)
			{
			case WSAENETRESET:
			case WSAECONNRESET:
				printf("Connection down! port:%d\n",p->port);
				return 0;
				break;
			case WSAEMSGSIZE:
				printf("too large a package! port:%d\n",p->port);
				break;
			default:
				printf("unknown error %d! port:%d\n",err,p->port);
				return 0;
				break;
			}
		}
			
	}
}

DWORD __stdcall ServerProc(ServerParam* p)
{
    p->slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(p->slisten == INVALID_SOCKET)
    {
        printf("socket error !");
        return 0;
    }

    //绑定IP和端口
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(p->port);
    sin.sin_addr.S_un.S_addr = INADDR_ANY; 
    if(bind(p->slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        printf("bind error !");
		throw ExcBindErr;
    }

    //开始监听
    if(listen(p->slisten, 5) == SOCKET_ERROR)
    {
        printf("listen error !");
        throw ExcBindErr;
    }

    //循环接收数据
    sockaddr_in remoteAddr;
    int nAddrlen = sizeof(remoteAddr);
    char revData[255]; 
    printf("port %d waiting for connections...\n",p->port);
    p->sClient = accept(p->slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
    if(p->sClient == INVALID_SOCKET)
    {
        printf("accept error !");
        throw ExcBindErr;
    }
    printf("port %d accepted ：%s \n",p->port , inet_ntoa(remoteAddr.sin_addr));
	/*send(sClient, sendData, strlen(sendData), 0);
    //接收数据
    int ret = recv(sClient, revData, 255, 0);        
    if(ret > 0)
    {
        revData[ret] = 0x00;
        printf(revData);
    }

    //发送数据
    char * sendData = "你好，TCP客户端！\n";
    
    closesocket(sClient);
    closesocket(slisten);*/
	return 0;
}


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
		swprintf(fstr,L" %f",dna[i]);
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
				Sleep(250);

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
				tmp=outindex[i];
				outindex[i]=outindex[i+1];
				outindex[i+1]=tmp;
				flg=1;
			}
		}
	}while(flg);
}

void slave(char* ip,long port)
{
	CreatePipes();
	SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sclient == INVALID_SOCKET)
	{
		printf("invalid socket !");
		throw ExcBindErr;
	}

	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(port);
	serAddr.sin_addr.S_un.S_addr = inet_addr(ip); 
	if (connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		printf("connect error !");
		closesocket(sclient);
		throw ExcBindErr;
	}

	ClientReply rpy;
	stack<ServerConfig> Q;
	int cnt=0;
	for(;;)
	{
		ServerConfig sc={0};


		int ret = recv(sclient, (char*)&sc, sizeof(ServerConfig), 0);
		if(ret > 0)
		{
			if(sc.Magic==132143 && sc.Magic2==21439424)
			{
				printf("receiving tasks...\n");
				cnt++;
				Q.push(sc);
				printf("%d:",sc.id);
				print_gene(sc.dnas,sc.ndna);
				
			}
			else
			{
				if(sc.Magic==0x90909090)
				{
					//MessageBox(0,L"K",L"",64);
					printf("Go with %d tasks\n",cnt);
					int ii=0;
					
					rpy.Magic=0xFEFEFEFE;rpy.Magic2=0xAAAABBBB;
					while(!Q.empty())
					{
						ServerConfig& cfg=Q.top();
						int index[2] ={0,1};
						rpy.data[ii].id =cfg.id;
						printf("Testing gene : ");print_gene(cfg.dnas,cfg.ndna);
						int s1=SimulateOneGame(cfg.dnas,cfg.ndna,index);
						printf("score : %d\n",s1);

						index[0]=1;index[1]=0;
						printf("Testing gene : ");print_gene(cfg.dnas,cfg.ndna);
						int s2= -SimulateOneGame(cfg.dnas,cfg.ndna,index);
						printf("score : %d\n",s2);
						rpy.data[ii].sc =(s1+s2)/2;
						Q.pop();
						ii++;
					}
					rpy.cnt=ii;
					send(sclient,(char*)&rpy,sizeof(rpy),0);
				}
			}

		}
	}
	closesocket(sclient);

}

double mutation(double ori)
{
	int go=rand()%100+1;
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
//master "AgentGo_AI.exe" "C:\Users\Menooker\Desktop\Go\gnugo-3.8\gnugo.exe --mode gtp --level 1"  -d3 1000 1 1 -s 2 -c 6 -r 10
void master(int slaves,int DNAs,double initDNA[],int cnt,int rounds,double* olddata,ServerParam* parm)
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

	HANDLE* hThreads=(HANDLE*)malloc(sizeof(HANDLE)*slaves);
	for(int i=0;i<slaves;i++)
	{
		parm[i].pscore=scores;
		hThreads[i]=CreateThread(0,0,(LPTHREAD_START_ROUTINE)ServerRecvProc,&parm[i],0,0);
	}
	hEvent=CreateEvent(0,FALSE,FALSE,0);

	double *tmp=data;
	for(int i=0;i<cnt;i++)
	{
		hatchery[i]=tmp;
		if(!olddata)
		{
			for(int j=0;j<DNAs;j++)
			{
				hatchery[i][j]=initDNA[j]*0.75 + initDNA[j]*0.5 / cnt * i ;
			}
			if(i>0)
				mate(hatchery[i],hatchery[i-1],hatchery[i],DNAs);
		}
		tmp+=DNAs;
	}

	CreatePipes();
	ServerConfig sc;
	int s1,s2,j;
	int slave_rnd=cnt/(slaves+1);
	FILE* outfile=fopen("bests.csv","a+");
	for(int rnd=0;rnd<rounds;rnd++)
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

		int ii=0;
		ServerPendingCnt = slave_rnd * slaves;
		for(int i=0;i<slaves;i++)
		{
			for(int j=0;j<slave_rnd;j++)
			{
				sc.Magic=132143;sc.Magic2=21439424;
				sc.ndna=DNAs;
				sc.id=ii;
				memcpy(sc.dnas,hatchery[ii],DNAs*sizeof(double));
				send(parm[i].sClient,(char*)&sc,sizeof(sc),0);
				ii++;
			}
			sc.Magic=0x90909090;
			send(parm[i].sClient,(char*)&sc,sizeof(sc),0);
			//MessageBox(0,L"K",L"",64);
		}

		for(int i=ii;i<cnt;i++)
		{
			int index[2]={0,1};
			printf("%d:%d Testing gene : ",rnd,i);print_gene(hatchery[i],DNAs);
			s1=SimulateOneGame(hatchery[i],DNAs,index);
			printf("score : %d\n",s1);

			index[0]=1;index[1]=0;
			printf("%d:%d Testing gene : ",rnd,i);print_gene(hatchery[i],DNAs);
			s2= -SimulateOneGame(hatchery[i],DNAs,index);
			printf("score : %d\n",s2);
			//s2=s1;
			scores[i]= (s1+s2)/2;
		}
		if(slaves)
			WaitForSingleObject(hEvent,-1);
		bubble(scores,sort_index,cnt);

		fprintf(outfile,"%d",rnd);
		for(int i=0;i<DNAs;i++)
		{
			fprintf(outfile,",%f",hatchery[sort_index[0]][i]);
		}
		fprintf(outfile,",%d\n",scores[sort_index[0]]);

		for(int i=cnt/2;i<cnt;i++)
		{
			j=i-cnt/2;
			mate(hatchery[sort_index[j]],hatchery[sort_index[j+1]],hatchery[sort_index[i]],DNAs);	
		}

	}
	fclose(outfile);
	free(sort_index);
	free(scores);
	free(data);
	free(hatchery);
	for(int i=0;i<slaves;i++)
	{
		TerminateThread(hThreads[i],0);
		CloseHandle(hThreads[i]);
		
	}
	free(hThreads);
	CloseHandle(hEvent);
}


void param_abort()
{
	printf("Bad parameters!\nusage for master mode:\nGeneHatchery master [-f {path to saved records}] [-s {slaves}] [-r {generations}] [-c {competitors}] [-d {DNAs dna0 dna1 dna2...}]\n");
	printf("usage for slave mode:\nGeneHatchery slave {master ip} {port}");
	exit(-1);
}
//-f progress.ghp
int _tmain(int argc, _TCHAR* argv[])
{
    WORD sockVersion = MAKEWORD(2,2);
    WSADATA wsaData;
    if(WSAStartup(sockVersion, &wsaData)!=0)
    {
        return 0;
    }
	srand(time(NULL));
	if(argc>=3)
	{
		if(!wcscmp(argv[1],L"slave") && argc==6)
		{
			DebugeeExe=argv[2];
			ReferenceExe=argv[3];
			char ip[100];
			sprintf(ip,"%ws",argv[4]);
			int port=_wtoi(argv[5]);
			printf("Connecting %s:%d",ip,port);
			slave(ip,port);
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
				ServerParam* servers=0;
				HANDLE* handles=0;
				if(slaves>0)
				{
					servers=(ServerParam*)malloc(sizeof(ServerParam)*slaves);
					handles=(HANDLE*)malloc(sizeof(HANDLE)*slaves);
					for(int i=0;i<slaves;i++)
					{
						servers[i].port=3000+i;
						handles[i]=CreateThread(0,0,(LPTHREAD_START_ROUTINE)ServerProc,&servers[i],0,0);
					}
					WaitForMultipleObjects(slaves,handles,TRUE,-1);
					for(int i=0;i<slaves;i++)
					{
						CloseHandle(handles[i]);
					}
				}
				master(slaves,DNAs,initDNA,cnt,rounds,olddata,servers);
				if(servers)
				{
					free(servers);
					free(handles);
				}//fix-me : close the sockets
			}
			else
				param_abort();
		}
		else
			param_abort();
	}
	else
	{
		param_abort();
	}
	WSACleanup();
	system("pause");
	return 0;
}

