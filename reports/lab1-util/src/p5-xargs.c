#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p[2];
    pipe(p);// 开启具名管道，对应于命令行的管道操作符
    char buf[1024] = {"\0"};
    char *params[128]; // 暂存命令行参数
    int index = 0;
    int k;
    for (int i = 1; i < argc; i++) {
        params[index++] = argv[i];
    }
    while ((k = read(0, buf, 1024)) > 0) { // fd=0代表管道操作符的读端
        char tmp[1024] = {"\0"};
        params[index] = tmp;
        for (int i = 0; i < strlen(buf); i++) {
            if (buf[i] == '\n') {// 读参数完毕，开始执行
                if (fork() == 0) { // 创建合作子进程
                    exec(argv[1], params);// 执行
                }
                wait(0);  // 父进程等待子进程执行完毕，并释放
            } else {
                tmp[i] = buf[i]; // 未遇到终止符，读入参数
            }
        }
    }
    exit(0);
}

