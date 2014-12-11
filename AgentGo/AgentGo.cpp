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

		Piece pc=bd.getRandomPiece(isW+1);
		a=pc.row;
		b=pc.col;
		dprintf("genmove a= %d ,b= %d\n",a,b);
		return !pc.isEmpty();
	}

};

int _tmain(int argc, _TCHAR* argv[])
{
	
	dinitdbg();
	dprintf("%d\n",sizeof(Board));
	srand(time(0));
	MyGame gm;
	gm.MainLoop(); 
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