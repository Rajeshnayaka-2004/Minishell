
#include "main.h"

int execute_external(char *argv[], int background,
                     const char *command)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    /* Child process */
    if (pid == 0)
    {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        /* Give the child its own process group */
        if (setpgid(0, 0) == -1)
            _exit(1);

        /* Replace child process with requested command */
        execvp(argv[0], argv);

        /* Reached only if execvp fails */
        perror(argv[0]);

        _exit(errno == ENOENT ? 127 : 126);
    }

    /* Parent process */
    setpgid(pid, pid);

    /* Background command */
    if (background)
    {
        add_job(pid, command, 0);
        return 0;
    }

    /* Foreground command */
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
        return 1;
    }

    /* Child stopped by a signal such as Ctrl+Z */
    if (WIFSTOPPED(status))
    {
        add_job(pid, command, 1);
        return 128 + WSTOPSIG(status);
    }

    /* Child completed normally */
    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    /* Child terminated by a signal */
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return 0;
}
