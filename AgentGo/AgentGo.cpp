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
double index_a = 10000;
double index_b = 1;
double index_c = 1;
double index_amaf_a1 = 40;
double index_amaf_a2 = 39./180;
double index_amaf_b1 = 0.3;
double index_amaf_b2 = 0.3/180;
double index_cnsv = 0.01;
double index_cmp_gap = 2;
double index_cmp_prp = 0.1;

double amaf[AMAF_RANGE];
double amaf1[13][13] = {0};
double amaf2[13][13] = {0};
double mc[13][13] = {0};
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
	int avrg_win;
	int mark[13][13];
	int mark2[13][13];
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
		int agent = mj->isWh + 1;
		int enemy = 3 - agent;
		int row = mj->i;
		int col = mj->j;
		Board bd_copy(mj->bd);
		mj->bd->put(agent,mj->i,mj->j);
		
		int numset=150;
		int num_piece = mj->bd->num_black + mj->bd->num_white;
		double num_a = 1/(index_amaf_a1-index_amaf_a2*num_piece);
		double num_b = index_amaf_b1-index_amaf_b2*num_piece;
		for( int k=0;k<AMAF_RANGE;k++){
			//amaf[k] = 1.0/(k+1);
			amaf[k] = 1-pow(num_a*(k+1-1),num_b);
			if ( amaf[k]<0 ) amaf[k] = 0;
			else if ( amaf[k]>1 ) amaf[k] = 1;
		}
		int bonus_a = numset*index_a;
		int bonus_b = numset*index_b;
		// degree of conservation, if relative score less than 0, it will be multiplied by this degree
		// the larger the avrg_win the larger cnsv
		double cnsv = pow(2., (mj->avrg_win)*index_cnsv);	

		//提子
		//if( bd_copy.checkKill(agent,row,col) ) total_mark[row][col] += bonus_a;
		//死棋
		if( bd_copy.checkDying(agent,row,col) ) total_mark[row][col] -= bonus_a;
		//无意义的边角
		if( bd_copy.checkNoSense(agent,row,col) ) total_mark[row][col] -= bonus_a;
		//逃
		if( bd_copy.checkSurvive(agent,row,col) ) total_mark[row][col] += bonus_b;

		//连
		if (row>=0 && row<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row][col-1]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row>=0 && row<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row][col-1]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row-1>=0 && row-1<=12 && col>=0 && col<=12 && mj->bd->data[row-1][col]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row-1>=0 && row-1<=12 && col>=0 && col<=12 && mj->bd->data[row-1][col]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row>=0 && row<=12 && col+1>=0 && col+1<=12 && mj->bd->data[row][col+1]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row>=0 && row<=12 && col+1>=0 && col+1<=12 && mj->bd->data[row][col+1]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row+1>=0 && row+1<=12 && col>=0 && col<=12 && mj->bd->data[row+1][col]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row+1>=0 && row+1<=12 && col>=0 && col<=12 && mj->bd->data[row+1][col]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		//尖
		if (row+1>=0 && row+1<=12 && col+1>=0 && col+1<=12 && mj->bd->data[row+1][col+1]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row+1>=0 && row+1<=12 && col+1>=0 && col+1<=12 && mj->bd->data[row+1][col+1]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row+1>=0 && row+1<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row+1][col-1]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row+1>=0 && row+1<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row+1][col-1]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row-1>=0 && row-1<=12 && col+1>=0 && col+1<=12 && mj->bd->data[row-1][col+1]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row-1>=0 && row-1<=12 && col+1>=0 && col+1<=12 && mj->bd->data[row-1][col+1]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row-1>=0 && row-1<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row-1][col-1]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row-1>=0 && row-1<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row-1][col-1]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		//飞
		if (row-1>=0 && row-1<=12 && col+2>=0 && col+2<=12 && mj->bd->data[row-1][col+2]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row-1>=0 && row-1<=12 && col+2>=0 && col+2<=12 && mj->bd->data[row-1][col+2]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row-2>=0 && row-2<=12 && col+1>=0 && col+1<=12 && mj->bd->data[row+1][col-2]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row-2>=0 && row-2<=12 && col+1>=0 && col+1<=12 && mj->bd->data[row+1][col-2]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row-1>=0 && row-1<=12 && col-2>=0 && col-2<=12 && mj->bd->data[row-1][col-2]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row-1>=0 && row-1<=12 && col-2>=0 && col-2<=12 && mj->bd->data[row-1][col-2]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row-2>=0 && row-2<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row-2][col-1]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row-2>=0 && row-2<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row-2][col-1]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row+1>=0 && row+1<=12 && col+2>=0 && col+2<=12 && mj->bd->data[row+1][col+2]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row+1>=0 && row+1<=12 && col+2>=0 && col+2<=12 && mj->bd->data[row+1][col+2]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row+2>=0 && row+2<=12 && col-2>=0 && col-2<=12 && mj->bd->data[row+2][col+1]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row+2>=0 && row+2<=12 && col-2>=0 && col-2<=12 && mj->bd->data[row+2][col+1]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row+1>=0 && row+1<=12 && col-2>=0 && col-2<=12 && mj->bd->data[row+1][col-2]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row+1>=0 && row+1<=12 && col-2>=0 && col-2<=12 && mj->bd->data[row+1][col-2]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		if (row+2>=0 && row+2<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row+2][col-1]==agent+1)
		{
			total_mark[row][col]+=bonus_b;
		}
		else if (row+2>=0 && row+2<=12 && col-1>=0 && col-1<=12 && mj->bd->data[row+2][col-1]==enemy)
		{
			total_mark[row][col]-=bonus_b;
		}
		///断
		if (row-1>=0 && row+1<=12 && col-1>=0 && col+1<=12
			&& ((mj->bd->data[row][col+1]==agent || mj->bd->data[row][col-1]==agent) ||
			(mj->bd->data[row-1][col+1]==agent && mj->bd->data[row-1][col-1]==agent) ||
			(mj->bd->data[row+1][col+1]==agent && mj->bd->data[row+1][col-1]==agent)
			)
			&& mj->bd->data[row-1][col]==enemy && mj->bd->data[row+1][col]==enemy)
		{
			total_mark[row][col]+=bonus_b*20;
		}
		if (row-1>=0 && row+1<=12 && col-1>=0 && col+1<=12
			&& ((mj->bd->data[row-1][col]==agent || mj->bd->data[row+1][col]==agent) ||
			(mj->bd->data[row-1][col+1]==agent && mj->bd->data[row+1][col+1]==agent) ||
			(mj->bd->data[row-1][col-1]==agent && mj->bd->data[row+1][col-1]==agent)
			)&& mj->bd->data[row][col+1]==enemy && mj->bd->data[row][col-1]==enemy)
		{
			total_mark[row][col]+=bonus_b*20;
		}
		if (row-1>=0 && row+1<=12 && col-1>=0 && col+1<=12 &&
			mj->bd->data[row+1][col+1]==agent && mj->bd->data[row+1][col]==enemy && mj->bd->data[row][col+1]==enemy &&
			(mj->bd->data[row][col-1]==agent||(mj->bd->data[row-1][col]==agent)))
		{
			total_mark[row][col]+=bonus_b*20;
		}
		if (row-1>=0 && row+1<=12 && col-1>=0 && col+1<=12 &&
			mj->bd->data[row-1][col+1]==agent && mj->bd->data[row-1][col]==enemy && mj->bd->data[row][col+1]==enemy &&
			(mj->bd->data[row][col-1]==agent||(mj->bd->data[row+1][col]==agent)))
		{
			total_mark[row][col]+=bonus_b*20;
		}
		if (row-1>=0 && row+1<=12 && col-1>=0 && col+1<=12 &&
			mj->bd->data[row+1][col-1]==agent && mj->bd->data[row+1][col]==enemy && mj->bd->data[row][col-1]==enemy &&
			(mj->bd->data[row-1][col]==agent||(mj->bd->data[row][col+1]==agent)))
		{
			total_mark[row][col]+=bonus_b*20;
		}
		if (row-1>=0 && row+1<=12 && col-1>=0 && col+1<=12 &&
			mj->bd->data[row-1][col-1]==agent && mj->bd->data[row-1][col]==enemy && mj->bd->data[row][col-1]==enemy &&
			(mj->bd->data[row][col+1]==agent||(mj->bd->data[row+1][col]==agent)))
		{
			total_mark[row][col]+=bonus_b*20;
		}

		// let the two random players to evaluate the situation
		Board bdnew(mj->bd);
		for (int ii=0;ii<numset;++ii)
		{
			mj->bd->clone(bdnew);
			memset(mj->mark,0,sizeof(int)*13*13);
			memset(mj->mark2,0,sizeof(int)*13*13);
			bool agent_go=1;
			bool enemy_go=1;
			int cstep=1;
			int win_rltv = 0; // the score relative to the avrg_win
			while (agent_go || enemy_go)
			{
				cstep++;
				//first move should made by enemy
				Piece rand;
				rand=mj->bd->getRandomPiece(enemy);
				if (!rand.isEmpty()){
					mj->bd->put(rand);
					enemy_go = true;
					if(mj->mark2[rand.row][rand.col]==0) mj->mark2[rand.row][rand.col]=cstep;
				}
				else{
					enemy_go = false;
					mj->bd->pass(enemy);
				}
				rand=mj->bd->getRandomPiece(agent);
				/*int count = 0;
				while( cstep<=2 && count<=5 && abs(rand.row-row)+abs(rand.col-col)>6 ){
					rand = mj->bd->getRandomPiece(agent);
					count++;
				}*/
				if (!rand.isEmpty()){
					mj->bd->put(rand);
					agent_go = true;
					if(mj->mark[rand.row][rand.col]==0) mj->mark[rand.row][rand.col]=cstep;
				}
				else{
					agent_go = false;
					mj->bd->pass(agent);
				}
			}

			win_rltv = mj->bd->countScore(agent) - mj->bd->countScore(enemy) - mj->avrg_win;
			double score = (win_rltv>=0)? win_rltv : win_rltv*cnsv;
			for (int i=0;i<13;++i){
				for (int j=0;j<13;++j){
					if (mj->mark[i][j]!=0 && mj->mark[i][j]<=AMAF_RANGE){
						amaf1[i][j] += (100*amaf[mj->mark[i][j]-1])*score*index_c;
					}
					else if (mj->mark2[i][j]!=0 && mj->mark2[i][j]<=AMAF_RANGE){
						amaf2[i][j] -= (100*amaf[mj->mark2[i][j]-1])*score*index_c;
					}
				}
			}
			total_mark[row][col] += (100)*score*index_c;
			mc[row][col] += (100)*score*index_c;
			//dprintf("%d %d %f %f\n", mj->avrg_win, win_rltv, cnsv, score);

			/*if( row==5 && col==11 ){
				dprintf("10 0 mc:%f %f\n",(100)*score*index_c,total_mark[row][col]);
			}*/
		}
	}
};


class MyGame:public GTPAdapter
{
	int step;
	bool can[17][17];
	void boundedWaiting(Scheduler<MyWorker>* psch)
	{
			if(!psch->wait(5000))
			{
				dprintf("Wait time expired\n");
				psch->abort();
				if(!psch->wait(1000))
				{
					psch->end();
					dprintf("Abort time expired\n");
					for(int cc=0;cc<10;cc++)
					{
						if(!psch->all_threads_dead())
							Sleep(70);
						else
							break;
					}
				}
			}
	}
	void onClear()
	{
		step=0;
		for(int i=0;i<13;i++)
		{
			for(int j=0;j<13;j++)
			{
				can[i][j]=0;
			}
		}
	}
	void onBoardSize(int sz)
	{
	
	}
	void onMoved(int isW,int a,int b)
	{
		for (int i=a;i<a+5;++i)
		{
			for (int j=b;j<b+5;++j)
			{
				can[i][j]=1;
			}
		}
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
	int testAvrgWin(bool isW){
		int numset = 500;
		int sum_win = 0;
		int agent = isW+1;
		int enemy = 3 - agent;
		Board bdtest;
		for (int ii=0;ii<numset;++ii){
			bdtest.clone(bd);
			bool agent_go=1;
			bool enemy_go=1;
			while (agent_go || enemy_go){
				// different from marking, here the first play is made by player himself
				Piece rand;
				rand=bdtest.getRandomPiece(agent);
				if (!rand.isEmpty()){
					bdtest.put(rand);
					agent_go = true;
				}
				else{
					agent_go = false;
					bdtest.pass(agent);
				}
				rand=bdtest.getRandomPiece(enemy);
				if (!rand.isEmpty()){
					bdtest.put(rand);
					enemy_go = true;
				}
				else{
					enemy_go = false;
					bdtest.pass(enemy);
				}
			}
			sum_win += bdtest.countScore(agent) - bdtest.countScore(enemy);
		}
		//just return an integer rather than a float
		return (sum_win/numset);
	}
	bool onMove(int isW,int& a,int& b)
	{
		// play random
		
		/*Piece p = bd.getRandomPiece(isW+1);
		if( p.isEmpty() ) return false;
		else{
			a = p.row;
			b = p.col;
			return true;
		}*/
		
		//step = 5;
		memset(amaf1,0,sizeof(double)*169);
		memset(amaf2,0,sizeof(double)*169);
		memset(mc,0,sizeof(double)*169);
		memset(total_mark,0,sizeof(double)*169);
		bool domove=0;
		step+=1;
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
			int avrg_win = testAvrgWin(isW);
			/////////////submit the jobs
				for( int i=0;i<13;i++)
				{
					for(int j=0;j<13;j++)
					{
						// !can[i+2][j+2] || 
						if( false
							|| bd.checkTrueEye(isW+1,i,j)
							|| bd.data[i][j]!=GO_NULL
							|| bd.checkSuicide(isW+1,i,j)
							|| bd.checkNoSense(isW+1,i,j)
							|| bd.checkCompete(isW+1,i,j)
						){	
							continue;
						}
						jobs[i][j]=new MyJob(bd);
						jobs[i][j]->i=i;
						jobs[i][j]->j=j;
						jobs[i][j]->isWh=isW;
						jobs[i][j]->avrg_win=avrg_win;
						psch->submit(jobs[i][j],1);
					}
				}
			/////run the threads and wait for the work completes
			psch->go();
			psch->wait();
			delete psch;
		
			/////delete "jobs"
			double max_score;
			bool max_init = false;
			int max_i;
			int max_j;
			double avrg_scale_mc = 0;
			double avrg_scale_amaf1 = 0;
			double avrg_scale_amaf2 = 0;
			/*for (int i=0;i<13;++i)
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
							avrg_scale_mc += abs(mc[i][j]/169);
							avrg_scale_amaf1 += abs(amaf1[i][j]/169);
							avrg_scale_amaf2 += abs(amaf2[i][j]/169);
						}
				}
			}*/
			for (int i=0;i<13;++i)
			{
				for (int j=0;j<13;++j)
				{
					if (bd.data[i][j]==GO_NULL && !bd.checkTrueEye(isW+1,i,j) && !bd.checkSuicide(isW+1,i,j) && !bd.checkCompete(isW+1,i,j))
					{
						total_mark[i][j] += mc[i][j];
						total_mark[i][j] += amaf1[i][j];
						total_mark[i][j] += amaf2[i][j];
						/*if( abs(mc[i][j]/avrg_scale_mc - amaf1[i][j]/avrg_scale_amaf1) > index_cmp_gap ) total_mark[i][j] += amaf1[i][j]*index_cmp_prp;
						else total_mark[i][j] += amaf1[i][j];
						if( abs(mc[i][j]/avrg_scale_mc - amaf2[i][j]/avrg_scale_amaf2) > index_cmp_gap ) total_mark[i][j] += amaf2[i][j]*index_cmp_prp;
						else total_mark[i][j] += amaf2[i][j];
						dprintf("cmp %d %d : %f %f \n",i,j,abs(mc[i][j]/avrg_scale_mc - amaf1[i][j]/avrg_scale_amaf1),abs(mc[i][j]/avrg_scale_mc - amaf2[i][j]/avrg_scale_amaf2));
						*/
						if( !max_init || total_mark[i][j]>max_score ){
							max_score = total_mark[i][j];
							max_i = i;
							max_j = j;
							max_init = true;
							domove = true;
						}
					}
					if(jobs[i][j])
					{
						delete jobs[i][j];
						jobs[i][j]=0;
					}
				}
			}
			a = max_i;
			b = max_j;
			if( a==0 || b==0 || a==BOARD_SIZE-1 || b== BOARD_SIZE-1 ){
				dprintf("border!");
			}
			// use random
			/*if( step>3 ){
				Piece p = bd.getRandomPiece(isW+1);
				a = p.row;
				b = p.col;
			}*/
		}
		if (domove)
			return true;
		else
			return false;
	}

};

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc==9)
	{
		index_a=_wtof(argv[1]);
		index_b=_wtof(argv[2]);
		index_c=_wtof(argv[3]);
		index_amaf_a1 = _wtof(argv[4]);
		index_amaf_a2 = _wtof(argv[5]);
		index_amaf_b1 = _wtof(argv[6]);
		index_amaf_b2 = _wtof(argv[7]);
		index_cnsv = _wtof(argv[8]);
	}
	dinitdbg();
	dprintf("%f %f %f %f %f %f %f %f\n",index_a,index_b,index_c,
		index_amaf_a1,index_amaf_a2,index_amaf_b1,index_amaf_b2,index_cnsv);
	dprintf("sz bd %d\n",sizeof(Board));
	srand(time(0));
	MyGame gm;
	gm.MainLoop();
	/*
	Board bd;
	bd.put(2,1,0);
	bd.put(2,1,1);
	bd.put(2,0,1);
	bd.put(1,2,0);
	bd.put(1,2,1);
	bd.put(1,2,2);
	bd.put(1,1,2);
	bd.put(1,0,2);
	bool a=bd.checkSuicide(1,0,0);
	printf("%d",a);
	*/
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