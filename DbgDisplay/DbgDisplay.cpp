// DbgDisplay.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <Windows.h>
#include <conio.h>

DWORD __stdcall Runner(HANDLE hProcess)
{
	WaitForSingleObject(hProcess, -1);
	ExitProcess(0);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc!=3)
	{
		
		printf("Invalid Arg\n");
		system("pause");
		return 0;
	}
	char out_buffer[4096];
	HANDLE h=(HANDLE)_wtoi(argv[1]);
	HANDLE hProcess=(HANDLE)_wtoi(argv[2]);
	printf("%d\n",hProcess);
	CreateThread(0,0,Runner,hProcess,0,0);

	DWORD dwRead;  
	BOOL bSuccess = FALSE;
	for(;;)
	{
		ZeroMemory(out_buffer,4096);
		bSuccess = ReadFile( h, out_buffer, 4096, &dwRead, NULL);
		if ((bSuccess) && (dwRead!=0))  //如果成功了，且长度>0
		{
			printf(out_buffer);
		}
	}
	return 0;
}

