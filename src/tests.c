#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"

FILE* stropen (char* str) {
    return fmemopen(str, strlen(str), "r");
}

void t_lexer (void) {
    {
        FILE* buf = stropen("-- foobar");
        assert(lexer(buf) == TK_COMMENT);
        assert(lexer(buf) == TK_EOF);
        fclose(buf);
    }
    {
        FILE* buf = stropen("-- c1\n--c2\n\n");
        assert(lexer(buf) == TK_COMMENT);
        assert(lexer(buf) == TK_NEWLINE);
        assert(lexer(buf) == TK_COMMENT);
        assert(lexer(buf) == TK_NEWLINE);
        assert(lexer(buf) == TK_NEWLINE);
        assert(lexer(buf) == TK_EOF);
        fclose(buf);
    }
    {
        FILE* buf = stropen(" c1\nc2 c3'  \n  \nc4");
        assert(lexer(buf) == TK_VAR);         assert(!strcmp(lexer_value, "c1"));
        assert(lexer(buf) == TK_NEWLINE);
        assert(lexer(buf) == TK_VAR);         assert(!strcmp(lexer_value, "c2"));
        assert(lexer(buf) == TK_VAR);         assert(!strcmp(lexer_value, "c3'"));
        assert(lexer(buf) == TK_NEWLINE);
        assert(lexer(buf) == TK_NEWLINE);
        assert(lexer(buf) == TK_VAR);         assert(!strcmp(lexer_value, "c4"));
        assert(lexer(buf) == TK_EOF);
        fclose(buf);
    }
    {
        FILE* buf = stropen(" c1 C1 C'a a'? C!!");
        assert(lexer(buf) == TK_VAR);  assert(!strcmp(lexer_value, "c1"));
        assert(lexer(buf) == TK_DATA); assert(!strcmp(lexer_value, "C1"));
        assert(lexer(buf) == TK_DATA); assert(!strcmp(lexer_value, "C'a"));
        assert(lexer(buf) == TK_VAR);  assert(!strcmp(lexer_value, "a'?"));
        assert(lexer(buf) == TK_DATA); assert(!strcmp(lexer_value, "C!!"));
        assert(lexer(buf) == TK_EOF);
        fclose(buf);
    }
    {
        FILE* buf = stropen("let xlet letx");
        assert(lexer(buf) == TK_LET);
        assert(lexer(buf) == TK_VAR); assert(!strcmp(lexer_value, "xlet"));
        assert(lexer(buf) == TK_VAR); assert(!strcmp(lexer_value, "letx"));
        assert(lexer(buf) == TK_EOF);
        fclose(buf);
    }
}

int main (void) {
    t_lexer();
}
