#ifndef EXECUTOR_H
#define EXECUTOR_H

int find_file(char *command, char *path_env);
void execute_command(char **tokens, int token_count);
char **arg_processor(char *arg, int *argc);\
void file_redirect(char *filename, char **tokens, int token_count);
void add_history(const char *command);

#endif // EXECUTOR_H
