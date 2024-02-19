#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
void handle(int p[]){
    close(p[1]);
    int num;
    int p_next[2];
    pipe(p_next);
    if(fork()== 0){
        handle(p_next);
    }else{
        int tmp;
        close(p_next[0]);
        read(p[0] , &tmp , 4);
        if(tmp <= 0)exit(1);
        printf("primes is %d \n" , tmp);
        while(read(p[0] , &num , 4) > 0){
            if(num % tmp != 0){
                write(p_next[1] , &num , 4);
            }
        }
        close(p_next[1]);
        wait(0);
    }
    exit(0);
}
int main(){
    int p[2];
    int pid;
    //build  a  pipe
    pipe(p);
    pid = fork();
    if(pid == 0){
        handle(p);
    }else{
        close(p[0]);
        for(int i = 2  ; i < 40 ; i++){
            write(p[1] , &i , 4);
        }
        close(p[1]);
        wait(0);
    }
    exit(0);
}