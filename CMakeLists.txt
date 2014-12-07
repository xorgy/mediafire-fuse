cmake_minimum_required(VERSION 2.8)

project(mediafire-tools)

# creates file compile_commands.json in build directory which is used
# to feed iwyu during tests
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -Werror")
# the following is until we learn how to reorder the gcc arguments to correctly link on Ubuntu
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")

add_library(mfapi SHARED
	mfapi/mfconn.c
	mfapi/file.c
	mfapi/folder.c
	mfapi/patch.c
	mfapi/apicalls.c
	mfapi/apicalls/file_get_info.c
	mfapi/apicalls/user_get_info.c
	mfapi/apicalls/file_get_links.c
	mfapi/apicalls/file_delete.c
	mfapi/apicalls/user_get_session_token.c
	mfapi/apicalls/folder_get_info.c
	mfapi/apicalls/folder_create.c
	mfapi/apicalls/folder_get_content.c
	mfapi/apicalls/folder_delete.c
	mfapi/apicalls/device_get_status.c
	mfapi/apicalls/device_get_changes.c
	mfapi/apicalls/device_get_patch.c
	mfapi/apicalls/device_get_updates.c
	mfapi/apicalls/upload_simple.c
	mfapi/apicalls/upload_patch.c
	mfapi/apicalls/upload_poll_upload.c
	)

add_library(mfutils SHARED
	utils/http.c
	utils/strings.c
	utils/stringv.c
	utils/xdelta3.c
	utils/hash.c)

add_executable(mediafire-shell
	mfshell/main.c
	mfshell/mfshell.c
	mfshell/commands/folder.c
	mfshell/commands/auth.c
	mfshell/commands/chdir.c
	mfshell/commands/debug.c
	mfshell/commands/file.c
	mfshell/commands/get.c
	mfshell/commands/put.c
	mfshell/commands/help.c
	mfshell/commands/host.c
	mfshell/commands/lcd.c
	mfshell/commands/links.c
	mfshell/commands/list.c
	mfshell/commands/lpwd.c
	mfshell/commands/mkdir.c
	mfshell/commands/pwd.c
	mfshell/commands/whoami.c
	mfshell/commands/rmdir.c
	mfshell/commands/rm.c
	mfshell/commands/status.c
	mfshell/commands/changes.c
	mfshell/config.c
	mfshell/options.c
	mfshell/commands/updates.c)
target_link_libraries(mediafire-shell curl ssl crypto jansson mfapi mfutils)

enable_testing()

add_executable(mediafire-fuse
	fuse/main.c
	fuse/hashtbl.c
	fuse/filecache.c
	fuse/operations.c)
set_target_properties(mediafire-fuse PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
target_link_libraries(mediafire-fuse curl ssl fuse jansson mfapi mfutils)

add_test(iwyu ${CMAKE_SOURCE_DIR}/tests/iwyu.py ${CMAKE_BINARY_DIR})
add_test(indent ${CMAKE_SOURCE_DIR}/tests/indent.sh ${CMAKE_SOURCE_DIR})
add_test(valgrind_fuse ${CMAKE_SOURCE_DIR}/tests/valgrind_fuse.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
add_test(valgrind_shell ${CMAKE_SOURCE_DIR}/tests/valgrind_shell.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
