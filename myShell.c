#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
 
typedef struct ListOfStr
{
    char *str;
    struct ListOfStr *next;
} ListOfStr;
 
typedef struct List
{
    char **argv;
    struct List *next;
} List;
 
char *myStrCpy(char s[256])
{
    return strcpy((char *) calloc(strlen(s) + 1, sizeof(char *)), s);
}
 
int scanstr(char s[256])
{
    char c = ' ';
    int i = 0;
    while (c == ' ' && c != '\n')
        read(0, &c, 1);
    while (c != ' ' && c != '\n' && i < 255)
    {
        s[i++] = c;
        read(0, &c, 1);
    }
    s[i] = '\0';
    if (c == '\n')
        return -1;
    else
        return 0;
}
 
void append(ListOfStr **list, char str[256])
{
    ListOfStr *p = *list;
    while (p != NULL && p -> next != NULL)
        p = p -> next;
    if (*list == NULL)
        p = *list = (ListOfStr *) malloc(sizeof(ListOfStr));
    else
    {
        p -> next = (ListOfStr *) malloc(sizeof(ListOfStr));
        p = p -> next;
    }
    p -> str = myStrCpy(str);
    p -> next = NULL;
 
}
 
char **makeargv(ListOfStr *list, int argc)
{
    char **argv;
    ListOfStr *p = list;
    int i = 0;
    argv = (char **) calloc(argc + 1, sizeof(char *));
    while (p != NULL)
    {
        argv[i++] = p -> str;
        p = p -> next;
        free(list);
        list = p;
    }
    argv[i] = NULL;
    return argv;
}
 
void exec(List *list, int stdIn, int stdOut, char isBg)
{
    if (list != NULL)
    {
        if (isBg)
        {
            if (fork())
            {
                wait(NULL);
                return;
            }
            else if (fork())
                _exit(0);
            if (stdIn == 0)
            {
                close(stdIn);
                stdIn = open("/dev/null", O_RDONLY | O_CREAT, 0222);
            }
            if (stdOut == 1)
            {
                close(stdOut);
                stdOut = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0444);
            }
            dup2(stdOut, 2);
        }
        if (list -> next != NULL) // pipeline
        {
            int fd[2];
            while (list -> next != NULL)
            {
                pipe(fd);
                if (!fork())
                {
                    if (isBg)
                        close(stdOut);
                    if (stdIn != 0)
                    {
                        dup2(stdIn, 0);
                        close(stdIn);
                    }
                    dup2(fd[1], 1);
                    close(fd[0]);
                    execvp((list -> argv)[0], list -> argv);
                    perror((list -> argv)[0]);
                    _exit(1);
                }
                if (stdIn != 0)
                    close(stdIn);
                stdIn = fd[0];
                close(fd[1]);
                list = list -> next;
            }
            if (!fork())
            {
                dup2(stdIn, 0);
                close(stdIn);
                if (stdOut != 1)
                {
                    dup2(stdOut, 1);
                    close(stdOut);
                }
                execvp((list -> argv)[0], list -> argv);
                perror((list -> argv)[0]);
                _exit(1);
            }
        }
        else if (isBg || !fork())
        {
            if (stdIn != 0)
            {
                dup2(stdIn, 0);
                close(stdIn);
            }
            if (stdOut != 1)
            {
                dup2(stdOut, 1);
                close(stdOut);
            }
            execvp((list -> argv)[0], list -> argv);
            perror((list -> argv)[0]);
            _exit(1);
 
        }
        while (wait(NULL) != -1);
        if (isBg)
            _exit(0);
    }
}
 
void freeMas(char **mas)
{
    int i = -1;
    while (mas[++i] != NULL)
        free(mas[i]);
    free(mas);
}
 
void freeList(List *list)
{
    if (list != NULL)
    {
        freeList(list -> next);
        freeMas(list -> argv);
        free(list);
    }
}
 
int readline(List **list, int *stdIn, int *stdOut, char *isBg)
{
    char s[256], eol;
    int argc, wmode, scan_res;
    ListOfStr *argl;
    List *nodeOfList;
    scan_res = scanstr(s);
    if (strcmp(s, "exit") != 0)
    {
        *stdIn = *isBg = eol = 0;
        *stdOut = 1;
        *list = NULL;
        if (s[0] == '\0')
            eol = 1;
        while (!eol)
        {
            argc = 0;
            argl = NULL;
            while (!eol && strcmp(s, "|") != 0)
            {
                if (strcmp(s, "<") == 0)
                {
                    scan_res = scanstr(s);
                    *stdIn = open(s, O_RDONLY | O_CREAT, 0777);
                }
                else if (strcmp(s, ">") == 0 || strcmp(s, ">>") == 0)
                {
                    wmode = O_WRONLY | O_CREAT;
                    if (strcmp(s, ">>") == 0)
                        wmode |= O_APPEND;
                    else
                        wmode |= O_TRUNC;
                    scan_res = scanstr(s);
                    *stdOut = open(s, wmode, 0777);
                }
                else if (strcmp(s, "&") == 0)
                    *isBg = 1;
                else
                {
                    argc++;
                    append(&argl, s);
                }
                if (scan_res == -1 || s[0] == '\0')
                    eol = 1;
                else
                {
                    scan_res = scanstr(s);
                    if (s[0] == '\0')
                        eol = 1;
                }
            }
            if (*list == NULL)
                nodeOfList = *list = (List *) malloc(sizeof(List));
            else
            {
                nodeOfList -> next = (List *) malloc(sizeof(List));
                nodeOfList = nodeOfList -> next;
            }
            nodeOfList -> argv = makeargv(argl, argc);
            nodeOfList -> next = NULL;
            if (!eol)
                scan_res = scanstr(s);
        }
        return 0;
    }
    else
        return 1;
}
 
int main()
{
    List *list;
    char isBg;
    int stdIn, stdOut;
    if (!fork())
        execlp("echo", "echo", "-e", "-n", "\033[1m\033[32mmyShell\033[0m:\033[34m\033[1m~\033[0m$ ", NULL);
    else
        wait(NULL);
    while (readline(&list, &stdIn, &stdOut, &isBg) == 0)
    {
        exec(list, stdIn, stdOut, isBg);
        freeList(list);
        if (!fork())
            execlp("echo", "echo", "-e", "-n", "\033[1m\033[32mmyShell\033[0m:\033[34m\033[1m~\033[0m$ ", NULL);
        else
            wait(NULL);
    }
    return 0;
}