#ifndef EXECUTOR_H
#define EXECUTOR_H

int find_file(char *command, char *path_env);
void execute_command(char *command);
char **arg_processor(char *arg, int *argc);

#endif // EXECUTOR_H
