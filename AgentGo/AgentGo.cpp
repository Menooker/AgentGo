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

TinyOP<Board> boardpool(1000);
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
		bd=boardpool.tnew(&old);//new Board(old);
	}
	~MyJob()
	{
		boardpool.tdelete(bd);
		//delete bd;
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
	
	long time_put=0, time_kill=0, time_getSet=0, time_random1=0, time_random2 =0;
	int test_size = 1000;

	clock_t cl=clock();
	Board* bd=new Board();
	for(int xx=0; xx<test_size; xx++)
	{
			int white_go=1,black_go=1;
			int cstep=0;
			while (white_go || black_go)
			{
				Piece rand;
				//mj->bd->print();
				//AG_PANIC();
				rand=bd->getRandomPiece(GO_BLACK);
				if (!rand.isEmpty())
				{
					bd->put(rand);//hei fang xia
					black_go=1;
				}
				else
				{
					black_go=0;
					bd->pass(GO_BLACK);
				}
				rand=bd->getRandomPiece(GO_WHITE);
				if (!rand.isEmpty())
				{
					bd->put(rand);//bai fang xia
					cstep++;
					white_go=1;
					
				}
				else
				{
					white_go=0;
					bd->pass(GO_WHITE);
				}
			}
			#ifdef GO_BOARD_TIME
				time_put += bd->time_put;
				time_kill += bd->time_kill;
				time_random1 = bd->time_random1;
				time_random2 = bd->time_random2;
			#endif
			bd->clear();
	}
	printf("total time: %d\n",clock()-cl);
	#ifdef GO_BOARD_TIME
		printf("time_put: %d\n",time_put);
		printf("time_kill: %d\n",time_kill);
		printf("time_random1: %d\n",time_random1);
		printf("time_random2: %d\n",time_random2);
	#endif
	system("pause");
	return 0;
}





//old TinyMT test
//*/
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