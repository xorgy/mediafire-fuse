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
