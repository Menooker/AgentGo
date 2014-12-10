//����CPU���ĸ���
#if !defined (_WIN32) && !defined (_WIN64)
#define LINUX
#include <sysconf.h>
#else
#define WINDOWS
#include <windows.h>
#endif
unsigned core_count()
{
  unsigned count = 1; // ����һ��
  #if defined (LINUX)
  count = sysconf(_SC_NPROCESSORS_CONF);
  #elif defined (WINDOWS)
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  count = si.dwNumberOfProcessors;
  #endif

  return count;
}
