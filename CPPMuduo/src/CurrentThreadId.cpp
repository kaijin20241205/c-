#include "./../include/CurrentThreadId.hpp"

#include <unistd.h>                 // pid_t、syscall
#include <sys/syscall.h>            // SYS_gettid


namespace CurrentThread
{
    thread_local int t_cachedThreadId  = 0;
    void cachedThreadid()
    {
        if (t_cachedThreadId == 0)
        {
            t_cachedThreadId = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}