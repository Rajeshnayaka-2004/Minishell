
#include "main.h"

/* Handle Ctrl+C and Ctrl+Z */
void signal_handler(int sig)
{
    int saved_errno = errno;

    pid_t pgid = (pid_t)foreground_pgid;

    if (pgid > 0)
    {
        if (sig == SIGINT)
            kill(-pgid, SIGINT);
        else if (sig == SIGTSTP)
            kill(-pgid, SIGTSTP);
    }
    else if (sig == SIGINT)
    {
        write(STDOUT_FILENO, "\n", 1);
    }

    errno = saved_errno;
}

/* Add a background or stopped job */
void add_job(pid_t pid, const char *command, int stopped)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].pid == 0)
        {
            jobs[i].job_id = i + 1;
            jobs[i].pid = pid;
            jobs[i].stopped = stopped;

            snprintf(jobs[i].command,
                     sizeof(jobs[i].command),
                     "%s", command);

            printf("[%d] %ld\n",
                   jobs[i].job_id, (long)pid);
            return;
        }
    }

    fprintf(stderr, "minishell: job table is full\n");
}

/* Update the state of child processes */
void check_jobs(void)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status,
                          WNOHANG | WUNTRACED |
                          WCONTINUED)) > 0)
    {
        for (int i = 0; i < MAX_JOBS; i++)
        {
            if (jobs[i].pid != pid)
                continue;

            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                printf("\n[%d] Done %s\n",
                       jobs[i].job_id,
                       jobs[i].command);

                jobs[i].pid = 0;
                jobs[i].stopped = 0;
            }
            else if (WIFSTOPPED(status))
            {
                jobs[i].stopped = 1;

                printf("\n[%d] Stopped %s\n",
                       jobs[i].job_id,
                       jobs[i].command);
            }
            else if (WIFCONTINUED(status))
            {
                jobs[i].stopped = 0;
            }
        }
    }
}
