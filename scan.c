
#include "main.h"

char *trim_whitespace(char *str)
{
    while (isspace((unsigned char)*str))
        str++;

    size_t len = strlen(str);

    while (len > 0 &&
           isspace((unsigned char)str[len - 1]))
    {
        str[--len] = '\0';
    }

    return str;
}

int scan_input(char *input, char *argv[])
{
    int argc = 0;
    char *token = strtok(input, " \t");

    while (token != NULL && argc < MAX_ARGS - 1)
    {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }

    argv[argc] = NULL;
    return argc;
}
