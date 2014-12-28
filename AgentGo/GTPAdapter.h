#ifndef _H_GTPADAPTER_
#define _H_GTPADAPTER_
#include "GoContest\Board.h"

class GTPAdapter
{
protected:
	Board bd;
	float komi;
private:
#ifdef GO_TIME_STAT
	double avgtime; 
	long times;
#endif
	virtual void onClear()=0;
	virtual void onBoardSize(int sz){}
	virtual void onPlay(int isW,int a,int b)=0;
	virtual bool onMove(int isW,int& a,int&b)=0;
	virtual void onMoved(int isW,int a,int b)=0;
public:
	void MainLoop();
	GTPAdapter()
	{
		avgtime=0;times=0;
	}
};

#endif