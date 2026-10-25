
#include "main.h"

int check_command_type(char *command)
{
    const char *builtins[] =
    {
        "exit",
        "cd",
        "pwd",
        "echo",
        "jobs",
        "fg",
        "bg",
        NULL
    };

    for (int i = 0; builtins[i] != NULL; i++)
    {
        if (strcmp(command, builtins[i]) == 0)
            return 1;
    }

    return 0;
}
