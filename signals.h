#ifndef _MFSHELL_SIGNALS_H_
#define _MFSHELL_SINGALS_H_

#include <signal.h>

// int     sig_handler_SIGWINCH(int signum,siginfo_t *info,void *context);

void    sig_handler_SIGWINCH(int signum);

void    sig_install_SIGWINCH(void);

#endif
