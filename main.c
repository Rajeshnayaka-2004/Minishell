
#include "main.h"

int last_status = 0;
pid_t shell_pid;
volatile sig_atomic_t foreground_pgid = -1;
char prompt[MAX_INPUT] = "minishell$ ";
Job jobs[MAX_JOBS] = {0};

static void expand_variables(const char *src, char *dst, size_t size)
{
    size_t i = 0, j = 0;

    while (src[i] && j < size - 1)
    {
        char value[128] = "";

        if (src[i] == '$' && src[i + 1] == '$')
        {
            snprintf(value, sizeof(value), "%ld", (long)shell_pid);
            i += 2;
        }
        else if (src[i] == '$' && src[i + 1] == '?')
        {
            snprintf(value, sizeof(value), "%d", last_status);
            i += 2;
        }
        else if (strncmp(src + i, "$SHELL", 6) == 0)
        {
            snprintf(value, sizeof(value), "%s", "./minishell");
            i += 6;
        }
        else if (src[i] == '$' &&
                 (isalpha((unsigned char)src[i + 1]) ||
                  src[i + 1] == '_'))
        {
            char name[128];
            size_t k = 0;
            i++;

            while ((isalnum((unsigned char)src[i]) ||
                    src[i] == '_') &&
                   k < sizeof(name) - 1)
                name[k++] = src[i++];

            name[k] = '\0';

            const char *env = getenv(name);
            if (env)
                snprintf(value, sizeof(value), "%s", env);
        }
        else
        {
            dst[j++] = src[i++];
            continue;
        }

        size_t len = strlen(value);

        if (len > size - j - 1)
            len = size - j - 1;

        memcpy(dst + j, value, len);
        j += len;
    }

    dst[j] = '\0';
}

int main(void)
{
    char *input = NULL;
    size_t capacity = 0;

    shell_pid = getpid();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
    signal(SIGQUIT, SIG_IGN);

    while (1)
    {
        check_jobs();

        printf("%s", prompt);
        fflush(stdout);

        ssize_t n = getline(&input, &capacity, stdin);

        if (n < 0)
        {
            putchar('\n');
            break;
        }

        if (n > 0 && input[n - 1] == '\n')
            input[n - 1] = '\0';

        char *cmd = trim_whitespace(input);

        if (*cmd == '\0')
            continue;

        /* PS1 assignment must have no spaces around '=' */
        if (strncmp(cmd, "PS1=", 4) == 0)
        {
            snprintf(prompt, sizeof(prompt), "%s ", cmd + 4);
            continue;
        }

        char original[MAX_INPUT];
        char expanded[MAX_INPUT];

        snprintf(original, sizeof(original), "%s", cmd);
        expand_variables(cmd, expanded, sizeof(expanded));

        int background = 0;
        size_t len = strlen(expanded);

        if (len > 0 && expanded[len - 1] == '&')
        {
            background = 1;
            expanded[len - 1] = '\0';
        }

        char *clean = trim_whitespace(expanded);

        if (strchr(clean, '|') != NULL)
        {
            last_status = execute_pipeline(clean, background, original);
            continue;
        }

        char *argv[MAX_ARGS];
        int argc = scan_input(clean, argv);

        if (argc == 0)
            continue;

        if (check_command_type(argv[0]))
        {
            int result = execute_internal(argc, argv);

            if (result == 2)
                break;
        }
        else
        {
            last_status = execute_external(argv, background, original);
        }
    }

    free(input);
    return 0;
}
