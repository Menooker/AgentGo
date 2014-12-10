#define AG_DBG
#ifdef AG_DBG
#define AG_PANIC(a) __asm int 3
#else
#define AG_PANIC(a)
#endif