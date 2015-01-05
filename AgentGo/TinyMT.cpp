#include "TinyMT.h"
namespace TinyMT
{

#define ReturnIfNullThread() if(this->tid.id==0){return; }

#ifdef TMT_ON_WINDOWS
	DWORD __stdcall Runner(TThread* p)
	{
		p->run();
		p->closethread();
		return 0;
	}
#endif 

	void TThread::restore()
	{
		if(isLazy)
		{
#ifdef TMT_ON_WINDOWS
			SetThreadPriority(this->tid.h, priority);
#endif
		}
	}
	void TThread::lazy()
	{
		isLazy=true;
#ifdef TMT_ON_WINDOWS
		SetThreadPriority(this->tid.h, TMT_THREAD_LOW);
#endif
	}

	void TThread::startandwait(TJob* j,int priority)
	{
		this->job=j;
#ifdef TMT_ON_WINDOWS
		this->tid.h=CreateThread(0,0,(LPTHREAD_START_ROUTINE)Runner,this,CREATE_SUSPENDED,&this->tid.id);
		SetThreadPriority(this->tid.h, priority);
		this->priority=priority;
		isLazy=false;
		ResumeThread(this->tid.h);
		WaitForSingleObject(this->tid.h,-1);
#endif
	}
	void TThread::startandwait(TJob* j)
	{
		this->startandwait(j,TMT_THREAD_HIGH);
	}

	void TThread::start(TJob* j,int priority)
	{
		this->job=j;
#ifdef TMT_ON_WINDOWS
		this->tid.h=CreateThread(0,0,(LPTHREAD_START_ROUTINE)Runner,this,CREATE_SUSPENDED,&this->tid.id);
		SetThreadPriority(this->tid.h, priority);
		this->priority=priority;
		isLazy=false;
		ResumeThread(this->tid.h);

#endif
	}
	void TThread::start(TJob* j)
	{
		this->start(j,TMT_THREAD_HIGH);
	}

	void TThread::tsleep(long ms)
	{
#ifdef TMT_ON_WINDOWS
		Sleep(ms);
#endif
	}
	void TThread::exit()
	{
#ifdef TMT_ON_WINDOWS
		if(this->tid.id && this->tid.id!=GetCurrentThreadId()) //fix-me : why sometimes this->tid.id equals 0 ?
			return;
		this->closethread();
		ExitThread(0);
#endif
		
	}
	void TThread::suspend()
	{
		ReturnIfNullThread();
#ifdef TMT_ON_WINDOWS
		SuspendThread(this->tid.h);
#endif
	}
	void TThread::resume()
	{
		ReturnIfNullThread();
#ifdef TMT_ON_WINDOWS
		ResumeThread(this->tid.h);
#endif
	}

	bool TThread::alive()
	{
#ifdef TMT_ON_WINDOWS
		DWORD r;
		if(GetExitCodeThread(tid.h ,&r))
		{
			return (r==STILL_ACTIVE);
		}
		else
			return false;
#endif
	}

	void TThread::stop()
	{
		ReturnIfNullThread();
		tid.id=0;
#ifdef TMT_ON_WINDOWS
		if(!TerminateThread(this->tid.h,0))
		{
			dprintf("error when ending thread : %d\n",GetLastError());
		}
#endif
		
	}

	void TObject::enter()
	{
		EnterSync(this->syncblk);
	}
	void TObject::leave()
	{
		LeaveSync(this->syncblk);
	}


	


}
