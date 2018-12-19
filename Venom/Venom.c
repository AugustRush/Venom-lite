//
//  Venom.c
//  Venom
//
//  Created by pingwei liu on 2018/12/17.
//  Copyright © 2018 pingwei liu. All rights reserved.
//

#include "Venom.h"
#include "xxhash/xxhash.h"
#include <string.h>

#define VN_PAGE_SIZE 4096
#define VN_SEGMENT_SIZE 5

typedef int8_t byte;

typedef struct VNNode {
    uint32_t hash;
    uint32_t offset;
    uint32_t keyLength;
    uint32_t valueLength;
} VNNode;

struct Venom {
    int fd;
    uint32_t hash;
    uint32_t size;
    uint32_t count;
    uint32_t removedCount;
    uint32_t usedCount;
    uint32_t nodeStart;
    uint32_t dataStart;
    uint32_t dataUsed;
    VNNode *nodes;
    byte *data;
};

const size_t VNNODE_SIZE = sizeof(VNNode);

Venom *VenomInit(void) {
    uint32_t size = 200 * VN_PAGE_SIZE;
    Venom *venom = calloc(1, size);
    venom->fd = -1;
    venom->hash = XXH32("Valid_Venom_File", 16, 0);
    venom->size = size;
    venom->count = 0;
    venom->removedCount = 0;
    venom->nodeStart = sizeof(Venom);
    venom->dataStart = VN_PAGE_SIZE * 100;
    venom->dataUsed = 0;
    venom->nodes = (VNNode *)((byte *)venom + venom->nodeStart);
    venom->data = (byte *)venom + venom->dataStart;
    return venom;
}

void VenomResetNode(VNNode *node, uint32_t hash, uint32_t keyLength, uint32_t valueLength, uint32_t offset) {
    node->hash = hash;
    node->offset = offset;
    node->keyLength = keyLength;
    node->valueLength = valueLength;
}

void VenomUpdateNodeUsedCount(Venom *venom, int number) {
    uint32_t count = venom->usedCount + number;
    uint32_t afterUsed = count * VNNODE_SIZE;
    if (venom->nodeStart + afterUsed > venom->dataStart) {
        printf("need to extend memory\n");
    }
    venom->usedCount = count;
}

void VenomUpdateDataBytesCount(Venom *venom, uint32_t count) {
    uint32_t afterUsed = venom->dataUsed + count;
    if (venom->dataStart + afterUsed > venom->size) {
        printf("need to extend memory\n");
    }
    venom->dataUsed = afterUsed;
}

uint32_t VenomWriteNodeData(Venom *venom, uint32_t offset, const void *key, uint32_t keyLength, const void *value, uint32_t valueLength, uint8_t type) {
    venom->data[offset] = type;
    memcpy(venom->data + offset + 1, key, keyLength);
    memcpy(venom->data + offset + 1 + keyLength, value, valueLength);
    return keyLength + valueLength + 1;
}

const void * VenomReadNodeData(Venom *venom, VNNode *node, const void *key, uint32_t keyLength, uint32_t *valueLength, uint8_t *type) {
    *valueLength = node->valueLength;
    *type = venom->data[node->offset];
    if (keyLength == node->keyLength && memcmp(key, venom->data + node->offset + 1, keyLength) == 0) {
        return venom->data + node->offset + 1 + keyLength;
    }
    return NULL;
}

VNNode *VenomSearchNode(Venom *venom, uint32_t hash, const void *key, uint32_t keyLength, uint32_t *valueLength, uint8_t *type) {
    VNNode *result = NULL;
    if (venom->count > 0) {
        VNNode *leftNode = venom->nodes;
        VNNode *rightNode = venom->nodes + venom->usedCount - VN_SEGMENT_SIZE;
        if (leftNode->hash < hash && rightNode->hash > hash) {
            uint32_t left = 0;
            uint32_t right = venom->usedCount / VN_SEGMENT_SIZE - 1;
            uint32_t flag = right / 2;
            VNNode *midNode = venom->nodes + flag * VN_SEGMENT_SIZE;
            while (right > left) {
                if (midNode->hash > hash) {
                    if (right == flag) {
                        //                        flag = right - 1;
                        break;
                    } else {
                        right = flag;
                    }
                } else if (midNode->hash < hash) {
                    if (left == flag) {
                        //                        flag = left + 1;
                        break;
                    } else {
                        left = flag;
                    }
                } else {
                    result = midNode;
                    break;
                }
                
                flag = (left + right) / 2;
                midNode = venom->nodes + flag * VN_SEGMENT_SIZE;
            }
            
            if (result == NULL) {
                uint32_t startIndex = flag * 5 + 1;
                uint32_t lastIndex = flag * 5 + VN_SEGMENT_SIZE - 1;
                for (uint32_t i = startIndex; i <= lastIndex; i++) {
                    VNNode *node = venom->nodes + i;
                    if (node->hash == hash) {
                        result = node;
                    }
                }
            }
            
        } else {
            if (leftNode->hash == hash) {
                result = leftNode;
            } else if (rightNode->hash == hash) {
                result = rightNode;
            }
        }
    }
    
    return result;
}

void VenomResetSegment(Venom *venom, uint32_t start) {
    for (uint32_t i = start; i < start + VN_SEGMENT_SIZE; i++) {
        VNNode *node = venom->nodes + i;
        if (node->keyLength > 0) {
            node->keyLength = 0;
            node->hash = 0;
        } else {
            break;
        }
    }
}

//if exsit return NODE, other return NULL
VNNode * VenomNodeInsertOrReplace(Venom *venom, uint32_t hash, const void *key, uint32_t keyLength, uint32_t valueLength, uint32_t referenceOffset) {
    VNNode *result = NULL;
    if (venom->count == 0) {
        VenomResetNode(venom->nodes, hash, keyLength, valueLength, referenceOffset);
        VenomUpdateNodeUsedCount(venom, VN_SEGMENT_SIZE);
        venom->count += 1;
    } else {
        VNNode *leftNode = venom->nodes;
        VNNode *rightNode = venom->nodes + venom->usedCount - VN_SEGMENT_SIZE;
        if (leftNode->hash > hash) {
            uint32_t usedCount = venom->usedCount;
            VenomUpdateNodeUsedCount(venom, VN_SEGMENT_SIZE);
            memmove(leftNode + VN_SEGMENT_SIZE, leftNode, usedCount * VNNODE_SIZE);
            VenomResetSegment(venom, 0);
            VenomResetNode(leftNode, hash, keyLength, valueLength, referenceOffset);
            venom->count += 1;
        } else if (rightNode->hash < hash) {
            VenomUpdateNodeUsedCount(venom, VN_SEGMENT_SIZE);
            VenomResetNode(rightNode + VN_SEGMENT_SIZE, hash, keyLength, valueLength, referenceOffset);
            venom->count += 1;
        } else {
            uint32_t left = 0;
            uint32_t right = venom->usedCount / VN_SEGMENT_SIZE - 1;
            uint32_t flag = right / 2;
            int isExsit = 0;
            VNNode *midNode = venom->nodes + flag * VN_SEGMENT_SIZE;
            while (right > left) {
                if (midNode->hash > hash) {
                    if (right == flag) {
                        break;
                    } else {
                        right = flag;
                    }
                } else if (midNode->hash < hash) {
                    if (left == flag) {
                        break;
                    } else {
                        left = flag;
                    }
                } else {
                    isExsit = 1;
                    break;
                }
                
                flag = (left + right) / 2;
                midNode = venom->nodes + flag * VN_SEGMENT_SIZE;
            }
            
            if (isExsit) {
                printf("is equal , need to update\n");
                result = midNode;
            } else {
                uint32_t startIndex = flag * 5 + 1;
                uint32_t lastIndex = flag * 5 + VN_SEGMENT_SIZE - 1;
                uint32_t realIndex = startIndex;
                uint32_t endIndex = lastIndex + 1;
                for (uint32_t i = startIndex; i <= lastIndex; i++) {
                    VNNode *node = venom->nodes + i;
                    if (node->keyLength == 0) {
                        endIndex = i;
                        break;
                    }
                    if (node->hash < hash) {
                        realIndex += 1;
                    }
                }
                
                if (realIndex > lastIndex) {
                    uint32_t usedCount = venom->usedCount;
                    VenomUpdateNodeUsedCount(venom, VN_SEGMENT_SIZE);
                    memmove(venom->nodes + realIndex + VN_SEGMENT_SIZE, venom->nodes + realIndex, (usedCount - realIndex) * VNNODE_SIZE);
                    VenomResetSegment(venom, realIndex);
                    VenomResetNode(venom->nodes + realIndex, hash, keyLength, valueLength, referenceOffset);
                    venom->count += 1;
                } else {
                    if (endIndex > lastIndex) { //full
                        uint32_t usedCount = venom->usedCount;
                        VenomUpdateNodeUsedCount(venom, VN_SEGMENT_SIZE);
                        memmove(venom->nodes + endIndex + VN_SEGMENT_SIZE, venom->nodes + endIndex, (usedCount - endIndex) * VNNODE_SIZE);
                        VenomResetSegment(venom, endIndex);
                        memcpy(venom->nodes + endIndex, venom->nodes + lastIndex, VNNODE_SIZE);
                        uint32_t moveCount = lastIndex - realIndex;
                        if (moveCount > 0) {
                            memmove(venom->nodes + realIndex + 1, venom->nodes + realIndex, moveCount * VNNODE_SIZE);
                        }
                        VenomResetNode(venom->nodes + realIndex, hash, keyLength, valueLength, referenceOffset);
                        venom->count += 1;
                    } else {
                        uint32_t moveCount = endIndex - realIndex;
                        if (moveCount > 0) {
                            memmove(venom->nodes + realIndex + 1, venom->nodes + realIndex, moveCount * VNNODE_SIZE);
                        }
                        VenomResetNode(venom->nodes + realIndex, hash, keyLength, valueLength, referenceOffset);
                        venom->count += 1;
                    }
                }
            }
        }
    }
    
    return result;
}

void VenomPut(Venom *venom, const void *key, uint32_t keyLength, const void *value, uint32_t valueLength, uint8_t type) {
    if (venom != NULL) {
        uint32_t hash = XXH32(key, keyLength, 0);
        VNNode *exsit = VenomNodeInsertOrReplace(venom, hash, key, keyLength, valueLength, venom->dataUsed);
        if (exsit != NULL) {
            printf("need handle exsit node\n");
        } else {
            uint32_t bytesCount = VenomWriteNodeData(venom, venom->dataUsed, key, keyLength, value, valueLength, type);
            VenomUpdateDataBytesCount(venom, bytesCount);
        }
    }
}

const void *VenomGet(Venom *venom, const void *key, uint32_t keyLength, uint32_t *valueLength, uint8_t *type) {
    if (venom != NULL) {
        uint32_t hash = XXH32(key, keyLength, 0);
        VNNode *node = VenomSearchNode(venom, hash, key, keyLength, valueLength, type);
        if (node != NULL) {
            return VenomReadNodeData(venom, node, key, keyLength, valueLength, type);
        }
    }
    return NULL;
}

void VenomDebugPrint(Venom *venom) {
    for (uint32_t i = 0; i < venom->usedCount; i++) {
        VNNode node = venom->nodes[i];
        printf("%u: {hash: %u}\n",i,node.hash);
    }
    printf("-------------------------\n");
}


void VenomRelease(Venom *venom) {
    free(venom);
}
