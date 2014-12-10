#include <iostream>
#include <string>
#include <Windows.h>
#include "GTPAdapter.h"
#include "DbgPipe.h"

using namespace std;
#define STR_VERSION ("0.0.1")
#define STR_NAME ("AgentGo")
#define STR_PRO_VERSION ("2")
#define seqi(a,b) (!stricmp(a,b))
#define seqn(a,b,n) (!strnicmp(a,b,n))

#define GaIntToChar(i)  ((char)((i<='H'-'A')?i+'A':i+'A'+1))
#define GaCharToInt(i)  ((int)((i<='h')?i-'a':i-'a'-1))

void GTPAdapter::MainLoop()
{
	char buf[200];
	int c,a,b;
	float _komi;
	//const char* pchar;
	for(;;)
	{
		c=0;a=0;b=0;_komi=0.0;
		cin.getline(buf,199);
		//MessageBoxA(0,buf,"hhh",32);
		//pchar=buf.c_str();
		if(seqn(buf,"genmove ",8))
		{
			bool played;
			int color=tolower((int)buf[8]);
			if(color=='w')
			{
				played=this->onMove(1,a,b);
			}
			else if(color=='b')
			{
				played=this->onMove(0,a,b);
			}
			else
			{
				cout<<"? "<<"unknown command : "<<buf<<endl<<endl;
				continue;
			}
			if(played)
			{
				dprintf("genmove a= %d ,b= %d\n",a,b);
				bd.put((color=='w'?GO_WHITE:GO_BLACK),a,b);
				cout<<"= "<<GaIntToChar(a)<<b+1<<endl<<endl;
			}
			else
			{
				cout<<"= "<<"pass"<<endl<<endl;
			}
		}
		else if(sscanf(buf,"play %c %c%d",&c,&a,&b)==3)
		{
			a=tolower(a);
			c=tolower(c);
			if(c=='w')
			{
				bd.put(GO_WHITE,GaCharToInt(a),b-1);
				this->onPlay(1,GaCharToInt(a),b-1);
			}
			else if(c=='b')
			{
				bd.put(GO_BLACK,GaCharToInt(a),b-1);
				this->onPlay(0,GaCharToInt(a),b-1);
			}
			else
			{
				cout<<"? "<<"unknown command : "<<buf<<endl<<endl;
				continue;
			}
			cout<<"= "<<endl<<endl;
		}
		else if(sscanf(buf,"play %c pass",&c)==1)
		{
			c=tolower(c);
			if(c=='w' || c=='b')
			{
				cout<<"= "<<endl<<endl;
			}
			else
			{
				cout<<"? "<<"unknown command : "<<buf<<endl<<endl;
				continue;
			}
		}
		else if(sscanf(buf,"komi %f",&_komi)==1)
		{
			this->komi=_komi;
			cout<<"= "<<endl<<endl;
		}
		else if(seqi(buf,"version"))
		{
			cout<<"= "<<STR_VERSION<<endl<<endl;
		}
		else if(seqi(buf,"list_commands"))
		{
			cout<<"= "<<"protocol_version\nname\nversion\nlist_commands\nquit\nboardsize\nclear_board\nkomi\nplay\ngenmove"<<endl<<endl;
		}
		else if(seqi(buf,"name"))
		{
			cout<<"= "<<STR_NAME<<endl<<endl;
		}
		else if(seqi(buf,"protocol_version"))
		{
			cout<<"= "<<STR_PRO_VERSION<<endl<<endl;
		}
		else if(seqn(buf,"boardsize ",10))
		{
			if(strlen(buf)>10)
			{
				this->onBoardSize(atoi(&buf[10]));
			}
			cout<<"="<<endl<<endl;
		}
		else if(seqi(buf,"clear_board"))
		{
			bd.clear();
			this->onClear();
			cout<<"="<<endl<<endl;
		}
		else if(seqi(buf,"quit"))
		{
			cout<<"="<<endl<<endl;
			return;
		}
		else
		{
					
			cout<<"? "<<"unknown command : "<<buf<<endl<<endl;
		}
	}
}