
#ifndef MAIN_H
#define MAIN_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100
#define MAX_JOBS 50

typedef struct
{
    int job_id;
    pid_t pid;
    int stopped;
    char command[MAX_INPUT];
} Job;

extern int last_status;
extern pid_t shell_pid;
extern volatile sig_atomic_t foreground_pgid;
extern char prompt[MAX_INPUT];
extern Job jobs[MAX_JOBS];

char *trim_whitespace(char *str);
int scan_input(char *input, char *argv[]);
int check_command_type(char *command);
int execute_internal(int argc, char *argv[]);
int execute_external(char *argv[], int background,
                     const char *command);
int execute_pipeline(char *input, int background,
                     const char *command);

void signal_handler(int sig);
void add_job(pid_t pid, const char *command, int stopped);
void check_jobs(void);

#endif
