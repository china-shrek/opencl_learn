#ifndef _STD_AFX_H__
#define _STD_AFX_H__

#include "Poco/Thread.h"
#include "Poco/Mutex.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Task.h"
#include "Poco/Stopwatch.h"
using namespace Poco;
using namespace Poco::Util;

#include <string>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

extern Util::LayeredConfiguration *g_cfg;
extern Logger *g_log;

#define TRY try{
#define CATCH }catch (Exception &e){ g_log->warning("[%s][%d][%s][%d]", string(__FUNCTION__), int(__LINE__), e.displayText(), e.code()); }
//#define LOGGING_TASK() g_log->information("line=%d func=%s id=%d exit file=%s", (int)__LINE__, string(__FUNCTION__), (int)Thread::currentTid(),string(__FILE__));
#define LOGGING_TASK()

#define WATCH_FUNC_TIME(name) (sw##name.elapsed())

#define WATCH_FUNC_BEGIN(name) { Stopwatch sw##name; sw##name.start();g_log->information("******Trace Time Cost[line=%d name=%s]*********begin",(int)__LINE__, string(#name));

#define WATCH_FUNC_END(name) sw##name.stop(); g_log->information("******Trace Time Cost[line=%d name=%s cost=%?i/ms]*********end",(int)__LINE__, string(#name),WATCH_FUNC_TIME(name));}

#define SAFE_DEL(ptr) if(ptr){delete ptr;ptr=NULL;}

#define INIT_WITH_TMPL


#endif //_STD_AFX_H__