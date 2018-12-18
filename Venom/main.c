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

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");
    
    Venom *venom = VenomInit();
    uint32_t loops = 10000;
    for (int i = loops; i > 0; i--) {
        VenomPut(venom, &i, 4, &i, 4, 1);
    }
    
    VenomDebugPrint(venom);
    
    for (int i = loops; i > 0; i--) {
        uint8_t type;
        uint32_t valueL;
        int res = -1;
        const void *value = VenomGet(venom, &i, 4, &valueL, &type);
        memcpy(&res, value, 4);
        printf("value is %d\n",res);
    }
    return 0;
}
