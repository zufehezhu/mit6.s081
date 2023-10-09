#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc , char *argv[]){
    int pid;
    int p1[2];
    int p2[2];
    pipe(p1);
    pipe(p2);
    char buf[1024]={"\0"};
    pid = fork();
    if(pid != 0){
        close(p1[0]);
        write(p1[1],"ping\n" , 5);
        close(p1[1]);
    }if(pid == 0){
        close(p2[0]);
        write(p2[1],"pong\n" , 5);
        close(p2[1]);
    }
    sleep(1);
    if(pid == 0){
        close(p1[1]);
        int c = getpid();
        read(p1[0] , buf , 5);
        // for(int i = 0 ; i < 5 ; i++){
        //     printf("%c",buf[i]);
        // }printf("\n");
        printf(" process %d have received ping\n" , c);
        close(p1[0]);
    }else{
        wait(0);
        close(p2[1]);
        read(p2[0] , buf , 5);
        int c = getpid();
        // for(int i = 0 ; i < 5 ; i++){
        //     printf("%c",buf[i]);
        // }printf("\n");
        printf(" process %d have received pong \n" , c);
        close(p2[0]);
    }
    exit(0);
}