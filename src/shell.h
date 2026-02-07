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
#define NUM_BUILTINS 6
#define MAX_PIPES 10
#define MAX_HISTORY 100
#define MAX_COMMANDS (MAX_PIPES + 1)
#define APPEND_CHECKPOINT 0
extern int STDOUT_REDIRECT;
extern int STDERR_REDIRECT;

#endif // SHELL_H
