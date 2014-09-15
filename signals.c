#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "signals.h"

void
sig_install_SIGWINCH(void)
{
    signal(SIGWINCH,sig_handler_SIGWINCH);

    return;
}

void
sig_handler_SIGWINCH(int signum)
{
    extern int  term_resized;

    term_resized = 1;

    return;
}


/*
void
sig_hanlder_SIGWINCH(int signum,siginfo_t *info,void *context)
{
    extern int  term_resized;

    term_resized = 1;

    return;
}*/


/*
void
sig_install_SIGWINCH(void)
{
    static struct sigaction     handler;

    memset(&handler,0,sizeof(handler));

    handler.sa_sigaction = sig_hanlder_SIGWINCH;
    sigfillset(&handler.sa_mask);
    handler.sa_flags = SA_SIGINFO;
    sigaction(SIGWINCH,&handler,NULL);

    return;
}
*/

