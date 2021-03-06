#include "all.h"

FILE* stropen (const char* mode, size_t size, char* str) {
    size = (size != 0) ? size : strlen(str);
    return fmemopen(str, size, mode);
}

void all_init (FILE* out, FILE* inp) {
    static char buf1[65000];
    static char buf2[65000];
    FILE* out1 = stropen("w", sizeof(buf1), buf1);
    FILE* out2 = stropen("w", sizeof(buf2), buf2);
    ALL = (State_All) { inp,{out,out1,out2},{},0 };
    if (inp != NULL) {
        parser_init();
    }
}
