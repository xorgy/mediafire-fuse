cmake_minimum_required(VERSION 2.8)

project(mediafire-tools)


add_library(mfapi SHARED mfapi/mfconn.c mfapi/file.c mfapi/folder.c mfapi/apicalls/file_get_info.c mfapi/apicalls/user_get_info.c mfapi/apicalls/file_get_links.c mfapi/apicalls/user_session.c mfapi/apicalls/folder_get_info.c mfapi/apicalls/folder_create.c mfapi/apicalls/folder_get_content.c)

add_library(mfutils SHARED utils/http.c  utils/json.c  utils/strings.c  utils/stringv.c)

add_executable(mfshell mfshell/main.c mfshell/mfshell.c mfshell/commands/auth.c mfshell/commands/chdir.c mfshell/commands/debug.c mfshell/commands/file.c mfshell/commands/get.c mfshell/commands/help.c mfshell/commands/host.c mfshell/commands/lcd.c mfshell/commands/links.c mfshell/commands/list.c mfshell/commands/lpwd.c mfshell/commands/mkdir.c mfshell/commands/pwd.c mfshell/commands/whoami.c)

target_link_libraries(mfshell curl ssl crypto jansson mfapi mfutils)
