#include "kernel/types.h"
#include "user/user.h"

//写法一
int main0(int argc, char *argv[]) {
    int p[2];//parent to child.
    int q[2];//child to parent.
    // int pipe(int p[]) Create a pipe, put read/write file descriptors in p[0] and p[1].
    pipe(p);
    pipe(q);
    int child_pid=fork();
    if(child_pid<0){printf("fork error!"); exit(-1);}
    if(child_pid>0){//parent
        close(p[0]);//close parent read
        close(q[1]);//close child write
        // so, now p[1] -> q[0]
        write(p[1], "ping", 16 );

        char buf[1024] = {"\0"};//string end with '\0'
        read(q[0], buf, 16);
        printf("%d: received %s\n",getpid(),buf);

        close(p[1]);//release
        close(q[0]);//release
        exit(0);
    }else{//child
        close(p[1]);//close parent write
        close(q[0]);//close child read
        // so, now q[1] -> p[0]

        char buf[1024] = {"\0"};
        read(p[0], buf, 16);
        printf("%d: received %s\n",getpid(),buf);

        write(q[1], "pong", 16 );

        close(p[0]);//release
        close(q[1]);//release
        exit(0);
    }
    exit(0);
}

// 写法二
int main(int argc, char *argv[]) {
    int pp[2];
    pipe(pp);
    int child_pid=fork();
    if(child_pid<0){printf("fork error!"); exit(-1);}
    if(child_pid>0){//parent
        write(pp[1], "ping", 16 );

        sleep(5);
        char buf[1024] = {"\0"};//string end with '\0'
        read(pp[0], buf, 16);
        printf("%d: received %s\n",getpid(),buf);

        exit(0);
    }else{
        char buf[1024] = {"\0"};//string end with '\0'
        read(pp[0], buf, 16);
        printf("%d: received %s\n",getpid(),buf);

        write(pp[1], "pong",16);

        exit(0);
    }
    exit(0);
}