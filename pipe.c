
#include "main.h"

int execute_pipeline(char *input, int background, const char *command)
{
    char *commands[MAX_ARGS];
    pid_t pids[MAX_ARGS];
    int pipes[MAX_ARGS][2];
    int count = 0;
    int i, j;
    pid_t pgid = 0;

    char *token = strtok(input, "|");

    /* Separate commands using the pipe symbol */
    while (token != NULL && count < MAX_ARGS)
    {
        token = trim_whitespace(token);

        if (strlen(token) == 0)
        {
            printf("Invalid pipe command\n");
            return -1;
        }

        commands[count++] = token;
        token = strtok(NULL, "|");
    }

    if (count < 2)
    {
        printf("Invalid pipeline\n");
        return -1;
    }

    /* Create the required pipes */
    for (i = 0; i < count - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("pipe");

            for (j = 0; j < i; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            return -1;
        }
    }

    /* Create a child process for each command */
    for (i = 0; i < count; i++)
    {
        pids[i] = fork();

        if (pids[i] == -1)
        {
            perror("fork");

            for (j = 0; j < count - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            return -1;
        }

        if (pids[i] == 0)
        {
            /* Put all pipeline processes in one process group */
            if (pgid == 0)
                setpgid(0, 0);
            else
                setpgid(0, pgid);

            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);

            /* Read input from the previous command */
            if (i > 0)
                dup2(pipes[i - 1][0], STDIN_FILENO);

            /* Send output to the next command */
            if (i < count - 1)
                dup2(pipes[i][1], STDOUT_FILENO);

            /* Close all pipe file descriptors */
            for (j = 0; j < count - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            char *argv[MAX_ARGS];
            char command_copy[MAX_INPUT];

            snprintf(command_copy, sizeof(command_copy),
                     "%s", commands[i]);

            if (scan_input(command_copy, argv) == 0)
                _exit(1);

            execvp(argv[0], argv);

            perror(argv[0]);
            _exit(127);
        }
        else
        {
            /* Parent process */
            if (pgid == 0)
                pgid = pids[i];

            setpgid(pids[i], pgid);
        }
    }

    /* Parent closes all pipe descriptors */
    for (i = 0; i < count - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /* Background pipeline */
    if (background)
    {
        add_job(pgid, command, 0);
        return 0;
    }

    /* Foreground pipeline */
    foreground_pgid = pgid;

    int status = 0;
    int last_command_status = 0;

    for (i = 0; i < count; i++)
    {
        if (waitpid(pids[i], &status, WUNTRACED) == -1)
        {
            if (errno == EINTR)
            {
                i--;
                continue;
            }

            perror("waitpid");
            continue;
        }

        if (WIFSTOPPED(status))
        {
            add_job(pgid, command, 1);
        }

        if (i == count - 1)
        {
            if (WIFEXITED(status))
                last_command_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                last_command_status = 128 + WTERMSIG(status);
        }
    }

    foreground_pgid = 0;

    return last_command_status;
}
