#ifdef _MSC_VER 
#include <windows.h>

#define localtime_r(_time, _result) _localtime64_s(_result, _time)
#define sleep(x) Sleep((x*1000))
#endif
