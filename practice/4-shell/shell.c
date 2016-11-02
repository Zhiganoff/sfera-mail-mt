#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

typedef struct cmd_inf {
	char ** argv; // список из имени команды и аргументов
	char *infile; // переназначенный файл стандартного ввода
	char *outfile; // переназначенный файл стандартного вывода
	char *outfile2; // дозапись
	int first; // для корректного выхода из рекурсии
	int backgrnd; // =1, если команда подлежит выполнению в фоновом режиме
	struct cmd_inf* psubcmd; // команды для запуска в дочернем shell
	struct cmd_inf* pipe; // следующая команда после “|”
	struct cmd_inf* ifExecuted; // после &&
	struct cmd_inf* ifNotExecuted; // после ||
	struct cmd_inf* next; // следующая после “;” (или после “&”)
} cmd_inf;

void inputString(char **s);
void setStructToDefault(cmd_inf *cmd);
void render(char **command, cmd_inf *cmd, int f);
char *getWord(char **s);
int execute(cmd_inf *cmd);
void conveyor(cmd_inf *cmd);
void freeTree(cmd_inf * cmd);

int main(int argc, char *argv[])
{
	char *command;
    printf("Enter new command\n%%");
    command = NULL;
    inputString(&command);
	while (strcmp(command, "exit") != 0) {
		cmd_inf *first_cmd;
		first_cmd = malloc(sizeof(cmd_inf));
		setStructToDefault( first_cmd );
		char *beginCmd = command;
		render(&command, first_cmd, 0);
		execute(first_cmd);
		command = beginCmd;
		freeTree(first_cmd); /* Удаление дерева */
		printf("Enter new command\n%%");
		inputString(&command);
	}
	free(command);
	return 0;
}

void inputString(char **s)
{
	if (*s) free(*s);
	*s = NULL;
	int size = 10;
    char c;
    *s = (char*) malloc (sizeof(char)* size);
    int i = -1, j = 0;
    c = getchar();
    if (c == EOF) return;
    while ((c != '\n') && (c != '\0') && (c != EOF))
    {
        i++;
        j += i / 10;
        if (i == 10)
            *s = realloc (*s, sizeof(char) * size * (j + 1));
        i %= 10;
        *(*s + j * size + i) = c;
        scanf("%c", &c);
    };
    i++;
    j += i / 10;
    if (i == 10)
        *s = (char*) realloc (*s, sizeof(char) * size * (j + 1));
    i %= 10;
    *(*s + j * size + i) = '\0';
    return;
}

void setStructToDefault(cmd_inf *cmd)
{
	cmd->argv = NULL;
	cmd->infile = NULL;
	cmd->outfile = NULL;
	cmd->outfile2 = NULL;
	cmd->first = 0;
	cmd->backgrnd = 0;
	cmd->psubcmd = NULL;
	cmd->pipe = NULL;
	cmd->ifExecuted = NULL;
	cmd->ifNotExecuted = NULL;
	cmd->next = NULL;
	return;
}

void render(char **command, cmd_inf *cmd, int f)
{
	/* обработка строки */
	int argc = 0;
	while (**command != '\0') {
		char *tmp;
		tmp = getWord(command);
        if (strcmp(tmp, ";") == 0) {
            if (f > 0) {
                cmd->next = malloc(sizeof(cmd_inf));
                setStructToDefault(cmd->next);
                render(command, cmd->next, f);
                break;
            } else return;
        } else
		if (strcmp(tmp, "(") == 0) {
			cmd->psubcmd = malloc(sizeof(cmd_inf));
			setStructToDefault(cmd->psubcmd);
			cmd->psubcmd->first = 1;
			render(command, cmd->psubcmd, f + 1);
		} else
        if (strcmp(tmp, "&&") == 0) {
            cmd->ifExecuted = malloc(sizeof(cmd_inf));
            setStructToDefault(cmd->ifExecuted);
            render(command, cmd->ifExecuted, f + 1);
        } else
        if (strcmp(tmp, "||") == 0) {
            cmd->ifNotExecuted = malloc(sizeof(cmd_inf));
            setStructToDefault(cmd->ifNotExecuted);
            render(command, cmd->ifNotExecuted, f + 1);
        } else
        if (strcmp(tmp, "|") == 0) {
            cmd->pipe = malloc(sizeof(cmd_inf));
            setStructToDefault(cmd->pipe);
            render(command, cmd->pipe, f + 1);
        } else
        if (strcmp(tmp, "<") == 0){
            cmd->infile = malloc(sizeof(char*));
            cmd->infile = NULL;
            cmd->infile = getWord(command);
        } else
        if (strcmp(tmp, ">") == 0){
            cmd->outfile = malloc(sizeof(char*));
            cmd->outfile = NULL;
            cmd->outfile = getWord(command);
        } else
        if (strcmp(tmp, ">>") == 0){
            cmd->outfile2 = malloc(sizeof(char*));
            cmd->outfile2 = NULL;
            cmd->outfile2 = getWord(command);
        } else
        if (strcmp(tmp, ")") == 0){
            if (f > 0) {
                if (!cmd->first) *command -= 2;
                return;
            }
        } else
        if (strcmp(tmp, "&") == 0){
            cmd->backgrnd = 1;
        } else {
			argc++;
			if (argc == 1) cmd->argv = malloc((argc + 1) * sizeof(char *));
				else cmd->argv = realloc(cmd->argv, (argc + 1) * sizeof(char *));
			(cmd->argv)[argc-1] = malloc(strlen(tmp) * sizeof(char));
			strcpy(cmd->argv[argc-1], tmp);
			(cmd->argv)[argc] = NULL;
		}
	}
	return;
}

char *getWord(char **s)
{
	while (**s == ' '){
		(*s)++;
	}
	if (**s == '\0') return NULL;
	int k = 0;
	while (**s != ' ' && **s != '\0') {
		k++;
		(*s)++;
	}
	char *ans;
	ans = (char*) malloc((k + 1) * sizeof(ans));
	strncpy(ans, *s - k, k);
	*(ans + k) = '\0';
	return ans;
}

int execute(cmd_inf *cmd)
{
    if (!cmd->psubcmd && !cmd->argv) return 0;
    if (cmd->pipe) {
        conveyor(cmd);
        return 0;
    }
    if (cmd->psubcmd) {
        int ex_status;
        ex_status = execute(cmd->psubcmd);
        if (cmd->ifExecuted) {
            if (WIFEXITED(ex_status)) ex_status = execute(cmd->ifExecuted);
        } else
        if (cmd->ifNotExecuted) {
            if (!WIFEXITED(ex_status)) ex_status = execute(cmd->ifNotExecuted);
        }
        if (cmd->next) ex_status = execute(cmd->next);
        return ex_status;
    }
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        /* Перенаправление ввода/вывода */
        int fdIn, fdOut;
        if (cmd->infile){
            if ((fdIn = open(cmd->infile, O_RDONLY, 0666)) == -1) {
                printf("Can't open %s\n", cmd->infile);
                return 0;
                }
            dup2(fdIn, STDIN_FILENO);
            close(fdIn);
        }
        if (cmd->outfile) {
            fdOut = open(cmd->outfile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
            dup2(fdOut, STDOUT_FILENO);
            close(fdOut);
        } else
        if (cmd->outfile2){
            fdOut = open(cmd->outfile2, O_CREAT | O_WRONLY | O_APPEND, 0666);
            dup2(fdOut, STDOUT_FILENO);
            close(fdOut);
        }
        /* Исполнение */
        if (cmd->backgrnd) {
            pid_t pid1;
            pid1 = fork();
            if (pid1 == 0) {
                int fd;
                signal (SIGINT, SIG_IGN);
                fd = open("/dev/null", O_RDWR);
                dup2(fd, 0);
                dup2(fd, 1);
                close(fd);
                execvp(cmd->argv[0], cmd->argv);
                }
            exit(0);
            return 0;
        }

        if (cmd->argv) {
            execvp(cmd->argv[0], cmd->argv);
            perror(cmd->argv[0]);
            exit(1);
        }
    }
    //if (cmd->backgrnd) { wait(NULL); return 0;}
    int ex_status;
    wait(&ex_status);
    if (cmd->ifExecuted)
    {
        if (WIFEXITED(ex_status))
            if (!WEXITSTATUS(ex_status))
	            execute(cmd->ifExecuted);
    }
	else
    if (cmd->ifNotExecuted)
    {
        if (!WIFEXITED(ex_status))
	        execute(cmd->ifNotExecuted);
        else
        if (WIFEXITED(ex_status) && WEXITSTATUS(ex_status))
	        execute(cmd->ifNotExecuted);
    }
    if (cmd->next) execute(cmd->next);
    return ex_status;
}

void conveyor(cmd_inf *cmd)
{
    int saveIn = dup(0);
    int fd[2];
    while (cmd->pipe) {
        pipe(fd);
        if (!fork()) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            close(fd[0]);
            execvp(cmd->argv[0], cmd->argv);
            perror(cmd->argv[0]);
            exit(1);
        }
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        cmd = cmd->pipe;
    }
    if (!fork()) {
        execvp(cmd->argv[0], cmd->argv);
    }
    close(fd[0]);
    while (wait(NULL) != -1);
    dup2(saveIn, 0);
    close(saveIn);
    return;
}

void freeTree(cmd_inf *cmd)
{
	if (cmd->argv) {
        int i = 0;
        while (cmd->argv[i]) {
            free(cmd->argv[i]);
            i++;
        }
        free(cmd->argv);
    }
	if (cmd->infile) free(cmd->infile);
	if (cmd->outfile) free(cmd->outfile);
	if (cmd->outfile2) free(cmd->outfile2);
	if (cmd->psubcmd) freeTree(cmd->psubcmd);
	if (cmd->pipe) freeTree(cmd->pipe);
	if (cmd->ifExecuted) freeTree(cmd->ifExecuted);
	if (cmd->ifNotExecuted) freeTree(cmd->ifNotExecuted);
	if (cmd->next) freeTree(cmd->next);
    free(cmd);
    return;
}
