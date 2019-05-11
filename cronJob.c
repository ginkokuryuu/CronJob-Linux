#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>
// ASCII * = 42

char *dataDir = "/home/nmhhabiby/Sisop/fp/crontab.data";

struct data
{
    int cronTime[5];
    double timeDiff;
    char filePath[100];
    char afterTime[100];
    char program[100];
    int done;
    char line[100];
};

int GetLine(char string[], FILE *file);

void GetData(char line[], struct data *struk);

void *CronJob(void *ar);

int main()
{
    pid_t pid, sid;

    pid = fork();

    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    sid = setsid();

    if (sid < 0)
    {
        exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0)
    {
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    FILE *cron;
    struct data *struk;
    struk = malloc(10 * sizeof(struct data));
    pthread_t threads[10];
    struct stat fileStat;
    int lastCount = -1;
    time_t lastTime, modifTime;
    time(&lastTime);
    char line[100];
    int count;

    while (1)
    {
        stat(dataDir, &fileStat);
        modifTime = fileStat.st_mtime;
        if (modifTime != lastTime)
        {
            cron = fopen(dataDir, "r");
            fseek(cron, 0, SEEK_SET);
            lastTime = modifTime;
            count = 0;
            while (GetLine(line, cron) != -1)
            {
                count++;
                struct data *pstruk = struk + count;
                strcpy(pstruk->line, line);
                GetData(line, pstruk);
                if (lastCount < count)
                {
                    pthread_create(&threads[count], NULL, CronJob, (void *)pstruk);
                }
            }
            int x;
            for(x = count; x <= lastCount; x++){
                struct data *temp = struk + x;
                temp->done = 1;
            }
            lastCount = count;
            fclose(cron);
            cron = NULL;
        }
        sleep(1);
    }

    exit(EXIT_SUCCESS);
}

int GetLine(char string[], FILE *file)
{
    char c;
    int i = 0;
    memset(string, '\0', 100 * sizeof(char));
    while (fscanf(file, "%c", &c) != EOF)
    {
        if (c == '\n')
        {
            return 0;
        }
        string[i] = c;
        i++;
    }
    if (strcmp(string, "\0") == 0)
        return -1;
    return 0;
}

void GetData(char line[], struct data *struk)
{
    //ngambil * * * * *
    int i = 0, it = 0, count;
    char temp[10];
    while (i < 5)
    {
        count = 0;
        while (line[it] != ' ')
        {
            temp[count] = line[it];
            count++;
            it++;
        }
        if (temp[0] != '*')
            struk->cronTime[i] = atoi(temp);
        else
            struk->cronTime[i] = -1;
        it++;
        i++;
    }

    //ngambil file path dan program yang ngejalanin
    strcpy(struk->afterTime, &line[it]);

    strcpy(struk->filePath, &line[it]);
    if (strstr(struk->filePath, " ") != NULL)
    {
        strcpy(struk->program, strstr(struk->filePath, " ") + 1);
        struk->filePath[strlen(struk->filePath) - strlen(struk->program) - 1] = '\0';
    }
    else
    {
        strcpy(struk->program, "\0");
    }

    struk->done = 0;
}

void *CronJob(void *ar)
{
    struct data *argv = (struct data *)ar;
    struct tm *currTime;
    struct stat fileStat;
    time_t raw, check;
    while (argv->done == 0)
    {
        time(&raw);
        currTime = localtime(&raw);
        if (argv->cronTime[0] == currTime->tm_min || argv->cronTime[0] < 0)
        {
            if (argv->cronTime[1] == currTime->tm_hour || argv->cronTime[1] < 0)
            {
                if (argv->cronTime[2] == currTime->tm_mday || argv->cronTime[2] < 0)
                {
                    if (argv->cronTime[3] == currTime->tm_mon + 1 || argv->cronTime[3] < 0)
                    {
                        if (argv->cronTime[4] == currTime->tm_wday || argv->cronTime[4] < 0)
                        {
                            printf("%s\n", argv->line);
                            int child = fork();
                            int status;
                            if(child == 0){
                                if(argv->program[0] != '\0'){
                                    char *arg[] = {argv->program, argv->filePath, NULL};
                                    execv(argv->program, arg);
                                }
                                else{
                                    char *arg[] = {argv->filePath, NULL};
                                    execv(argv->filePath, arg);
                                }
                            }
                            else{
                                while(wait(&status) > 0);
                            }
                            time(&check);
                            while (difftime(check, raw) <= 60.0)
                            {
                                sleep(1);
                                time(&check);
                            }
                        }
                    }
                }
            }
        }
    }
}