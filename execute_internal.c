
#include "main.h"

static int find_job(char *arg)
{
    if (arg == NULL)
        return -1;

    int id = atoi(arg[0] == '%' ? arg + 1 : arg);

    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].pid > 0 && jobs[i].job_id == id)
            return i;
    }

    return -1;
}

int execute_internal(int argc, char *argv[])
{
    if (argc == 0)
        return 1;

    /* Exit Mini Shell */
    if (strcmp(argv[0], "exit") == 0)
        return 2;

    /* Change directory */
    if (strcmp(argv[0], "cd") == 0)
    {
        char *path = argc > 1 ? argv[1] : getenv("HOME");

        if (path == NULL || chdir(path) == -1)
        {
            perror("cd");
            last_status = 1;
        }
        else
        {
            last_status = 0;
        }

        return 1;
    }

    /* Print current directory */
    if (strcmp(argv[0], "pwd") == 0)
    {
        char cwd[MAX_INPUT];

        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s\n", cwd);
            last_status = 0;
        }
        else
        {
            perror("pwd");
            last_status = 1;
        }

        return 1;
    }

    /* Echo arguments and special variables */
    if (strcmp(argv[0], "echo") == 0)
    {
        for (int i = 1; i < argc; i++)
        {
            printf("%s", argv[i]);

            if (argv[i + 1] != NULL)
                printf(" ");
        }

        printf("\n");
        last_status = 0;
        return 1;
    }

    /* Display background and stopped jobs */
    if (strcmp(argv[0], "jobs") == 0)
    {
        for (int i = 0; i < MAX_JOBS; i++)
        {
            if (jobs[i].pid > 0)
            {
                printf("[%d] %s %s\n",
                       jobs[i].job_id,
                       jobs[i].stopped ? "Stopped" : "Running",
                       jobs[i].command);
            }
        }

        return 1;
    }

    /* Resume a job in the background or foreground */
    if (strcmp(argv[0], "bg") == 0 ||
        strcmp(argv[0], "fg") == 0)
    {
        int index = find_job(argc > 1 ? argv[1] : NULL);

        if (index == -1)
        {
            fprintf(stderr, "%s: job not found\n", argv[0]);
            last_status = 1;
            return 1;
        }

        pid_t pid = jobs[index].pid;

        if (kill(-pid, SIGCONT) == -1)
        {
            perror(argv[0]);
            last_status = 1;
            return 1;
        }

        jobs[index].stopped = 0;

        if (strcmp(argv[0], "bg") == 0)
        {
            printf("[%d] %s &\n",
                   jobs[index].job_id,
                   jobs[index].command);
        }
        else
        {
            foreground_pgid = pid;

            int status;
            pid_t result;

            do
            {
                result = waitpid(pid, &status, WUNTRACED);
            }
            while (result == -1 && errno == EINTR);

            foreground_pgid = -1;

            if (result == -1)
            {
                perror("waitpid");
                last_status = 1;
            }
            else if (WIFSTOPPED(status))
            {
                jobs[index].stopped = 1;
            }
            else
            {
                jobs[index].pid = 0;

                if (WIFEXITED(status))
                    last_status = WEXITSTATUS(status);
                else if (WIFSIGNALED(status))
                    last_status = 128 + WTERMSIG(status);
            }
        }

        return 1;
    }

    return 0;
}
