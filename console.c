/*
 * Copyright (C) 2013 Bryan Christ <bryan.christ@mediafire.com>
 *               2014 Johannes Schauer <j.schauer@email.de>
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
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include "mfshell.h"
#include "console.h"
#include "private.h"
#include "stringv.h"

int
console_control_mode(int mode)
{
    static struct termios   saved_state;
    struct termios          new_state;
    int                     retval = 0;

    switch(mode)
    {
        case CONSOLE_STATE_SAVE:
        {
            retval = tcgetattr(STDIN_FILENO,&saved_state);
            break;
        }

        case CONSOLE_STATE_RESTORE:
        {
            retval = tcsetattr(STDIN_FILENO,TCSANOW,&saved_state);
            break;
        }

        case CONSOLE_ECHO_OFF:
        {
            tcgetattr(STDIN_FILENO,&new_state);
            new_state.c_lflag &= (~ECHO);
            // new_state.c_cc[VERASE] = '\b';
            // new_state.c_lflag &= (~ECHOE);
            tcsetattr(STDIN_FILENO,TCSANOW,&new_state);
            break;
        }

        case CONSOLE_ECHO_ON:
        {
            tcgetattr(STDIN_FILENO,&new_state);
            new_state.c_lflag |= ECHO;
            tcsetattr(STDIN_FILENO,TCSANOW,&new_state);
            break;
        }

        default:
            retval = -1;
    }

    return retval;
}

int
console_get_metrics(int *height,int *width)
{
    struct winsize  metrics;

    if(height == NULL || width == NULL) return -1;

    ioctl(0,TIOCGWINSZ,&metrics);

    *height = metrics.ws_row;
    *width = metrics.ws_col;

    return 0;
}

int
_execute(mfshell_t *mfshell, int argc, char **argv)
{
    _cmd_t* curr_cmd;
    for (curr_cmd = mfshell->commands; curr_cmd->name != NULL; curr_cmd++) {
        if (strcmp(argv[0], curr_cmd->name) == 0) {
            return curr_cmd->handler(mfshell, argc, argv);
        }
    }
}

int
_execute_shell_command(mfshell_t *mfshell,char *command)
{
    extern int      term_resized;
    extern int      term_height;
    extern int      term_width;

    char            **argv = NULL;
    int             argc = 0;
    int             retval;

    if(mfshell == NULL) return -1;
    if(command == NULL) return -1;

    // check to see if the terminal has been resized
    if(term_resized == 1)
    {
        if(console_get_metrics(&term_height,&term_width) == 0)
        {
            term_resized = 1;
        }
    }

    if(term_width < 30)
    {
        printf("[EE] terminal size to small\n\r");
        return -1;
    }

    argv = stringv_split(command," ",5);
    if(argv == NULL) return -1;

    argc = stringv_len(argv);

    // TODO: handle retval
    retval = _execute(mfshell, argc, argv);

    stringv_free(argv,STRINGV_FREE_ALL);

    return 0;
}

