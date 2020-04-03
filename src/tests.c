#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "parser.h"

FILE* stropen (char* str) {
    return fmemopen(str, strlen(str), "r");
}

void t_lexer (void) {
    {
        LX.buf = stropen("-- foobar");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_EOF);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen("-- c1\n--c2\n\n");
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_COMMENT);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_EOF);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen(" c1\nc2 c3'  \n  \nc4");
        Tk tk1 = lexer(); assert(tk1.sym == TK_VAR); assert(!strcmp(tk1.val.s, "c1"));
        assert(lexer().sym == TK_LINE);
        Tk tk3 = lexer(); assert(tk3.sym == TK_VAR); assert(!strcmp(tk3.val.s, "c2"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_VAR); assert(!strcmp(tk4.val.s, "c3'"));
        assert(lexer().sym == TK_LINE);
        assert(lexer().sym == TK_LINE);
        Tk tk5 = lexer(); assert(tk5.sym == TK_VAR); assert(!strcmp(tk5.val.s, "c4"));
        assert(lexer().sym == TK_EOF);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen(" c1 C1 C'a a'? C!!");
        Tk tk1 = lexer(); assert(tk1.sym == TK_VAR);  assert(!strcmp(tk1.val.s, "c1"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_DATA); assert(!strcmp(tk2.val.s, "C1"));
        Tk tk3 = lexer(); assert(tk3.sym == TK_DATA); assert(!strcmp(tk3.val.s, "C'a"));
        Tk tk4 = lexer(); assert(tk4.sym == TK_VAR);  assert(!strcmp(tk4.val.s, "a'?"));
        Tk tk5 = lexer(); assert(tk5.sym == TK_DATA); assert(!strcmp(tk5.val.s, "C!!"));
        assert(lexer().sym == TK_EOF);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen("let xlet letx");
        assert(lexer().sym == TK_LET);
        Tk tk1 = lexer(); assert(tk1.sym == TK_VAR); assert(!strcmp(tk1.val.s, "xlet"));
        Tk tk2 = lexer(); assert(tk2.sym == TK_VAR); assert(!strcmp(tk2.val.s, "letx"));
        assert(lexer().sym == TK_EOF);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen(": :: :");
        assert(lexer().sym == TK_NONE);
        assert(lexer().sym == TK_DECL);
        assert(lexer().sym == TK_NONE);
        fclose(LX.buf);
    }
}

void t_parser_type (void) {
    {
        LX.buf = stropen("()");
        assert(parser_type().sub == TYPE_UNIT);
        fclose(LX.buf);
    }
}

void t_parser_expr (void) {
    {
        LX.buf = stropen("(())");
        assert(parser_expr().sub == EXPR_UNIT);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen("( ( ) )");
        assert(parser_expr().sub == EXPR_UNIT);
        fclose(LX.buf);
    }
    {
        LX.buf = stropen("(\n( \n");
        Expr e = parser_expr();
        assert(e.sub == EXPR_NONE); assert(!strcmp(e.err.msg, "(ln 2, col 2): expected `)`"));
        fclose(LX.buf);
    }
}

void t_parser_decl (void) {
    {
        LX.buf = stropen("a :: ()");
        Decl decl = parser_decl();
        assert(decl.var.sym  == TK_VAR);
        assert(!strcmp(decl.var.val.s, "a"));
        assert(decl.type.sub == TYPE_UNIT);
        fclose(LX.buf);
    }
}

void t_parser (void) {
    {
        LX.buf = stropen("xxx (  ) XXX");
        Expr e1 = parser_expr();
        assert(e1.sub == EXPR_VAR);  assert(!strcmp(e1.tk.val.s, "xxx"));
        assert(parser_expr().sub == EXPR_UNIT);
        Expr e2 = parser_expr();
        assert(e2.sub == EXPR_CONS); assert(!strcmp(e2.tk.val.s, "XXX"));
        fclose(LX.buf);
    }
    t_parser_type();
    t_parser_expr();
    t_parser_decl();
}

int main (void) {
    t_lexer();
    t_parser();
}
