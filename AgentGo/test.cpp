#include "TinyMT.h"

using namespace TinyMT;
class MyWorker2:public SWorker
{
public:
	void work(TJob* j)
	{
		
	}
};
int s()
{
	TJob j[100];
	Scheduler sch(10,false);
	for( int i=0;i<100;i++)
	{

		//sch.submit(&j[i]);
		//t[i].start(&j[i]);
	}
	sch.go();
	system("pause");
	sch.end();
	return 0;
}