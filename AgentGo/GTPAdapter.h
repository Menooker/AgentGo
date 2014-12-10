#ifndef _H_GTPADAPTER_
#define _H_GTPADAPTER_
#include "GoContest\Board.h"

class GTPAdapter
{
protected:
	Board bd;
	float komi;
private:
	virtual void onClear()=0;
	virtual void onBoardSize(int sz){}
	virtual void onPlay(int isW,int a,int b)=0;
	virtual bool onMove(int isW,int& a,int&b)=0;
public:
	void MainLoop();

};

#endif