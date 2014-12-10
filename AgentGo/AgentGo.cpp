// AgentGo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "TinyMT.h"
#include "GTPAdapter.h"
#include "DbgPipe.h"
#include <time.h>
#include <stdlib.h>
#include "GoContest\Board.h"
#include "TinyOP.h"

using namespace TinyMT;


class MyObject: public TObject
{

}myobj;
TObject ooo;
class MyJob:public TJob
{
public:
	Board* bd;
	int i;
	int j;
	int mark[13][13];
	MyJob(Board& old)
	{
		bd=new Board(old);
	}
	~MyJob()
	{
		delete bd;
	}
};
class MyThread:public TThread
{
	virtual void run()
	{
		MyJob* mj=(MyJob*)this->job;
		printf("Thread %d running\n",mj->i);
		this->tsleep(100);
		for(int i=0;i<1;i++)
		{

			osync(myobj)
			{
				printf("Thread %d entered\n",mj->i);
				this->tsleep(1000);
				break;
			}
			esync
		}
		printf("Thread %d left\n",mj->i);
	}
};




class MyWorker:public SWorker
{
	void work(TJob* j)
	{
		MyJob* mj=(MyJob*)j;
		//here////////////////////////////
		//dprintf("job (%d,%d) enter\n",mj->i,mj->j);
	}
};


class MyGame:public GTPAdapter
{

	void onClear()
	{
		
	}
	void onBoardSize(int sz)
	{
	
	}
	void onPlay(int isW,int a,int b)
	{
		dprintf("isW %d, a= %d ,b= %d\n",isW,a,b);
	}
	bool onMove(int isW,int& a,int& b)
	{

		/*for(int i=0;i<10000;i++)
		{
			a=rand()%13;
			b=rand()%13;
			if(bd.data[a][b]==0)
			{
				dprintf("genmove a= %d ,b= %d\n",a,b);
				return true;
			}
		}
		return false;//*/
		
		MyJob* jobs[13][13]={0};
		Scheduler<MyWorker>* psch=new Scheduler<MyWorker>(true);

		/////////////submit the jobs
		for( int i=0;i<13;i++)
		{
			for(int j=0;j<13;j++)
			{
				if(bd.data[i][j]!=GO_NULL)
					continue;
				jobs[i][j]=new MyJob(bd);
				jobs[i][j]->i=i;
				jobs[i][j]->j=j;
				psch->submit(jobs[i][j],1);
			}
		}

		/////run the threads and wait for the work completes
		psch->go();
		psch->wait();
		delete psch;

		/////////////////////////////////-<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


		
		/////delete "jobs"
		for( int i=0;i<13;i++)
		{
			for(int j=0;j<13;j++)
			{
				if(jobs[i][j])
					delete jobs[i][j];
			}
		}

		return true;//*/
	}

};

int _tmain(int argc, _TCHAR* argv[])
{
	
	Board bd;
	Board *bds[1000];
	bd.put(GO_WHITE,1,2);
	TinyOP<Board> pool(1000);
	printf("BD :%d\n",&bd);
	time_t ti=clock();
	for(int i=0;i<1000;i++)
	{
		bds[i]=pool.tnew(&bd);
		//bds[i]=new Board(bd);
		//printf("%d\n",bds[i]);
		
		if(bds[i]->data[1][2]!=GO_WHITE)
		{
			printf("ERR");
			break;
		}
	}
	for(int i=0;i<1000;i++)
	{
		//delete bds[i];
		pool.tdelete(bds[i]);
	
	}
	printf("%d",clock()-ti);
	system("pause");
	/*dinitdbg();
	srand(time(0));
	MyGame gm;
	gm.MainLoop(); //*/
	/*
	for(int cnt=0;cnt<1000;cnt++)
	{

	MyJob j[100];
	Scheduler<MyWorker>* psch=new Scheduler<MyWorker>(true);

	for( int i=0;i<100;i++)
	{
		j[i].i=i;
		psch->submit(&j[i],1);
	}
	psch->go();
	psch->wait();
	psch->abort(); //fix-me : "delete" after abort may cause reference to freed memory
	//psch->end();
	delete psch;
	//MessageBox(0,L"",L"",32);
	printf("KKKKKKKKKKKKKKK\n");
	
	}
	/*psch=new Scheduler<MyWorker>(10);
	for( int i=0;i<100;i++)
	{
		j[i].i=i;
		psch->submit(&j[i]);
		//t[i].start(&j[i]);
	}
	psch->go();
	psch->wait();
	psch->end();*/

	//system("pause");
	//ExitProcess(0);
	return 0;
}

