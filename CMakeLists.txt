cmake_minimum_required(VERSION 2.8)

project(mediafire-tools)

set(mf-sources account.c cmd_auth.c cmd_chdir.c cmd_debug.c cmd_file.c cmd_get.c cmd_help.c cmd_host.c cmd_lcd.c cmd_links.c cmd_list.c cmd_lpwd.c cmd_mkdir.c cmd_pwd.c cmd_whoami.c connection.c console.c download.c file.c file_info.c file_links.c folder.c folder_create.c folder_info.c json.c keys.c list.c mfshell.c signals.c signature.c strings.c stringv.c user_session.c)
set(mf-headers account.h chdir.h command.h connection.h console.h download.h file_info.h file_links.h folder_create.h folder_info.h json.h list.h macros.h mfshell.h private.h signals.h strings.h stringv.h user_session.h)

add_library(mf-obj OBJECT ${mf-sources} ${mf-headers})

add_executable(mfshell main.c $<TARGET_OBJECTS:mf-obj>)
target_link_libraries(mfshell curl ssl crypto jansson)
