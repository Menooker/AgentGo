#include <Windows.h>
#include <stdio.h> 
#include <stdarg.h> 
#include "DbgPipe.h"

static HANDLE hRead=0;
static HANDLE hWrite=0;

void InitDbgConsole()
{
	WCHAR CommandLine[200];
	STARTUPINFO siStartInfo;  
  //����һ�����ڲ����ӽ��̵�PROCESS_INFORMATION�ṹ�� �������CreateProcess,����˵����  
    PROCESS_INFORMATION piProcInfo; 

	SECURITY_ATTRIBUTES  sec_att;     
    sec_att.bInheritHandle       = TRUE;     
    sec_att.lpSecurityDescriptor = NULL;     
    sec_att.nLength              = sizeof(SECURITY_ATTRIBUTES);  

	if   (!CreatePipe(&hRead, &hWrite,&sec_att, 0))  
          return;  

	HANDLE hCurrentProcess;
	DuplicateHandle(GetCurrentProcess(),GetCurrentProcess(),GetCurrentProcess(),&hCurrentProcess, 0, TRUE, DUPLICATE_SAME_ACCESS);
	wsprintfW(CommandLine,L"DbgDisplay.exe %d %d",hRead,hCurrentProcess);
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );  
	siStartInfo.cb = sizeof(STARTUPINFO);  
	siStartInfo.wShowWindow=TRUE;//�˳�Ա��ΪTRUE�Ļ�����ʾ�½����̵�������
	siStartInfo.dwFlags  |= STARTF_USESHOWWINDOW;  
	/*siStartInfo.dwFlags  |= STARTF_USESTDHANDLES;  
	siStartInfo.hStdOutput = 0;     //��˼�ǣ��ӽ��̵�stdout�����hStdOutWrite  
	siStartInfo.hStdError  =  0;        //��˼�ǣ��ӽ��̵�stderr�����hStdErrWrite  
	siStartInfo.hStdInput  = hRead;  */
  
	// �����ӽ��̣��������˵����CreateProcess����  
	int bSuccess = CreateProcess(NULL,  
      CommandLine,    // �ӽ��̵�������  
      NULL,                   // process security attributes  
      NULL,                   // primary thread security attributes  
      TRUE,                   // handles are inherited  
      CREATE_NEW_CONSOLE,                          // creation flags  
      NULL,                  // use parent's environment  
      NULL,                  // use parent's current directory  
      &siStartInfo,      // STARTUPINFO pointer  
      &piProcInfo);     // receives PROCESS_INFORMATION  
	
   //���ʧ�ܣ��˳�  
	if (!bSuccess )
		return;  
	WaitForInputIdle(piProcInfo.hProcess,100);


}
int DpPrintf(char* str,...)
{
	va_list args;
	va_start( args, str );
	char buf[4*1024];
	vsprintf_s(buf,4*1024,str,args);
	DWORD dwWritten;
	WriteFile( hWrite, buf, strlen(buf)+1, &dwWritten, NULL);  
	FlushFileBuffers(hWrite);
	return 0;
}