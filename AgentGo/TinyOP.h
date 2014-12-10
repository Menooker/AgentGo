#ifndef _H_TINYOP_
#define _H_TINYOP_
#include "TinyMT.h"
#include <stdlib.h>


#define ALIGN_TO_LONG(sz) ( ((long)sz/sizeof(long)*sizeof(long))+ ((sz%sizeof(long))?sizeof(long):0))
#define OBJ_TO_NODE(pobj) (Node*)pobj 

template<typename  T> class TinyOP:TinyMT::TObject
{
private:	
	struct Node
	{
		T obj;
		Node* next;
	};
	long freed;
	unsigned size;
	Node* buf;
	Node* p;
public:
	TinyOP(unsigned num)
	{
		buf=(Node*)malloc(num*ALIGN_TO_LONG(sizeof(Node)));
#ifdef TMT_DEBUG
		memset(buf,0xfe,num*ALIGN_TO_LONG(sizeof(Node)));
#endif
		p=buf;
		char* nxt=(char*)p;
		Node* cur;
		for(unsigned i=0;i<num;i++)
		{
			cur=(Node*)nxt;
			nxt=(char*)cur+ALIGN_TO_LONG(sizeof(Node));
			cur->next=(Node*)nxt;
		}
		cur->next=0;
		size=num;freed=num;
	}



	T* alloc()
	{
		if(freed<=0)
		{
			AG_PANIC("Out of memory!!!!!!");
			return 0;
		}
		this->enter();
		Node* ret=p;
		p=ret->next;
		freed--;
		this->leave();
		return &ret->obj;
	}

	void free(T* pobj)
	{
		this->enter();
		Node* ret=OBJ_TO_NODE(pobj);
		ret->next=p;
		p=ret;
#ifdef TMT_DEBUG
		memset(pobj,0xfe,sizeof(T));
#endif
		freed++;
		this->leave();
	}

	void tdelete(T* &po)
	{
		po->~T();
		this->free(po);
		po=0;
	}

	T*  tnew()
	{
		return new (alloc()) T;
	}

	template<typename P1>
	T*  tnew(P1 p1)
	{
		return new (alloc()) T(p1);
	}

	template<typename P1, typename P2>
	T*  tnew(P1 p1, P2 p2)
	{
		return new (alloc()) T(p1, p2);
	}

	template<typename P1, typename P2, typename P3>
	T*  tnew(P1 p1, P2 p2, P3 p3)
	{
		return new (alloc()) T(p1, p2, p3);
	}

	template<typename P1, typename P2, typename P3, typename P4>
	T*  tnew(P1 p1, P2 p2, P3 p3, P4 p4)
	{
		return new (alloc()) T(p1, p2, p3, p4);
	}

	template<typename P1, typename P2, typename P3, typename P4, typename P5>
	T*  tnew(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5)
	{
		return new (alloc()) T(p1, p2, p3, p4, p5);
	}



	~TinyOP()
	{
		::free(buf);
	}
};

#endif



