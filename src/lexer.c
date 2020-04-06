#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

State_All ALL = { NULL,NULL,{},0 };
State_Tok NXT = { -1,0,0,{} };
State_Tok PRV = { -1,0,0,{} };

static char* reserved[] = {
    "case", "data", "else", "func", "let", "set", "var", "where"
};

int is_reserved (TK_val* val) {
    int N = sizeof(reserved) / sizeof(reserved[0]);
    for (int i=0; i<N; i++) {
        if (!strcmp(val->s, reserved[i])) {
            return TK_RESERVED+1 + i;
        }
    }
    return 0;
}

const char* lexer_tk2str (Tk* tk) {
    static char str[512];
    switch (tk->sym) {
        case TK_EOF:
            sprintf(str, "end of file");
            break;
        case TK_LINE:
            sprintf(str, "new line");
            break;
        case TK_DECL:
            sprintf(str, "`::`");
            break;
        case TK_ARROW:
            sprintf(str, "`->`");
            break;
        case TK_ARG:
            sprintf(str, "`...`");
            break;
        case TK_IDVAR:
        case TK_IDDATA:
            sprintf(str, "`%s`", tk->val.s);
            break;
        default:
            if (tk->sym < TK_CHAR) {
                sprintf(str, "`%c`", tk->sym);
            } else if (tk->sym > TK_RESERVED) {
                sprintf(str, "`%s`", reserved[tk->sym-TK_RESERVED-1]);
            } else {
//printf("%d\n", tk->sym);
                assert(0 && "TODO");
            }
            break;
    }
    return str;
}

TK lexer_ (TK_val* val) {
    while (1) {
        int c = fgetc(ALL.inp);
//printf("0> [%c] [%d]\n", c, c);
        switch (c)
        {
            case '(':
            case ')':
                return c;

            case EOF:
                return TK_EOF;

            case '\n': {
                int i = 0;
                while (1) {
                    c = fgetc(ALL.inp);
                    if (c != ' ') {
                        ungetc(c, ALL.inp);
                        break;
                    }
                    i++;
                }
                if (i%4 != 0) {
                    return TK_ERR;
                }
                val->n = i/4;
                return TK_LINE;
            }

            case ':':
                c = fgetc(ALL.inp);
                if (c == ':') {
                    return TK_DECL;
                } else {
                    ungetc(c, ALL.inp);
                    return ':';
                }

            case '.': {
                int c2 = fgetc(ALL.inp);
                int c3 = fgetc(ALL.inp);
                if (c2=='.' && c3=='.') {
                    return TK_ARG;
                }
                return TK_ERR;
            }

            case '-':
                c = fgetc(ALL.inp);
                if (c == '-') {
                    while (1) {
                        c = fgetc(ALL.inp);
                        if (c == EOF) {
                            break;      // EOF stops comment
                        }
                        if (c=='\n' || c=='\r') {
                            ungetc(c, ALL.inp);
                            break;      // NEWLINE stops comment
                        }
                    }
                    return TK_COMMENT;
                } else if (c == '>') {
                    return TK_ARROW;
                }

            case '=':
                return c;

            default:

                if (!isalpha(c)) {
                    return TK_ERR;
                }

                int i = 0;
                while (isalnum(c) || c=='_' || c=='\'' || c=='?' || c=='!') {
                    val->s[i++] = c;
                    c = fgetc(ALL.inp);
                }
                val->s[i] = '\0';
                ungetc(c, ALL.inp);

                int key = is_reserved(val);
                if (key) {
                    return key;
                }

                return (islower(val->s[0]) ? TK_IDVAR : TK_IDDATA);
        }
    }
}

Tk lexer () {
    Tk ret;
    TK tk = lexer_(&ret.val);
    while (1) {
        int c = fgetc(ALL.inp);
        if (c != ' ') {
            ungetc(c, ALL.inp);
            break;
        }
    }
    ret.sym = tk;
    return ret;
}
