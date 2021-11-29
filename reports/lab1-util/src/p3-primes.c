#include "kernel/types.h"
#include "user/user.h"

//int read(int fd, char *buf, int n) Read n bytes into buf; returns number read; or 0 if end of file.

void prime(int pipe_fd)
{
    // 从pipe读到的首个数。
    int base;
    // 如果从pipe中读不到数，说明递归已经结束了
    if (read(pipe_fd, &base, sizeof(int)) == 0)
    {
        exit(0);
    }
    printf("prime %d\n", base);

    // 否则进行递归
    int p[2];
    pipe(p);
    if (fork() == 0)//child
    {
        close(p[1]);
        prime(p[0]);
    }
    else//parent
    {
        close(p[0]);
        int n;
        int eof;
        do{
            eof = read(pipe_fd, &n, sizeof(int));//eof==0 means "end of file"
            if (n % base != 0)
            {
                write(p[1], &n, sizeof(int));
            }
        } while (eof!=0);

        close(p[1]);
    }
    wait(0);
    exit(0);
}


int main(int argc, char const *argv[])
{
    int origin_fd[2];
    pipe(origin_fd);
    if (fork() > 0)//parent; main; 
    {
        close(origin_fd[0]);
        int i;
        for (i = 2; i < 36; i++)// 针对主进程，枚举2~35传给子进程筛选质数。
        {
            write(origin_fd[1], &i, sizeof(int));
        }
        close(origin_fd[1]);
    }
    else
    {
        close(origin_fd[1]);
        prime(origin_fd[0]);
    }
    wait(0);
    exit(0);
}