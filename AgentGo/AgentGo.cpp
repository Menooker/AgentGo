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

double total_mark[13][13];
double index_a=1000;
double index_b=1;
double index_c=1;
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
	int isWh;
	int mark[13][13];
	MyJob(Board& old)
	{
		bd=boardpool.tnew(&old);
		//bd=new Board(old);
		for (int i=0;i<13;++i)
		{
			for (int j=0;j<13;++j)
			{
				mark[i][j]=0;
			}
		}
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
		int num_b_1=mj->bd->num_black;
		int num_w_1=mj->bd->num_white;
		mj->bd->put(mj->isWh+1,mj->i,mj->j);
		int num_b_2=mj->bd->num_black;
		int num_w_2=mj->bd->num_white;
		int run=2-mj->isWh;
		int numset=150;
					//即时利益(吃子优化)
			if (mj->isWh==1)
			{
				total_mark[mj->i][mj->j]+=(num_w_2-num_w_1+num_b_1-num_b_2)*index_a;
			}
			else
			{
				total_mark[mj->i][mj->j]-=(num_w_2-num_w_1+num_b_1-num_b_2)*index_a;
			}
				//连
				if (mj->i>=0 && mj->i<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i][mj->j-1]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i>=0 && mj->i<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i][mj->j-1]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i-1>=0 && mj->i-1<=12 && mj->j>=0 && mj->j<=12 && mj->bd->data[mj->i-1][mj->j]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i-1>=0 && mj->i-1<=12 && mj->j>=0 && mj->j<=12 && mj->bd->data[mj->i-1][mj->j]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i>=0 && mj->i<=12 && mj->j+1>=0 && mj->j+1<=12 && mj->bd->data[mj->i][mj->j+1]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i>=0 && mj->i<=12 && mj->j+1>=0 && mj->j+1<=12 && mj->bd->data[mj->i][mj->j+1]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i+1>=0 && mj->i+1<=12 && mj->j>=0 && mj->j<=12 && mj->bd->data[mj->i+1][mj->j]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i+1>=0 && mj->i+1<=12 && mj->j>=0 && mj->j<=12 && mj->bd->data[mj->i+1][mj->j]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				//尖
				if (mj->i+1>=0 && mj->i+1<=12 && mj->j+1>=0 && mj->j+1<=12 && mj->bd->data[mj->i+1][mj->j+1]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i+1>=0 && mj->i+1<=12 && mj->j+1>=0 && mj->j+1<=12 && mj->bd->data[mj->i+1][mj->j+1]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i+1>=0 && mj->i+1<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i+1][mj->j-1]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i+1>=0 && mj->i+1<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i+1][mj->j-1]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i-1>=0 && mj->i-1<=12 && mj->j+1>=0 && mj->j+1<=12 && mj->bd->data[mj->i-1][mj->j+1]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i-1>=0 && mj->i-1<=12 && mj->j+1>=0 && mj->j+1<=12 && mj->bd->data[mj->i-1][mj->j+1]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i-1>=0 && mj->i-1<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i-1][mj->j-1]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i-1>=0 && mj->i-1<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i-1][mj->j-1]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				//飞
				if (mj->i-1>=0 && mj->i-1<=12 && mj->j+2>=0 && mj->j+2<=12 && mj->bd->data[mj->i-1][mj->j+2]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i-1>=0 && mj->i-1<=12 && mj->j+2>=0 && mj->j+2<=12 && mj->bd->data[mj->i-1][mj->j+2]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i-2>=0 && mj->i-2<=12 && mj->j+1>=0 && mj->j+1<=12 && mj->bd->data[mj->i+1][mj->j-2]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i-2>=0 && mj->i-2<=12 && mj->j+1>=0 && mj->j+1<=12 && mj->bd->data[mj->i+1][mj->j-2]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i-1>=0 && mj->i-1<=12 && mj->j-2>=0 && mj->j-2<=12 && mj->bd->data[mj->i-1][mj->j-2]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i-1>=0 && mj->i-1<=12 && mj->j-2>=0 && mj->j-2<=12 && mj->bd->data[mj->i-1][mj->j-2]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i-2>=0 && mj->i-2<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i-2][mj->j-1]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i-2>=0 && mj->i-2<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i-2][mj->j-1]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i+1>=0 && mj->i+1<=12 && mj->j+2>=0 && mj->j+2<=12 && mj->bd->data[mj->i+1][mj->j+2]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i+1>=0 && mj->i+1<=12 && mj->j+2>=0 && mj->j+2<=12 && mj->bd->data[mj->i+1][mj->j+2]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i+2>=0 && mj->i+2<=12 && mj->j-2>=0 && mj->j-2<=12 && mj->bd->data[mj->i+2][mj->j+1]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i+2>=0 && mj->i+2<=12 && mj->j-2>=0 && mj->j-2<=12 && mj->bd->data[mj->i+2][mj->j+1]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i+1>=0 && mj->i+1<=12 && mj->j-2>=0 && mj->j-2<=12 && mj->bd->data[mj->i+1][mj->j-2]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i+1>=0 && mj->i+1<=12 && mj->j-2>=0 && mj->j-2<=12 && mj->bd->data[mj->i+1][mj->j-2]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				if (mj->i+2>=0 && mj->i+2<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i+2][mj->j-1]==mj->isWh+1)
				{
					total_mark[mj->i][mj->j]+=index_b;
				}
				else if (mj->i+2>=0 && mj->i+2<=12 && mj->j-1>=0 && mj->j-1<=12 && mj->bd->data[mj->i+2][mj->j-1]==run)
				{
					total_mark[mj->i][mj->j]-=index_b;
				}
				//点
				//
				if (mj->i>=2 && mj->i<=10 && mj->j>=2 && mj->j<=10 && 
					(  (mj->bd->data[mj->i-2][mj->j]==run && mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i-1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i][mj->j+2]==run && mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i][mj->j+1]==GO_NULL)
					|| (mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i+2][mj->j]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i+1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i][mj->j-2]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i][mj->j-1]==GO_NULL)
					))
				{
					dprintf("i=%d,j=%d,yes\n",mj->i,mj->j);
					total_mark[mj->i][mj->j]+=500000;
				}
				//1
				if (mj->i==1 && mj->j>=2 && mj->j<=10 && 
					(  ( 1 && mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i-1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i][mj->j+2]==run && mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i][mj->j+1]==GO_NULL)
					|| (mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i+2][mj->j]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i+1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i][mj->j-2]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i][mj->j-1]==GO_NULL)
					))
				{
					dprintf("i=%d,j=%d,yes\n",mj->i,mj->j);
					total_mark[mj->i][mj->j]+=500000;
				}
				if (mj->i==11 && mj->j>=2 && mj->j<=10 && 
					(  (mj->bd->data[mj->i-2][mj->j]==run && mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i-1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i][mj->j+2]==run && mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i][mj->j+1]==GO_NULL)
					|| (mj->bd->data[mj->i+1][mj->j+1]==run && 1 && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i+1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i][mj->j-2]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i][mj->j-1]==GO_NULL)
					))
				{
					dprintf("i=%d,j=%d,yes\n",mj->i,mj->j);
					total_mark[mj->i][mj->j]+=500000;
				}
				if (mj->i>=2 && mj->i<=10 && mj->j==1 && 
					(  (mj->bd->data[mj->i-2][mj->j]==run && mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i-1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i][mj->j+2]==run && mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i][mj->j+1]==GO_NULL)
					|| (mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i+2][mj->j]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i+1][mj->j-1]==GO_NULL)
					|| (mj->bd->data[mj->i-1][mj->j-1]==run && 1 && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i][mj->j]==GO_NULL)
					))
				{
					dprintf("i=%d,j=%d,yes\n",mj->i,mj->j);
					total_mark[mj->i][mj->j]+=500000;
				}
				if (mj->i>=2 && mj->i<=10 && mj->j==11 && 
					(  (mj->bd->data[mj->i-2][mj->j]==run && mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i-1][mj->j]==GO_NULL)
					|| (1 && mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i][mj->j+1]==GO_NULL)
					|| (mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i+2][mj->j]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i+1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i][mj->j-2]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i][mj->j-1]==GO_NULL)
					))
				{
					dprintf("i=%d,j=%d,yes\n",mj->i,mj->j);
					total_mark[mj->i][mj->j]+=500000;
				}
				//0
				if (mj->i==0 && mj->j>=2 && mj->j<=10 && 
					(  ( 0)
					|| (mj->bd->data[mj->i][mj->j+2]==run && mj->bd->data[mj->i+1][mj->j+1]==run && 1 &&mj->bd->data[mj->i][mj->j+1]==GO_NULL)
					|| (mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i+2][mj->j]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i+1][mj->j]==GO_NULL)
					|| (1 && mj->bd->data[mj->i][mj->j-2]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i][mj->j-1]==GO_NULL)
					))
				{
					dprintf("i=%d,j=%d,yes\n",mj->i,mj->j);
					total_mark[mj->i][mj->j]+=500000;
				}
				if (mj->i==12 && mj->j>=2 && mj->j<=10 && 
					(  (mj->bd->data[mj->i-2][mj->j]==run && mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i-1][mj->j+1]==run&&mj->bd->data[mj->i-1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i][mj->j+2]==run && 1 && mj->bd->data[mj->i-1][mj->j+1]==run&&mj->bd->data[mj->i][mj->j+1]==GO_NULL)
					|| (0)
					|| (mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i][mj->j-2]==run && 1&&mj->bd->data[mj->i][mj->j-1]==GO_NULL)
					))
				{
					dprintf("i=%d,j=%d,yes\n",mj->i,mj->j);
					total_mark[mj->i][mj->j]+=500000;
				}
				if (mj->i>=2 && mj->i<=10 && mj->j==0 && 
					(  (mj->bd->data[mj->i-2][mj->j]==run && 1 && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i-1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i][mj->j+2]==run && mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i-1][mj->j+1]==run &&mj->bd->data[mj->i][mj->j+1]==GO_NULL)
					|| (mj->bd->data[mj->i+1][mj->j+1]==run && mj->bd->data[mj->i+2][mj->j]==run && 1 &&mj->bd->data[mj->i+1][mj->j]==GO_NULL)
					|| (0)
					))
				{
					dprintf("i=%d,j=%d,yes\n",mj->i,mj->j);
					total_mark[mj->i][mj->j]+=500000;
				}
				if (mj->i>=2 && mj->i<=10 && mj->j==12 && 
					(  (mj->bd->data[mj->i-2][mj->j]==run && mj->bd->data[mj->i-1][mj->j-1]==run && 1 &&mj->bd->data[mj->i-1][mj->j]==GO_NULL)
					|| (0)
					|| (1 && mj->bd->data[mj->i+2][mj->j]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i+1][mj->j]==GO_NULL)
					|| (mj->bd->data[mj->i-1][mj->j-1]==run && mj->bd->data[mj->i][mj->j-2]==run && mj->bd->data[mj->i+1][mj->j-1]==run &&mj->bd->data[mj->i][mj->j-1]==GO_NULL)
					))
				{
					dprintf("i=%d,j=%d,yes\n",mj->i,mj->j);
					total_mark[mj->i][mj->j]+=500000;
				}
				//
		Board bdnew(mj->bd);
		for (int ii=0;ii<numset;++ii)
		{
			mj->bd->clone(bdnew);
			memset(mj->mark,0,sizeof(int)*13*13);
			bool white_go=1;
			bool black_go=1;
			if (mj->isWh==1)
			{
				int cstep=0;
				while (white_go || black_go)
				{
					Piece rand;
					rand=mj->bd->getRandomPiece(GO_BLACK);
					if (!rand.isEmpty())
					{
						mj->bd->put(rand);//hei fang xia
						black_go=1;
					}
					else
					{
						black_go=0;
						mj->bd->pass(GO_BLACK);
					}
					rand=mj->bd->getRandomPiece(GO_WHITE);
					if (!rand.isEmpty())
					{
						mj->bd->put(rand);//bai fang xia
						cstep++;
						white_go=1;
						mj->mark[rand.row][rand.col]=cstep;
					}
					else
					{
						white_go=0;
						mj->bd->pass(GO_WHITE);
					}
				}
				int white_win=mj->bd->num_white-mj->bd->num_black;

				for (int i=0;i<13;++i)
				{
					for (int j=0;j<13;++j)
					{
						if (mj->mark[i][j]!=0)
						{
							total_mark[i][j]+=(100/mj->mark[i][j])*white_win*index_c;
						}

					}
				}
			}
			else
			{
				int cstep=0;
				while (white_go || black_go)
				{
					Piece rand;
					//mj->bd->print();
					//AG_PANIC();
					rand=mj->bd->getRandomPiece(GO_WHITE);
					if (!rand.isEmpty())
					{
						mj->bd->put(rand);//hei fang xia
						white_go=1;
					}
					else
					{
						white_go=0;
						mj->bd->pass(GO_WHITE);
					}
					rand=mj->bd->getRandomPiece(GO_BLACK);
					if (!rand.isEmpty())
					{
						mj->bd->put(rand);//bai fang xia
						cstep++;
						black_go=1;
						mj->mark[rand.row][rand.col]=cstep;
					}
					else
					{
						black_go=0;
						mj->bd->pass(GO_BLACK);
					}
				}
				int black_win=mj->bd->num_black-mj->bd->num_white;

				for (int i=0;i<13;++i)
				{
					for (int j=0;j<13;++j)
					{
						if (mj->mark[i][j]!=0)
						{
							total_mark[i][j]+=(100/mj->mark[i][j])*black_win*index_c;
						}

					}
				}
			}
		}
	}
};


class MyGame:public GTPAdapter
{
	int step;
	void onClear()
	{
		step=0;
	}
	void onBoardSize(int sz)
	{
	
	}
	void onPlay(int isW,int a,int b)
	{
		for (int i=a;i<a+5;++i)
		{
			for (int j=b;j<b+5;++j)
			{
				can[i][j]=1;
			}
		}
		dprintf("isW %d, a= %d ,b= %d\n",isW,a,b);
	}
	bool onMove(int isW,int& a,int& b)
	{
		double tmp;
		bool domove=0;
		step+=1;
		for(int i=0;i<13;i++)
		{
			for(int j=0;j<13;j++)
			{
				total_mark[i][j]=1000000.0;
			}
		}
		if (step>=1 && step<=4)
		{
			int takereg[9]={0};
				int i,j;
				for (i=0;i<4;++i)
				{
					for (j=0;j<4;++j)
					{
						if (bd.data[i][j]!=0)
							takereg[0]=1;
					}
					for (j=4;j<9;++j)
					{
						if (bd.data[i][j]!=0)
							takereg[1]=1;
					}
					for (j=9;j<13;++j)
					{
						if (bd.data[i][j]!=0)
							takereg[2]=1;
					}
				}
				for (i=4;i<9;++i)
				{
					for (j=0;j<4;++j)
					{
						if (bd.data[i][j]!=0)
							takereg[3]=1;
					}
					for (j=4;j<9;++j)
					{
						if (bd.data[i][j]!=0)
							takereg[4]=1;
					}
					for (j=9;j<13;++j)
					{
						if (bd.data[i][j]!=0)
							takereg[5]=1;
					}
				}
				for (i=9;i<13;++i)
				{
					for (j=0;j<4;++j)
					{
						if (bd.data[i][j]!=0)
							takereg[6]=1;
					}
					for (j=4;j<9;++j)
					{
						if (bd.data[i][j]!=0)
							takereg[7]=1;
					}
					for (j=9;j<13;++j)
					{
						if (bd.data[i][j]!=0)
							takereg[8]=1;
					}
				}
				if (takereg[0]==0)
				{
					a=3;
					b=3;
				}
				else if (takereg[8]==0)
				{
					a=9;
					b=9;
				}
				else if (takereg[2]==0)
				{
					a=3;
					b=9;
				}
				else if (takereg[6]==0)
				{
					a=9;
					b=3;
				}
				else if (takereg[1]==0)
				{
					a=3;
					b=5;
				}
				else if (takereg[7]==0)
				{
					a=9;
					b=7;
				}
				else if (takereg[3]==0)
				{
					a=7;
					b=3;
				}
				else if (takereg[5]==0)
				{
					a=5;
					b=9;
				}
				else
				{
					a=6;
					b=6;
				}
			return true;
		}
		else if (step>4)
		{
			MyJob* jobs[13][13]={0};
			Scheduler<MyWorker>* psch=new Scheduler<MyWorker>(true);

			/////////////submit the jobs
				for( int i=0;i<13;i++)
				{
					for(int j=0;j<13;j++)
					{
						if(!can[i+2][j+2] || bd.checkTrueEye(isW+1,i,j) || bd.data[i][j]!=GO_NULL || bd.checkSuicide(isW+1,i,j) 
							|| bd.checkCompete(isW+1,i,j))
							continue;
						jobs[i][j]=new MyJob(bd);
						jobs[i][j]->i=i;
						jobs[i][j]->j=j;
						jobs[i][j]->isWh=isW;
						psch->submit(jobs[i][j],1);
					}
				}

			/////run the threads and wait for the work completes
			psch->go();
			psch->wait();
			delete psch;
		
			/////delete "jobs"
			int tmp_i;
			int tmp_j;
			for (int i=0;i<13;++i)
			{
				for (int j=0;j<13;++j)
				{
						if (!bd.checkTrueEye(isW+1,i,j) && bd.data[i][j]==GO_NULL && !bd.checkSuicide(isW+1,i,j) && !bd.checkCompete(isW+1,i,j))
						{
							tmp=total_mark[i][j];
							tmp_i=i;
							tmp_j=j;
							domove=1;
							break;
						}
				}
				if (domove)
				{
					break;
				}
			}

			for (int i=0;i<13;++i)
			{
				for (int j=0;j<13;++j)
				{
					//dprintf("i=%d,j=%d mark=%f\n",i,j,total_mark[i][j]);//////////////////////////////////////////////////////////////////////////////////
					if (total_mark[i][j]>tmp)
					{
						if (!bd.checkTrueEye(isW+1,i,j) && bd.data[i][j]==GO_NULL && !bd.checkSuicide(isW+1,i,j) && !bd.checkCompete(isW+1,i,j))
						{
							tmp=total_mark[i][j];
							tmp_i=i;
							tmp_j=j;
						}
					}
					if(jobs[i][j])
					{
						delete jobs[i][j];
						jobs[i][j]=0;
					}
				}
			}
			a=tmp_i;
			b=tmp_j;
		}
		if (domove)
			return true;
		else
			return false;
	}

};

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc==4)
	{
		index_a=_wtof(argv[1]);
		index_b=_wtof(argv[2]);
		index_c=_wtof(argv[3]);
	}
	dinitdbg();
	dprintf("%f %f %f\n",index_a,index_b,index_c);
	dprintf("sz bd %d\n",sizeof(Board));
	srand(time(0));
	MyGame gm;
	gm.MainLoop(); 
	return 0;
}
