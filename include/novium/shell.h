#ifndef NOVIUM_SHELL_H
#define NOVIUM_SHELL_H

void shell_print_motd(void);
void shell_prompt(void);
void shell_exec(int argc, char **argv);
void shell_task(void);

#endif
