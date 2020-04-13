#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

///////////////////////////////////////////////////////////////////////////////

void dump_expr_ (Expr e, int spc) {
    for (int i=0; i<spc; i++) printf(" ");
    switch (e.sub) {
        case EXPR_RAW:
            printf("{ %s }", e.Raw.val.s);
            break;
        case EXPR_UNIT:
            fputs("()", stdout);
            break;
        case EXPR_VAR:
            fputs(e.Var.val.s, stdout);
            break;
        case EXPR_CONS:
            fputs(e.Var.val.s, stdout);
            break;
        case EXPR_SET:
            fputs(e.Set.var.val.s, stdout);
            fputs(" = ", stdout);
            dump_expr_(*e.Set.val, 0);
            break;
        case EXPR_FUNC:
            fputs("func (...)", stdout);
            break;
        case EXPR_RETURN:
            fputs("return ", stdout);
            dump_expr_(*e.Return, 0);
            break;
        case EXPR_BREAK:
            fputs("break ", stdout);
            if (e.Break) {
                dump_expr_(*e.Break, 0);
            }
            break;
        case EXPR_LOOP: {
            puts("loop:");
            dump_expr_(*e.Loop, spc+4);
            break;
        }
        case EXPR_MATCH:
            dump_expr_(*e.Match.expr, 0);
            fputs(" ~ ???", stdout);
            break;
        case EXPR_TUPLE:
            fputs("<", stdout);
            for (int i=0; i<e.Tuple.size; i++) {
                if (i>0) fputs(", ", stdout);
                dump_expr_(e.Tuple.vec[i], 0);
            }
            fputs(">", stdout);
            break;
        case EXPR_CALL:
            dump_expr_(*e.Call.func, 0);
            fputs("(", stdout);
            dump_expr_(*e.Call.arg, 0);
            fputs(")", stdout);
            break;
        case EXPR_SEQ:
            printf(": [%d]\n", e.Seq.size);
            for (int i=0; i<e.Seq.size; i++) {
                dump_expr_(e.Seq.vec[i], spc+4);
                puts("");
            }
            break;
        case EXPR_LET:
            fputs("let (...)", stdout);
            break;
        case EXPR_IF:
            fputs("if (...)", stdout);
            break;
        case EXPR_CASES:
            fputs("match (...)", stdout);
            break;
        default:
            printf(">>> %d\n", e.sub);
            assert(0 && "TODO");
    }
}

void dump_expr (Expr e) {
    dump_expr_(e, 0);
    puts("");
}

int is_rec (const char* v) {
    for (int i=0; i<ALL.data_recs.size; i++) {
        if (!strcmp(ALL.data_recs.buf[i].val.s, v)) {
            return 1;
        }
    }
    return 0;
}

Expr* expr_new (void) {
    Expr e;
    if (!parser_expr(&e)) {
        return NULL;
    }
    Expr* pe = malloc(sizeof(Expr));
    assert(pe != NULL);
    *pe = e;
    return pe;
}

///////////////////////////////////////////////////////////////////////////////

void pr_read (long* off, Tk* tk) {
    *off = ftell(ALL.inp);
    *tk  = lexer();

    // skip comment
    if (tk->sym == TK_COMMENT) {
        *off = ftell(ALL.inp);
        *tk  = lexer();
    }
}

void pr_lincol (void) {
    if (TOK1.tk.sym == '\n') {
        TOK2.lin = TOK1.lin + 1;
        TOK2.col = (TOK2.off - TOK1.off);
    } else {
        TOK2.col = TOK1.col + (TOK2.off - TOK1.off);
    }
}

void pr_init () {
    TOK0 = (State_Tok) { -1,0,0,{} };
    TOK1 = (State_Tok) { -1,1,1,{} };
    TOK2 = (State_Tok) { -1,1,1,{} };
    pr_read(&TOK1.off, &TOK1.tk);
    pr_read(&TOK2.off, &TOK2.tk);
    pr_lincol();
}

void pr_next () {
    TOK0 = TOK1;
    TOK1 = TOK2;
    pr_read(&TOK2.off, &TOK2.tk);
    pr_lincol();
}

int pr_accept1 (TK tk, int ok) {
    if (TOK1.tk.sym==tk && ok) {
        pr_next();
        return 1;
    } else {
        return 0;
    }
}

int pr_accept2 (TK tk, int ok) {
    if (TOK2.tk.sym==tk && ok) {
        pr_next();
        pr_next();
        return 1;
    } else {
        return 0;
    }
}

int pr_check0 (TK tk, int ok) {
    return (TOK0.tk.sym==tk && ok);
}

int pr_check1 (TK tk, int ok) {
    return (TOK1.tk.sym==tk && ok);
}

int pr_check2 (TK tk, int ok) {
    return (TOK2.tk.sym==tk && ok);
}

///////////////////////////////////////////////////////////////////////////////

int err_expected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): expected %s : have %s",
        TOK1.lin, TOK1.col, v, lexer_tk2str(&TOK1.tk));
    return 0;
}

int err_unexpected (const char* v) {
    sprintf(ALL.err, "(ln %ld, col %ld): unexpected %s", TOK1.lin, TOK1.col, v);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void init (FILE* out, FILE* inp) {
    ALL = (State_All) { out,inp,{},0,{0,{}} };
    if (inp != NULL) {
        pr_init();
    }
}

///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int size;
    void* vec;
} List;

typedef void* (*List_F) (void);

int parser_list_comma (List* ret, void* fst, List_F f, size_t unit) {
    if (!pr_accept1(',',1)) {
        return err_expected("`,`");
    }

    void* vec = malloc(unit);
    memcpy(vec, fst, unit);
    int i = 1;
    while (1) {
        void* item = f();
        if (item == NULL) {
            return 0;
        }
        vec = realloc(vec, (i+1)*unit);
        memcpy(vec+i*unit, item, unit);
        i++;
        if (!pr_accept1(',',1)) {
            break;
        }
    }

    ret->size = i;
    ret->vec  = vec;
    return 1;
}

int parser_list_line (int doind, List* ret, List_F f, size_t unit) {
    if (doind) {
        if (!pr_accept1(':',1)) {
            return err_expected("`:`");
        }
        if (!pr_check1('\n',1)) {
            return err_expected("new line");
        }
        ALL.ind += 4;
    }

    void* vec = NULL;
    int i = 0;
    while (1) {
        while (pr_accept1('\n',1));    // skip line + empty lines

        if (ALL.ind==0 && !pr_check1(' ',1) && !pr_check1(EOF,1)) {
            // ok
        } else if (i>0 && pr_check0('\n',1) && (!pr_check1(' ',1) || pr_check1(' ',TOK1.tk.val.n<ALL.ind))) {
            break;  // unnest
        } else if (!pr_accept1(' ', TOK1.tk.val.n==ALL.ind)) {
            char s[256];
            sprintf(s, "indentation of %d spaces", ALL.ind);
            return err_expected(s);
        }

        // COMMENT
        if (pr_accept1(TK_COMMENT,1)) {
            continue;
        }

        // ITEM
        void* item = f();;
        if (item == NULL) {
            return 0;
        }
        vec = realloc(vec, (i+1)*unit);
        memcpy(vec+i*unit, item, unit);
        i++;
    }

    if (doind) {
        ALL.ind -= 4;
    }
    ret->size = i;
    ret->vec  = vec;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_type_ (void) {
    static Type tp_;
    Type tp;
    if (!parser_type(&tp)) {
        return NULL;
    }
    tp_ = tp;
    return &tp_;
}

int parser_type (Type* ret) {
    // TYPE_RAW
    if (pr_accept1(TK_RAW,1)) {
        *ret = (Type) { TYPE_RAW, .Raw=TOK0.tk };

    // TYPE_UNIT
    } else if (pr_accept1('(',1)) {
        if (pr_accept1(')',1)) {
            *ret = (Type) { TYPE_UNIT, {} };
        } else {
            if (!parser_type(ret)) {
                return 0;
            }

    // TYPE_FUNC
            if (pr_accept1(TK_ARROW,1)) {
                Type tp;
                if (parser_type(&tp)) {
                    Type* inp = malloc(sizeof(*inp));
                    Type* out = malloc(sizeof(*out));
                    assert(inp != NULL);
                    assert(out != NULL);
                    *inp = *ret;
                    *out = tp;
                    *ret = (Type) { TYPE_FUNC, .Func={inp,out} };
                }
            }

    // TYPE_TUPLE
            if (pr_check1(',',1)) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&lst, ret, parser_type_, sizeof(Type))) {
                    return 0;
                }
                *ret = (Type) { TYPE_TUPLE, .Tuple={lst.size,lst.vec} };
            }
    // TYPE_PARENS
            if (!pr_accept1(')',1)) {
                return err_expected("`)`");
            }
        }
    // TYPE_DATA
    } else if (pr_accept1(TK_IDDATA,1)) {
        *ret = (Type) { TYPE_DATA, .Data=TOK0.tk };
    } else {
        return err_expected("type");
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_patt_ (void) {
    static Patt pt_;
    Patt pt;
    if (!parser_patt(&pt,0)) {
        return NULL;
    }
    pt_ = pt;
    return &pt_;
}

int parser_patt (Patt* ret, int is_match) {
    // PATT_RAW
    if (pr_accept1(TK_RAW,1)) {
        *ret = (Patt) { PATT_RAW, .Raw=TOK0.tk };
    // PATT_UNIT
    } else if (pr_accept1('(',1)) {
        if (pr_accept1(')',1)) {
            *ret = (Patt) { PATT_UNIT, {} };
        } else {
            if (!parser_patt(ret,is_match)) {
                return 0;
            }
    // PATT_TUPLE
            if (pr_check1(',',1)) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&lst, ret, parser_patt_, sizeof(Patt))) {
                    return 0;
                }
                *ret = (Patt) { PATT_TUPLE, .Tuple={lst.size,lst.vec} };
            }
    // PATT_PARENS
            if (!pr_accept1(')',1)) {
                return err_expected("`)`");
            }
        }
    // PATT_ANY
    } else if (pr_accept1('_',1)) {
        *ret = (Patt) { PATT_ANY };
    // PATT_CONS
    } else if (pr_accept1(TK_IDDATA,1)) {
        *ret = (Patt) { PATT_CONS, .Cons={TOK0.tk,NULL} };
        if (pr_check1('(',1)) {
    // PATT_CONS(...)
            Patt arg;
            if (!parser_patt(&arg,is_match)) {
                return 0;
            }
            Patt* parg = malloc(sizeof(arg));
            assert(parg != NULL);
            *parg = arg;
            *ret = (Patt) { PATT_CONS, .Cons={ret->Cons.data,parg} };
        }
    // PATT_SET
    } else if (pr_accept1(TK_IDVAR,1)) {
        if (is_match) {
            Expr e = (Expr) { EXPR_VAR, {.size=0}, .Var=TOK0.tk };
            Expr* pe = malloc(sizeof(Expr));
            assert(pe != NULL);
            *pe = e;
            *ret = (Patt) { PATT_EXPR, .Expr=pe };
        } else {
            *ret = (Patt) { PATT_SET, .Set=TOK0.tk };
        }
    } else if (pr_accept1('~',1)) {
        Expr* pe = expr_new();
        if (pe == NULL) {
            return 0;
        }
        *ret = (Patt) { PATT_EXPR, .Expr=pe };
    } else {
        assert(0 && "TODO");
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_cons_ (void) {
    static Cons c_;
    Cons c;

    if (!pr_accept1(TK_IDDATA,1)) {
        err_expected("data identifier");
        return NULL;
    }
    c.tk = TOK0.tk;

    if (!parser_type(&c.type)) {
        c.type = (Type) { TYPE_UNIT, {} };
    }

    c_ = c;
    return &c_;
}

int parser_data (Data* ret) {
    if (!pr_accept1(TK_DATA,1)) {
        return err_expected("`data`");
    }
    if (!pr_accept1(TK_IDDATA,1)) {
        return err_expected("data identifier");
    }
    Tk id = TOK0.tk;

    Type tp;
    int tp_ok = parser_type(&tp);

    List lst = { 0, NULL };
    int lst_ok = pr_check1(':', 1);
    if (lst_ok) {
        if (!parser_list_line(1, &lst, &parser_cons_, sizeof(Cons))) {
            return 0;
        }
    }

    if (!tp_ok && !lst_ok) {
        // recursive pre declaration
        assert(ALL.data_recs.size < sizeof(ALL.data_recs.buf));
        ALL.data_recs.buf[ALL.data_recs.size++] = id;
        *ret = (Data) { id, 0, NULL };
        return 1;
    }

    *ret = (Data) { id, lst.size, lst.vec };
    for (int i=0; i<ret->size; i++) {
        ret->vec[i].idx = i;
    }

    // mark each Cons as is_rec as well
    if (is_rec(id.val.s)) {
        for (int i=0; i<ret->size; i++) {
            ALL.data_recs.buf[ALL.data_recs.size++] = ret->vec[i].tk;
        }
    }

    // realloc TP more spaces inside each CONS
    if (tp_ok) {
        if (lst_ok) {
            assert(0 && "TODO");
        } else {
            ret->size = 1;
            ret->vec  = malloc(sizeof(Cons));

            Cons c = (Cons) { 0, {}, tp };
            strcpy(c.tk.val.s, id.val.s);
            ret->vec[0] = c;
        }
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

int parser_decl_nopre (Decl* decl) {
    int is_func = pr_check0(TK_FUNC,1);
    if (!parser_patt(&decl->patt,0)) {
        return 0;
    }

    if (!pr_accept1(TK_DECL,1)) {
        return err_expected("`::`");
    }

    if (!parser_type(&decl->type)) {
        return 0;
    }

    if (is_func || (!is_func && pr_accept1('=',1))) {
        Expr init;
        if (!parser_expr(&init)) {
            return is_func;
        }
        decl->init = malloc(sizeof(*decl->init));
        assert(decl->init != NULL);
        *decl->init = init;
    } else {
        decl->init = NULL;
    }

    return 1;
}

int parser_decl (Decl* decl) {
    if (!pr_accept1(TK_MUT,1) && !pr_accept1(TK_VAL,1) && !pr_accept1(TK_FUNC,1)) {
        return err_expected("`mut` or `val` or `func`");
    }
    return parser_decl_nopre(decl);
}

void* parser_decl_ (void) {
    static Decl d_;
    Decl d;
    if (!parser_decl(&d)) {
        return NULL;
    }
    d_ = d;
    return &d_;
}

int parser_decls (Decls* ret) {
    List lst;
    if (!parser_list_line(1, &lst, &parser_decl_, sizeof(Decl))) {
        return 0;
    }
    *ret = (Decls) { lst.size, lst.vec };
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void* parser_expr_ (void) {
    static Expr e_;
    Expr e;
    if (!parser_expr(&e)) {
        return NULL;
    }
    e_ = e;
    return &e_;
}

void* parser_expr__ (void) {
    static Expr e_;
    Expr* pe = parser_expr_();
    if (pe == NULL) {
        return NULL;
    }
    Expr e = *pe;
    e_ = e;
    return &e_;
}

void* parser_case_ (void) {
    static Case c_;
    Case c;

    // patt
    if (pr_accept1(TK_ELSE,1)) {
        c.patt = (Patt) { PATT_ANY, {} };
    } else if (!parser_patt(&c.patt,0)) {
        return NULL;
    }

    // decls
    if (pr_accept1(TK_DECL,1)) {
        if (!parser_type(&c.type)) {
            return 0;
        }
    } else {
        c.type.sub = TYPE_NONE;
    }

    // ->
    pr_accept1(TK_ARROW,1);       // optional

    // expr
    Expr* pe = expr_new();
    if (pe == NULL) {
        return NULL;
    }

    c.expr = pe;
    c_ = c;
    return &c_;
}

void* parser_if_ (void) {
    static If c_;
    If c;

    if (pr_accept1(TK_ELSE,1)) {
        c.tst = malloc(sizeof(Expr));
        assert(c.tst != NULL);
        *c.tst = (Expr) { EXPR_RAW, .Raw={TK_RAW,{.s="1"}} };
    } else {
        c.tst = expr_new();
        if (c.tst == NULL) {
            return NULL;
        }
    }

    // ->
    pr_accept1(TK_ARROW,1);       // optional

    // expr
    c.ret = expr_new();
    if (c.ret == NULL) {
        return NULL;
    }

    c_ = c;
    return &c_;
}

int parser_expr_one (Expr* ret) {
    // EXPR_RAW
    if (pr_accept1(TK_RAW,1)) {
        *ret = (Expr) { EXPR_RAW, {.size=0}, .Raw=TOK0.tk };

    // EXPR_UNIT
    } else if (pr_accept1('(',1)) {
        if (pr_accept1(')',1)) {
            *ret = (Expr) { EXPR_UNIT, {.size=0}, {} };
        } else {
            if (!parser_expr(ret)) {
                return 0;
            }
    // EXPR_TUPLE
            if (pr_check1(',',1)) {
                List lst = { 0, NULL };
                if (!parser_list_comma(&lst, ret, parser_expr_, sizeof(Expr))) {
                    return 0;
                }
                *ret = (Expr) { EXPR_TUPLE, {.size=0}, .Tuple={lst.size,lst.vec} };
            }
    // EXPR_PARENS
            if (!pr_accept1(')',1)) {
                return err_expected("`)`");
            }
        }

    // EXPR_ARG
    } else if (pr_accept1(TK_ARG,1)) {
        *ret = (Expr) { EXPR_ARG, {.size=0}, {} };

    // EXPR_VAR
    } else if (pr_accept1(TK_IDVAR,1)) {
        *ret = (Expr) { EXPR_VAR, {.size=0}, .Var=TOK0.tk };

    // EXPR_CONS
    } else if (pr_accept1(TK_IDDATA,1)) {
        *ret = (Expr) { EXPR_CONS, {.size=0}, .Cons=TOK0.tk };

    // EXPR_NEW
    } else if (pr_accept1(TK_NEW,1)) {
        Expr* pe = expr_new();
        if (pe == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_NEW, {.size=0}, .New=pe };

    // EXPR_SET
    } else if (pr_accept1(TK_SET,1)) {
        if (!pr_accept1(TK_IDVAR,1) && !pr_accept1(TK_RAW,1)) {
            return err_expected("variable");
        }
        Tk var = TOK0.tk;
        if (!pr_accept1('=',1)) {
            return err_expected("`=`");
        }
        Expr* pe = expr_new();
        if (pe == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_SET, {.size=0}, .Set={var,pe} };

    // EXPR_FUNC
    } else if (pr_accept1(TK_FUNC,1)) {
        if (!pr_accept1(TK_DECL,1)) {
            return err_expected("`::`");
        }
        Type tp;
        if (!parser_type(&tp)) {
            return 0;
        }
        Expr* pe = expr_new();
        if (pe == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_FUNC, {.size=0}, .Func={tp,pe} };

    // EXPR_PASS
    } else if (pr_accept1(TK_PASS,1)) {
        *ret = (Expr) { EXPR_PASS, {.size=0} };

    // EXPR_RETURN
    } else if (pr_accept1(TK_RETURN,1)) {
        *ret = (Expr) { EXPR_RETURN, {.size=0}, .Return=expr_new() };

    // EXPR_SEQ
    } else if (pr_check1(':',1)) {
        List lst;
        if (!parser_list_line(1, &lst, &parser_expr__, sizeof(Expr))) {
            return 0;
        }
        *ret = (Expr) { EXPR_SEQ, {.size=0}, .Seq={lst.size,lst.vec} };

    // EXPR_LET
    // let <id> `::` Type `=` Expr [`->`] Expr
    } else if (pr_accept1(TK_LET,1)) {
        Decl d;
        if (!parser_decl_nopre(&d)) {
            return 0;
        }
        if (d.init == NULL) {
            return err_expected("`=`");
        }

        pr_accept1(TK_ARROW,1);  // optional `->`
        Expr* pe = expr_new();
        if (pe == NULL) {
            return 0;
        }

        *ret = (Expr) { EXPR_LET, {.size=0}, .Let={d.patt,d.type,d.init,pe} };

    } else if (pr_accept1(TK_IF,1)) {
    // EXPR_IFS
        if (pr_check1(':',1)) {
            List lst;
            if (!parser_list_line(1, &lst, &parser_if_, sizeof(If))) {
                return 0;
            }
            *ret = (Expr) { EXPR_IFS, {.size=0}, .Ifs={lst.size,lst.vec} };
    // EXPR_IF
        } else {
            Expr* tst = expr_new();
            if (tst == NULL) {
                return 0;
            }

            pr_accept1(TK_ARROW,1);  // optional `->`

            Expr* t = expr_new();
            if (tst == NULL) {
                return 0;
            }

            pr_accept1(TK_ARROW,1);  // optional `->`

            Expr* f = expr_new();
            if (tst == NULL) {
                return 0;
            }

            *ret = (Expr) { EXPR_IF, {.size=0}, .Cond={tst,t,f} };
        }

    // EXPR_CASES
    } else if (pr_accept1(TK_CASE,1)) {
        Expr* pe = expr_new();
        if (pe == NULL) {
            return 0;
        }
        List lst;
        if (!parser_list_line(1, &lst, &parser_case_, sizeof(Case))) {
            return 0;
        }
        *ret = (Expr) { EXPR_CASES, {.size=0}, .Cases={pe,lst.size,lst.vec} };

    // EXPR_BREAK
    } else if (pr_accept1(TK_BREAK,1)) {
        *ret = (Expr) { EXPR_BREAK, {.size=0}, .Break=expr_new() };

    // EXPR_LOOP
    } else if (pr_accept1(TK_LOOP,1)) {
        Expr* pe = expr_new();
        if (pe == NULL) {
            return 0;
        }
        *ret = (Expr) { EXPR_LOOP, {.size=0}, .Loop=pe };

    } else {
        return err_expected("expression");
    }

    assert(ret->decls.size == 0);

    if (
        (pr_check0('\n',1) && pr_check1(' ',ALL.ind) && pr_accept2(TK_WHERE,1))
    ||
        (pr_check0('\n',1) && ALL.ind==0 && pr_accept1(TK_WHERE,1))
    ||
        (!pr_check0('\n',1) && pr_accept1(TK_WHERE,1))
    ) {
        return parser_decls(&ret->decls);
    }

    return 1;
}

int parser_expr (Expr* ret) {
    Expr e;
    if (!parser_expr_one(&e)) {
        return 0;
    }
    *ret = e;

    // EXPR_CALL'S
    while (1) {
        if (!pr_check1('(',1)) {
            break;
        }
        Expr e;
        if (!parser_expr_one(&e)) {
            return 0;
        }
        Expr* func = malloc(sizeof(Expr));
        Expr* arg  = malloc(sizeof(Expr));
        assert(func!=NULL && arg!=NULL);
        *func = *ret;
        *arg  = e;
        *ret  = (Expr) { EXPR_CALL, {.size=0}, .Call={func,arg} };
    }

    // EXPR_MATCH'S
    while (1) {
        if (!pr_accept1('~',1)) {
            break;
        }
        Patt patt;
        if (!parser_patt(&patt,1)) {
            return 0;
        }

        Expr* pe = malloc(sizeof(Expr));
        Patt* pp = malloc(sizeof(Patt));
        assert(pe!=NULL && pp!=NULL);
        *pe = *ret;
        *pp = patt;
        *ret = (Expr) { EXPR_MATCH, {.size=0}, .Match={pe,pp} };
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////

// Glob ::= Data | Decl | Expr
// Prog ::= { Glob }

void* parser_glob_ (void) {
    static Glob g_;
    Glob g;

    if (pr_check1(TK_DATA,1)) {
        if (!parser_data(&g.data)) {
            return NULL;
        }
        g.sub = GLOB_DATA;
        g_ = g;
        return &g_;
    }

    if (pr_check1(TK_MUT,1) || pr_check1(TK_VAL,1) || pr_check1(TK_FUNC,1)) {
        if (!parser_decl(&g.decl)) {
            return NULL;
        }
        g.sub = GLOB_DECL;
        g_ = g;
        return &g_;
    }

    Expr* e = parser_expr__();  // other parser_expr* variations do not parse where
    if (e == NULL) {
        return NULL;
    }
    g.expr = *e;
    g.sub = GLOB_EXPR;
    g_ = g;
    return &g_;
}

int parser_prog (Prog* ret) {
    List lst;
    if (!parser_list_line(0, &lst, &parser_glob_, sizeof(Glob))) {
        return 0;
    }
    *ret = (Prog) { lst.size, lst.vec };

    if (!pr_accept1(EOF,1)) {
        return err_expected("end of file");
    }
    return 1;
}
