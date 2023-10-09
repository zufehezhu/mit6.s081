#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc , char* argv[]){
    if (argc != 2)
    {
        fprintf(0, "usage: sleep <number>\n");
        exit(1);
    }else{
        int n = atoi(argv[1]);
        sleep(n);
        printf("have sleeping %d s" , n);
        exit(0); 
    }
}