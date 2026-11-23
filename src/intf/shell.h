#pragma once

/*
 * Interactive command shell. Type `help` for the list of commands.
 */

/* Prompt, read a line, run it - forever. */
__attribute__((noreturn)) void shell_run(void);

/*
 * Run one command line (modified in place). Returns 0 on success, 1 if the
 * command reported an error, -1 for an unknown command or too many arguments.
 */
int shell_execute(char* line);

/*
 * Split `line` in place on spaces/tabs into at most `max` words.
 * Returns the number of words, or -1 if there are more than `max`.
 */
int shell_split(char* line, char** argv, int max);
