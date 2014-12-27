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
public:
	bool can[17][17];
	void MainLoop();
	GTPAdapter()
	{
		memset(can,0,sizeof(bool)*17*17);
		avgtime=0;times=0;
	}
};

#endif

