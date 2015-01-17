// AgentGo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "TinyMT.h"
#include "GTPAdapter.h"
#include "DbgPipe.h"
#include <time.h>
#include <stdlib.h>
#include "Board.h"
#include "UCTree.h"
#include "TinyOP.h"
#include <stdio.h>
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
class AmafJob:public TJob
{
public:
	Board* bd;
	int i;
	int j;
	int isWh;
	int avrg_win;
	int cstep[13][13];
	int cstep2[13][13];
	AmafJob(Board& old)
	{
		bd=boardpool.tnew(&old);
		//bd=new Board(old);
		for (int i=0;i<13;++i)
		{
			for (int j=0;j<13;++j)
			{
				cstep[i][j]=0;
				cstep2[i][j]=0;
			}
		}
	}
	~AmafJob()
	{
		boardpool.tdelete(bd);
		//delete bd;
	}
};

class NodeScoreJob:public TJob
{
public:
	UCNode* node;
	int isWh;
	int first_player;
	NodeScoreJob(UCNode* nd_ptr)
	{
		node = nd_ptr;
	}
	~NodeScoreJob()
	{
	}
};






class AmafWorker:public SWorker
{
	void work(TJob* j)
	{
		AmafJob* mj=(AmafJob*)j;
		int agent = mj->isWh + 1;
		int enemy = 3 - agent;
		int row = mj->i;
		int col = mj->j;
		Board bd_copy(mj->bd);
		mj->bd->put(agent,mj->i,mj->j);

		int numset = NUMSET_AMAF;
		int num_piece = mj->bd->num_black + mj->bd->num_white;
		double num_a = 1/(index_amaf_a1-index_amaf_a2*num_piece);
		double num_b = index_amaf_b1-index_amaf_b2*num_piece;
		for( int k=0;k<AMAF_RANGE;k++){
			amaf[k] = 1-pow(num_a*(k+1-1),num_b);
			if ( amaf[k]<0 ) amaf[k] = 0;
			else if ( amaf[k]>1 ) amaf[k] = 1;
		}
		int bonus_a = numset*index_a;
		int bonus_b = numset*index_b;
		// degree of conservation, if relative score less than 0, it will be multiplied by this degree
		// the larger the avrg_win the larger cnsv
		double cnsv = pow(2., (mj->avrg_win)*index_cnsv);
		
		// let the two random players to evaluate the situation
		Board bdnew(mj->bd);
		for (int ii=0;ii<numset;++ii)
		{
			mj->bd->clone(bdnew);
			memset(mj->cstep,0,sizeof(int)*13*13);
			memset(mj->cstep2,0,sizeof(int)*13*13);
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
					if(mj->cstep2[rand.row][rand.col]==0) mj->cstep2[rand.row][rand.col]=cstep;
				}
				else{
					enemy_go = false;
					mj->bd->pass(enemy);
				}
				rand=mj->bd->getRandomPiece(agent);
				if (!rand.isEmpty()){
					mj->bd->put(rand);
					agent_go = true;
					if(mj->cstep[rand.row][rand.col]==0) mj->cstep[rand.row][rand.col]=cstep;
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
					if (mj->cstep[i][j]!=0 && mj->cstep[i][j]<=AMAF_RANGE){
						amaf1[i][j] += (100*amaf[mj->cstep[i][j]-1])*score*index_c;
					}
					else if (mj->cstep2[i][j]!=0 && mj->cstep2[i][j]<=AMAF_RANGE){
						amaf2[i][j] -= (100*amaf[mj->cstep2[i][j]-1])*score*index_c;
					}
				}
			}
			total_mark[row][col] += (100)*score*index_c;
			mc[row][col] += (100)*score*index_c;
		}
	}
};

class NodeScoreWorker:public SWorker
{
	void work(TJob* j)
	{
		NodeScoreJob* mj=(NodeScoreJob*)j;
		int agent = mj->isWh + 1;
		int enemy = 3 - agent;
		int numset = NUMSET_NODE_SCORE;

		int sum_win = 0;
		Board bdtest;
		for (int ii=0;ii<numset;++ii){
			bdtest.clone(mj->node->bd);
			bool agent_go = 1;
			bool enemy_go = 1;
			bool started = false;
			while (agent_go || enemy_go){
				Piece rand;
				if( started==true || started==false && mj->first_player==mj->isWh ){
					rand=bdtest.getRandomPiece(agent);
					if (!rand.isEmpty()){
						bdtest.put(rand);
						agent_go = true;
					}
					else{
						agent_go = false;
						bdtest.pass(agent);
					}
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
				started = true;
			}
			sum_win += bdtest.countScore(agent) - bdtest.countScore(enemy);
		}

		mj->node->score = ((double)sum_win)/numset;
	}

};

class MyGame:public GTPAdapter
{
	int step;
	bool can[17][17];
	void boundedWaiting(Scheduler<AmafWorker>* psch)
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
		for(int i=0;i<13;i++)
		{
			for(int j=0;j<13;j++)
			{
				can[i][j]=0;
			}
		}
		step=0;
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

	// this function evaluates the situation for player isW under the situation of bd_base
	int testAvrgWin(const Board &bd_base, int isW, int first_player){
		int sum_win = 0;
		int agent = isW+1;
		int enemy = 3 - agent;
		int numset = NUMSET_PRETEST;
		Board bdtest;
		for (int ii=0;ii<numset;++ii){
			bdtest.clone(bd_base);
			bool agent_go = 1;
			bool enemy_go = 1;
			bool started = false;
			while (agent_go || enemy_go){
				Piece rand;
				if( started==true || started==false && first_player==isW ){
					rand=bdtest.getRandomPiece(agent);
					if (!rand.isEmpty()){
						bdtest.put(rand);
						agent_go = true;
					}
					else{
						agent_go = false;
						bdtest.pass(agent);
					}
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
				started = true;
			}
			sum_win += bdtest.countScore(agent) - bdtest.countScore(enemy);
		}
		//just return an integer rather than a float
		return (sum_win/numset);
	}

	void expandUCNode(UCNode &node, int isW){
		int i,j,k;
		int node_player = node.player;

		memset(amaf1,0,sizeof(double)*169);
		memset(amaf2,0,sizeof(double)*169);
		memset(mc,0,sizeof(double)*169);
		memset(total_mark,0,sizeof(double)*169);
		AmafJob* jobs[13][13]={0};
		Scheduler<AmafWorker>* psch=new Scheduler<AmafWorker>(true);

		// get a rough evalutaion of the situation for the node_player ( maybe not ourselves ), start from node_player
		int avrg_win = testAvrgWin(node.bd, node_player, node_player);
		// submit the jobs, evaluate the relative score for each position for the node_player
		for( i=0; i<13; i++ )
		{
			for( j=0; j<13; j++ )
			{
				if( false
					|| node.bd.checkTrueEye(isW+1,i,j)
					|| node.bd.data[i][j]!=GO_NULL
					|| node.bd.checkSuicide(isW+1,i,j)
					|| node.bd.checkNoSense(isW+1,i,j)
					|| node.bd.checkCompete(isW+1,i,j)
				){
					continue;
				}
				jobs[i][j]=new AmafJob(node.bd);
				jobs[i][j]->i=i;
				jobs[i][j]->j=j;
				jobs[i][j]->isWh=node_player;
				jobs[i][j]->avrg_win=avrg_win;
				psch->submit(jobs[i][j],1);
			}
		}
		// run the threads and wait for the work completes
		psch->go();
		psch->wait();
		delete psch;

		// sort the best choices for the nodeplayer, also delete "jobs"
		double choice_mark[UCT_WIDTH];
		Piece choices[UCT_WIDTH];
		for( i=0; i<UCT_WIDTH; i++ ){
			choice_mark[i] = -1.79E+308;
		}
		for ( i=0; i<13; ++i )
		{
			for ( j=0; j<13; ++j )
			{
				if (node.bd.data[i][j]==GO_NULL && !node.bd.checkTrueEye(isW+1,i,j) && !node.bd.checkSuicide(isW+1,i,j) && !node.bd.checkCompete(isW+1,i,j))
				{
					total_mark[i][j] += mc[i][j];
					total_mark[i][j] += amaf1[i][j];
					total_mark[i][j] += amaf2[i][j];
					double tm = total_mark[i][j];

					if( choice_mark[UCT_WIDTH-1] < tm ){
						choice_mark[UCT_WIDTH-1] = tm;
						choices[UCT_WIDTH-1] = Piece(node_player+1,i,j);
					}
					for( k=UCT_WIDTH-1; k>0; k-- ){
						if( choice_mark[k] > choice_mark[k-1] ){
							double temp_mark = choice_mark[k];
							Piece temp_piece = choices[k];
							choice_mark[k]= choice_mark[k-1];
							choices[k] = choices[k-1];
							choice_mark[k-1] = temp_mark;
							choices[k-1] = temp_piece;
 						}
						else break;
					}
				}
				if(jobs[i][j])
				{
					delete jobs[i][j];
					jobs[i][j]=0;
				}
			}
		}

		// append the choices as children to the node
		node.expand();
		for ( i=0; i<UCT_WIDTH; i++){
			node.children[i]->move = choices[i];
			node.children[i]->initBoard();
		}
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
		// dprintf("Evaluation: %d\n",testAvrgWin(bd,isW,isW));
		// return false;

		/*FILE *out;
		 if ((out = fopen("F:\\\\AgentGO\\hash.txt", "a")) == NULL)
		 {
		 } 
		 else
		 {
		  fprintf(out,"\n--------------------------------\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n--------------------------------\n",
			  bd.getPatternHash(1,0),bd.getPatternHash(5,0),bd.getPatternHash(9,0),
			  bd.getPatternHash(0,4),bd.getPatternHash(0,8),bd.getPatternHash(1,12),
			  bd.getPatternHash(5,12),bd.getPatternHash(9,12),bd.getPatternHash(12,8)
			  );
		 }
		 fclose(out);*/

		//dprintf("11 %d\n",bd.getPatternHash(1,1));
		//dprintf("15 %d\n",bd.getPatternHash(1,5));
		//dprintf("19 %d\n",bd.getPatternHash(1,9));
		//dprintf("51 %d\n",bd.getPatternHash(5,1));
		//dprintf("55 %d\n",bd.getPatternHash(5,5));
		//dprintf("59 %d\n",bd.getPatternHash(5,9));
		//dprintf("91 %d\n",bd.getPatternHash(9,1));
		//dprintf("95 %d\n",bd.getPatternHash(9,5));
		//dprintf("99 %d\n",bd.getPatternHash(9,9));
		/*return false;*/

		bool domove=0;
		step++;
		int own = isW+1;
		if (step==1)
		{
			if (bd.data[3][3]==0)
			{
				a = 3; b = 3; return true;
			}
			else if (bd.data[9][9]==0)
			{
				a = 9; b = 9; return true;
			}
		}
		if (step==2)
		{
			if (bd.data[3][9]==0)
			{
				a = 3; b = 9; return true;
			}
			else if (bd.data[9][3]==0)
			{
				a = 9; b = 3; return true;
			}
			else if (bd.data[9][9]==0)
			{
				a = 9; b = 9; return true;
			}
		}
		if (step>=3 && step<=5)
		{
			if (bd.data[3][9]==0)
			{
				a = 3; b = 9; return true;
			}
			if (bd.data[9][3]==0)
			{
				a = 9; b = 3; return true;
			}
			if (bd.data[3][3]==0)
			{
				a = 3; b = 3; return true;
			}
			if (bd.data[9][9]==0)
			{
				a = 9; b = 9; return true;
			}
			if (bd.data[3][3]==own && bd.data[3][9]==own)
			{
				if (bd.data[3][6]==0)
				{a = 3; b = 6; return true;}
			}
			if (bd.data[3][3]==own && bd.data[9][3]==own)
			{
				if (bd.data[6][3]==0)
				{a = 6; b = 3; return true;}
			}
			if (bd.data[3][9]==own && bd.data[9][9]==own)
			{
				if (bd.data[6][9]==0)
				{a = 6; b = 9; return true;}
			}
			if (bd.data[9][9]==own && bd.data[9][3]==own)
			{
				if (bd.data[9][6]==0)
				{a = 9; b = 6; return true;}
			}//
			if (bd.data[3][3]==own)
			{
				if (bd.data[3][6]==0)
				{a = 3; b = 6; return true;}
				if (bd.data[6][3]==0)
					{a = 6; b = 3; return true;}
			}
			if (bd.data[3][9]==own )
			{
				if (bd.data[3][6]==0)
				{a = 3; b = 6; return true;}
				if (bd.data[6][9]==0)
				{a = 6; b = 9; return true;}
			}
			if (bd.data[9][9]==own )
			{
				if (bd.data[6][9]==0)
				{a = 6; b = 9; return true;}
				if (bd.data[9][6]==0)
				{a = 9; b = 6; return true;}
			}
			if (bd.data[9][3]==own)
			{
				if (bd.data[9][6]==0)
				{a = 9; b = 6; return true;}
				if (bd.data[6][3]==0)
				{a = 6; b = 3; return true;}
			}
			////////
			if (bd.data[3][6]==0)
				{a = 3; b = 6; return true;}
			//
			if (bd.data[6][3]==0)
				{a = 6; b = 3; return true;}
			//
			if (bd.data[6][9]==0)
				{a = 6; b = 9; return true;}
			//
			if (bd.data[9][6]==0)
				{a = 6; b = 6; return true;}
			//
			if (bd.data[6][6]==0)
			{
				{a = 6; b = 6; return true;}
			}//////
			if (bd.data[3][3]==own && bd.data[3][9]==own)
			{
				if (bd.data[5][6]==0)
				{a = 5; b = 6; return true;}
			}//////
			if (bd.data[3][3]==own && bd.data[9][3]==own)
			{
				if (bd.data[6][5]==0)
				{a = 6; b = 5; return true;}
			}//////
			if (bd.data[3][9]==own && bd.data[9][9]==own)
			{
				if (bd.data[6][7]==0)
				{a = 6; b = 7; return true;}
			}//////
			if (bd.data[9][3]==own && bd.data[9][9]==own)
			{
				if (bd.data[7][6]==0)
				{a = 7; b = 6; return true;}
			}//////
			step++;

		}
		if (step>5)
		{
			// construct the uct
			UCTree uct(true,isW,bd);
			expandUCNode(uct.root,isW);
			for( int i=0; i<UCT_WIDTH; i++ ){
				expandUCNode(*uct.root.children[i],isW);
			}

			// mark the score of leaf nodes
			NodeScoreJob* jobs[UCT_WIDTH*UCT_WIDTH]={0};
			Scheduler<NodeScoreWorker>* psch=new Scheduler<NodeScoreWorker>(true);
			for( int i=0; i<UCT_WIDTH; i++ ){
				UCNode* branch = uct.root.children[i];
				for( int j=0; j<UCT_WIDTH; j++ ){
					UCNode* leaf = branch->children[j];
					int job_idx = i*UCT_WIDTH+j;
					jobs[job_idx] = new NodeScoreJob(leaf);
					jobs[job_idx]->isWh = isW;
					jobs[job_idx]->first_player = isW;
					psch->submit(jobs[job_idx],1);
				}
			}
			psch->go();
			psch->wait();
			delete psch;
			for( int i=0; i<UCT_WIDTH*UCT_WIDTH; i++ ){
				delete jobs[i];
				jobs[i] = 0;
			}

			// minmax and make the move
			Piece pc = uct.getBestMove();
			if(!pc.isEmpty()) domove = true;
			a = pc.row;
			b = pc.col;
			uct.clear();
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
	
	/*Board bd;
	bd.put(1,6,7);
	bd.put(2,5,7);
	bd.put(2,7,7);
	int a=bd.getPatternHash(6,6);
	printf("%d",a);*/
	
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
