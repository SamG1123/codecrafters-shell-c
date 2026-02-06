#ifndef BUILTINS_H
#define BUILTINS_H

#include "shell.h"

int is_builtin(const char *command);
void handle_echo(char **tokens, int token_count);
void handle_type(char **tokens, int token_count, const char *path_env);
void handle_pwd(void);
void handle_cd(const char *path, const char *HOME);
void handle_history(int count, char *arg, char *path_env, char *history_file);

extern const char *builtin_commands[];
extern char current_history_file[MAX_PATH_LEN];

#endif // BUILTINS_H
