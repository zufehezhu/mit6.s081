#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc , char *argv[]){
    printf("Hello %s!\n", argv[0]);
    printf("Hello World!\n");
    exit(0);
}