#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "shell.h"

int find_file(char *command, char *path_env);
void execute_command(char **tokens, int token_count);
char **arg_processor(char *arg, int *argc);\
void file_redirect(char *filename, char **tokens, int token_count);
void display_history(int HISTORY_COUNT, char history[MAX_HISTORY][MAX_COMMAND_LEN]);

#endif // EXECUTOR_H
