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


#ifndef _MFSHELL_CONSOLE_H_
#define _MFSHELL_CONSOLE_H_

enum
{
    CONSOLE_STATE_SAVE      = 0x01,
    CONSOLE_STATE_RESTORE,
    CONSOLE_ECHO_OFF,
    CONSOLE_ECHO_ON
};

int     console_control_mode(int mode);

#define console_save_state() \
            console_control_mode(CONSOLE_STATE_SAVE)

#define console_restore_state() \
            console_control_mode(CONSOLE_STATE_RESTORE)

#define console_echo_off() \
            console_control_mode(CONSOLE_ECHO_OFF)

#define console_echo_on() \
            console_control_mode(CONSOLE_ECHO_ON)

int     console_get_metrics(int *height,int *width);

#endif
