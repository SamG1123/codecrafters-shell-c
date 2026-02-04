#ifndef SHELL_H
#define SHELL_H

#ifdef _WIN32
#define PATH_LIST_SEPARATOR ';'
#define PATH_LIST_SEPARATOR_STR ";"
#define PATH_SEPARATOR '\\'
#else
#define PATH_LIST_SEPARATOR ':'
#define PATH_LIST_SEPARATOR_STR ":"
#define PATH_SEPARATOR '/'
#endif

#define MAX_COMMAND_LEN 1024
#define MAX_PATH_LEN 1024
#define NUM_BUILTINS 5

#endif // SHELL_H
