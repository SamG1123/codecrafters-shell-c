#ifndef BUILTINS_H
#define BUILTINS_H

int is_builtin(const char *command);
void handle_echo(const char *command);
void handle_type(const char *command, const char *path_env);
void handle_pwd(void);
void handle_cd(const char *path, const char *HOME);

extern const char *builtin_commands[];

#endif // BUILTINS_H
