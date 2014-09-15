/*
 * Copyright (C) 2013 Bryan Christ <bryan.christ@mediafire.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */


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

