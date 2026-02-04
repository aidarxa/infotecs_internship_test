#include "alloc.h"
#include <stdio.h>
int main(){
    alloc_init();
    uint8_t* ptr = custom_malloc(13);
    if (ptr == NULL) {
        printf("Allocation failed\n");
        return -1;
    }
    else {
        printf("Allocation succeeded: %p\n", ptr);
    }
    custom_free(ptr);
    return 0;
}