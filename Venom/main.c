//
//  main.c
//  Venom
//
//  Created by pingwei liu on 2018/12/17.
//  Copyright © 2018 pingwei liu. All rights reserved.
//

#include <stdio.h>
#include "Venom.h"
#include <string.h>
#include <sys/time.h>

void printTime()      //直接调用这个函数就行了，返回值最好是int64_t，long long应该也可以
{
    struct timeval tv;
    gettimeofday(&tv,NULL);    //该函数在sys/time.h头文件中
    printf("%ld  %d\n",tv.tv_sec,tv.tv_usec);
}

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");
    
    Venom *venom = VenomInit();
    uint32_t loops = 20000;
    
    printTime();
    
    for (int i = loops; i > 0; i--) {
        VenomPut(venom, &i, 4, &i, 4, 1);
    }
    
    printTime();
    
//    VenomDebugPrint(venom);
    
    for (int i = loops; i > 0; i--) {
        uint8_t type;
        uint32_t valueL;
        int res = -1;
        const void *value = VenomGet(venom, &i, 4, &valueL, &type);
        memcpy(&res, value, 4);
//        printf("value is %d\n",res);
    }
    
    printTime();
    return 0;
}
