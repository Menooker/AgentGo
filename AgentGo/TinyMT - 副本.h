#include "TinyMTConfig.h"
#include <vector>
#include <list>

#ifdef TMT_ON_WINDOWS
#include <Windows.h>

#define TMTSleep(s) Sleep(s)
typedef struct{
	HANDLE h;
	DWORD id;
}ThreadID;

#define InitSyncBlk(b)  InitializeCriticalSection(&b)
#define DelSyncBlk(b) DeleteCriticalSection(&b)
#define SyncBlk CRITICAL_SECTION  
#define EnterSync(b) EnterCriticalSection(&b)
#define LeaveSync(b) LeaveCriticalSection(&b)
#define TMT_THREAD_NORMAL THREAD_PRIORITY_NORMAL
#define TMT_THREAD_HIGH THREAD_PRIORITY_HIGHEST
#define TMT_THREAD_LOW THREAD_PRIORITY_LOWEST
#define TMT_EVENT HANDLE
#define InitEvent() CreateEventW(0,0,0,0)
#define DelEvent(e) CloseHandle(e)
#define WaitEvent(e) WaitForSingleObject(e,-1)
#define NotifyEvent(e) SetEvent(e)
#endif

namespace TinyMT{


class TEvent
{
private:
	TMT_EVENT mevent;
public:
	TEvent()
	{
		mevent=InitEvent();
	}
	~TEvent()
	{
		DelEvent(mevent);
	}
	void wait()
	{
		WaitEvent(mevent);
	}
	void notify()
	{
		NotifyEvent(mevent);
	}
};

class TObject
{
private:
	SyncBlk syncblk;
public:
	TObject()
	{
		InitSyncBlk(syncblk);
	}
	~TObject()
	{
		DelSyncBlk(syncblk);
	}
	void enter();
	void leave();
	//void wait();
	//void notify();
};
class Finalizer
{
public:
	TObject* o;
	Finalizer(TObject* ob)
	{
		o=ob;
		(o)->enter();
	}
	~Finalizer()
	{
		o->leave();
	}
};
#define sync(o) {Finalizer _finally_(o);
#define esync }

class TJob
{

};

struct JobItem
{
	TJob* j;
	long load;
};
class Joblist:public TObject
{
public:
	std::list<JobItem> list;
public:
	int length()
	{
		return list.size();
	}

	unsigned long load;
	Joblist()
	{
		load=0;
	}
	bool isEmpty()
	{
		return list.empty();
	}
	TJob* head()
	{
		return list.begin()->j;
	}
	TJob* fetch()  // assume the current thread owns the object && list not empty
	{
		//if(isEmpty())
		//	throw 0;
		std::list<JobItem>::iterator it=list.begin();
		TJob* j=it->j;
		load -= it->load;
		list.erase(it);
		return j;
	}
	void push(TJob* j)
	{
		load+=50;
		JobItem i={j,50};
		list.push_back(i);
	}
	void push(TJob* j,unsigned long loads)
	{
		load+=loads;
		JobItem i={j,loads};
		list.push_back(i);
	}
};

class TThread
{
protected:
	TJob* job;
	ThreadID tid;
	long priority;
	bool isLazy;
public:
	void lazy(); //set priority to lowest

	void restore(); //restore the priority before lazy()
	void closethread()
	{
		CloseHandle(tid.h);
		tid.h=0;
		tid.id=0;
		isLazy=false;
	}

	long gettid()
	{
		return tid.id;
	}
	TThread()
	{
		tid.h=0;
		tid.id=0;
		isLazy=false;
	}

	~TThread()
	{
		if(tid.h)
		{
			TerminateThread(tid.h,0);
			CloseHandle(tid.h);
		}
	}
	bool isRunning()
	{
		return tid.id!=0 ;
	}

	void suspend();
	void resume();
	void stop();
	void exit();

	static void tsleep(long ms);
	void start(TJob* j);
	void TThread::start(TJob* j,int priority);
	void TThread::startandwait(TJob* j,int priority);
	void TThread::startandwait(TJob* j);
	virtual void run()=0;
};


class TWorker:public TThread
{
private:
	bool active;

public:
	Joblist joblist;
	void stop()
	{
		active=false;
		TThread::stop();
	}

	void exit()
	{
		active=false; //fix-me : check if it is the current thread
		TThread::exit();
	}

	bool isActive()
	{
		return active && this->isRunning();
	}
	void idle()
	{
		active=false;
		lazy();
	}
	TWorker()
	{
		active=true;
	}
	virtual void askwork()
	{
		TMTSleep(0);
	}

	void push(TJob* j)
	{
		joblist.enter();
		joblist.push(j);
		joblist.leave();
	}
	void push(TJob* j,unsigned long loads)
	{
		joblist.enter();
		joblist.push(j,loads);
		joblist.leave();
	}

	unsigned long getload()
	{
		return joblist.load;
	}
	virtual void work(TJob* j)=0;
	void run()
	{
		TJob* j;
		for(;;)
		{
			for(;;)
			{
				joblist.enter();
				if(joblist.isEmpty())
				{
					joblist.leave();
					break;
				}
				j=joblist.fetch();
				joblist.leave();
				work(j);
				//delete j;
			}
			//printf("TID ask:%d\n",this->gettid());
			askwork();
			//printf("TID:%d ask ok\n",this->gettid());			
		}
		
	}
};


template<typename  T> class Scheduler;
class SWorker;


template<typename  T> class Scheduler
{
private:
	TObject endstate;
	TEvent ev;
	bool running;
	bool terminate_when_empty;
	class AutoSchedule:public TThread
	{
		void run()
		{
			for(;;)
			{

			}
		}
	};
	AutoSchedule autothread;
	class OpJob:public TJob
	{
	public:
		int op;
		Scheduler<T>* sch;
		OpJob(int op,Scheduler<T>* sch)
		{
			this->op=op;
			this->sch=sch;
		}
	};
	class OperationThread:public TThread
	{
		void run()
		{
			OpJob* op=(OpJob*)this->job ;
			if(op->op==1)
			{
				op->sch->pause();
			}
			delete op;
		}
	};
	OperationThread opthread;
	int cnt;
	std::vector<SWorker*> workers;

	bool only_one_worker_running()
	{
		bool one=false;
		for(int i=0;i<cnt;i++)
		{
			if(workers[i]->isActive())
			{
				if(one)
					return false;
				else
					one=true;
			}
		}
		return true;
	}

	int get_a_worker()
	{
		unsigned long mi=0xFFFFFFFF,t;
		int index=0;
		for(int i=0;i<cnt;i++)
		{
			t=workers[i]->getload();
			if(t<mi)
			{
				index=i;
				mi=t;
			}
		}
		return index;
	}
public:

	void wait()
	{
		ev.wait();
	}

	void askjob(T* worker)
	{
		/*if(only_one_worker_running())
			ev.notify();
		worker->exit();*/
		bool noactive=true;
		int le,max=-1,mcnt;
		Joblist* jl=0;
		for(;;) 
		{
			noactive=true;
			max=-1;
			//worker->joblist.enter();
			
				for(int i=0;i<cnt;i++)
				{
					if(workers[i]==worker)
						continue;
					workers[i]->joblist.enter();
					if(workers[i]->isActive())
					{	
						T* worker=workers[i];
						noactive=false;
						le=workers[i]->joblist.length();
						if(le>0 && le>max)
						{
								max=le;jl=&workers[i]->joblist;
						}	
					}
					workers[i]->joblist.leave();
				}
				if(noactive) 
					//terminate_when_empty mode may not enter this,because the last two threads may 
					//recognize each other as a active thread/
				{
					printf("END>>>>>>>>>>>>>>>>>>>>>>>>>>>%d\n",only_one_worker_running());
					if(terminate_when_empty)
					{
						ev.notify();
						worker->exit();
					}
					else
					{
						endstate.enter(); //only one thread can pause the workers
	//					this->running=false;
						ev.notify();
						OpJob* op=new OpJob(1,this); //call the end()
						opthread.startandwait(op);
						endstate.leave();
						return;
					}
				}
				if(jl==0) //no jobs to do
				{
					//printf("No job\n");
					if(terminate_when_empty)
					{
						if(only_one_worker_running())
						{
							ev.notify();
						}
						worker->exit();
					}
					worker->idle();
					//worker->joblist.leave();
					TMTSleep(0);
					return;
				}
				jl->enter();
				le=jl->length();
				if(le>0)
				{
					std::list<JobItem> tmplst;
					mcnt=le/2+ (le%2);
					for(int j=0;j<mcnt;j++)
					{
						
						tmplst.push_back(jl->list.back());
						jl->list.pop_back();
					}
					printf(">>>>>>>>>>>>>Steal %d jobs\n",mcnt);
					jl->leave();

					worker->joblist.enter(); 
					for(int j=0;j<mcnt;j++)
					{
						
						worker->joblist.list.push_back(tmplst.back());
						tmplst.pop_back();
					}
					worker->restore();
					worker->joblist.leave();
					return ;
				}
				else
				{
					//rarely happens
					jl->leave();
					continue;
				}
				
			//worker->joblist.leave();
		}
	}



	void InitScheduler(int worker_cnt,bool terminate_when_empty)
	{
		this->terminate_when_empty=terminate_when_empty;
		running=false;
		cnt=worker_cnt;
		for(int i=0;i<worker_cnt;i++)
		{
			workers.push_back(new T());
			workers.back()->set_scheduler(this);
		}
	}

	Scheduler(int worker_cnt,bool terminate_when_empty)
	{
		InitScheduler(worker_cnt,terminate_when_empty);
	}

	Scheduler(int worker_cnt)
	{
		InitScheduler(worker_cnt,true);
	}

	~Scheduler()
	{
		for(int i=0;i<cnt;i++)
		{
			delete workers[i];
		}
	}

	

	void go()
	{
		if(running)
			throw 0;
		running=true;
		//autothread.start(0,TMT_THREAD_LOW);
		for(int i=0;i<cnt;i++)
		{
			workers[i]->start(0);
		}
	}
	void end()
	{
		if(running)
		{
			running=false;
			//autothread.stop();
			for(int i=0;i<cnt;i++)
			{
				workers[i]->stop();
			}
		}
	}



	void pause()
	{
		//if(running)
		//{
			running=false;
			//autothread.suspend();
			for(int i=0;i<cnt;i++)
			{
				workers[i]->suspend();
			}
		//}
	}
	void resume()
	{
		if(!running)
		{

			//autothread.suspend();
			for(int i=0;i<cnt;i++)
			{
				workers[i]->resume();
			}
			running=false;
		}
	}

	void assign(TJob* j,int index)
	{
		T* wk=(T*)workers[index];
		wk->restore();
		wk->push(j);
	}

	void submit(TJob* j)
	{
		T* wk=(T*)workers[get_a_worker()];
		wk->restore(); //activate the thread
		wk->push(j);

	}
	void submit(TJob* j,unsigned long load)
	{
		T* wk=(T*)workers[get_a_worker()];
		wk->restore();
		wk->push(j,load);
	}
};


class SWorker:public TWorker
{
private:
	Scheduler<SWorker>*  scheduler;
public:
	void set_scheduler(void*  s)
	{
		scheduler=(Scheduler<SWorker>*)s;
	}

	virtual void askwork()
	{
		scheduler->askjob(this);
	}
};

}