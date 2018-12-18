//
//  Venom.h
//  Venom
//
//  Created by pingwei liu on 2018/12/17.
//  Copyright © 2018 pingwei liu. All rights reserved.
//

#ifndef Venom_h
#define Venom_h

#ifdef __cplusplus
#define VN_EXTERN_C_BEGIN extern "C" {
#define VN_EXTERN_C_END }
#else
#define VN_EXTERN_C_BEGIN
#define VN_EXTERN_C_END
#endif


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct Venom Venom;

VN_EXTERN_C_BEGIN

Venom *VenomInit(void);
void VenomPut(Venom *venom, const void *key, uint32_t keyLength, const void *value, uint32_t valueLength, uint8_t type);
const void *VenomGet(Venom *venom, const void *key, uint32_t keyLength, uint32_t *valueLength, uint8_t *type);
void VenomRelease(Venom *venom);
void VenomDebugPrint(Venom *venom);
VN_EXTERN_C_END

#endif /* Venom_h */
