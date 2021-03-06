/*
 *
 * Prog  ::= { Data | Expr }
 *
 * Data  ::= data <Id> [`=` Type] [`:` { Cons }]
 * Cons  ::= <Id> `=` Type
 *
 * Type  ::= `(` `)` | <Id>
 *        |  Type `->` Type
 *        | `(` Type { `,` Type } `)`
 *        | `(` Type `)`
 *
 * Patt  ::=  `(` `)` | `_` | <id>
 *        |   `~` Expr
 *        |   <Id> [ `(` Patt `)` ]
 *        |   `(` Patt { `,` Patt } `)`
 *
 * Expr  ::= Expr' [ where `:` { Expr } ]
 * Expr' ::= `(` `)` | `$` | `...` | <id>       // EXPR_UNIT | EXPR_NIL | EXPR_ARG | EXPR_VAR
 *        |  <Id> [`(` Expr `)`]                // EXPR_CONS
 *        | `{` <...> `}`                       // EXPR_RAW
 *        | `(` Expr { `,` Expr } `)`           // EXPR_TUPLE
 *        |  func `::` Type Expr                // EXPR_FUNC
 *        |  Expr `(` Expr `)`                  // EXPR_CALL
 *        | `(` Expr `)`
 *        |  set <id> `=` Expr                  // EXPR_SET
 *        |  `:` { Expr }                       // EXPR_SEQ
 *        |  case Expr `:`                      // EXPR_CASE
 *               { Patt [`::` Type] [`->`] Expr }
 *               [ `else` [`->`] Expr ]
 *        |  `if` Expr [`->`] Expr [`->`] Expr  // EXPR_IF
 *        |  `if` `:`                           // EXPR_IFS
 *               { Expr [`->`] Expr }
 *               [ `else` [`->`] Expr ]
 *        |  `loop` Expr                        // EXPR_LOOP
 *        |  `break`                            // EXPR_BREAK
 *        |  Expr `~` Expr                      // EXPR_MATCH
 *        |  let Patt `::` Type [`->`] Expr     // EXPR_LET
 *        |  (val | mut) Patt `::` Type         // EXPR_DECL
 *               [`=` Expr]
 *
 */

///////////////////////////////////////////////////////////////////////////////

typedef enum {
    PATT_RAW,
    PATT_UNIT,
    PATT_NIL,
    PATT_ARG,
    PATT_ANY,
    PATT_CONS,
    PATT_SET,
    PATT_EXPR,
    PATT_TUPLE
} PATT;

typedef enum {
    EXPR_RAW,
    EXPR_UNIT,
    EXPR_NIL,
    EXPR_ARG,
    EXPR_VAR,
    EXPR_CONS,
    EXPR_SET,
    EXPR_FUNC,
    EXPR_RETURN,
    EXPR_TUPLE,
    EXPR_SEQ,
    EXPR_CALL,
    EXPR_BLOCK,
    EXPR_LET,
    EXPR_DECL,
    EXPR_IF,
    EXPR_IFS,
    EXPR_MATCH,
    EXPR_CASES,
    EXPR_LOOP,
    EXPR_BREAK,
    EXPR_PASS,
    ////
    EXPR_TUPLE_IDX,
    EXPR_CONS_SUB
} EXPR;

typedef enum {
    GLOB_DATA,
    GLOB_DECL,
    GLOB_EXPR
} GLOB;

///////////////////////////////////////////////////////////////////////////////

struct Expr;

typedef struct Patt {
    PATT sub;
    union {
        Tk Raw;             // PATT_RAW
        struct Expr* Expr;  // PATT_EXPR
        Tk Set;             // PATT_SET
        struct {            // PATT_TUPLE
            int size;
            struct Patt* vec;
        } Tuple;
        struct {            // PATT_CONS
            Tk data;
            struct Patt* arg;
        } Cons;
    };
} Patt;

typedef struct Patt_Type {
    Patt patt;
    Type type;
} Patt_Type;

typedef struct Decl {
    Patt patt;
    Type type;
    struct Expr* init;
} Decl;

typedef struct {
    Decl decl;
    struct Expr* body;
} Let;

typedef struct {
    struct Expr* tst;
    struct Expr* ret;
} If;

typedef struct Expr {
    EXPR  sub;
    State_Tok tok;
    Env*  env;
    struct Expr* where;    // block that executes/declares before expression
    union {
        Tk Raw;
        Tk Unit;
        Tk Var;
        Decl Decl;              // EXPR_DECL
        Let  Let;               // EXPR_LET
        struct Expr* Loop;      // EXPR_LOOP
        struct Expr* Break;     // EXPR_BREAK
        struct Expr* Return;    // EXPR_RETURN
        struct {        // EXPR_TUPLE
            int size;
            struct Expr* vec;
        } Tuple;
        struct {        // EXPR_SEQ
            int size;
            struct Expr* vec;
        } Seq;
        struct {        // EXPR_SET
            Patt patt;
            struct Expr* expr;
        } Set;
        struct {        // EXPR_CALL
            struct Expr* func;
            struct Expr* arg;
            Tk* pool;   // l[]=f() -> f(l)
        } Call;
        struct {        // EXPR_CONS
            Tk id;
            struct Expr* arg;
        } Cons;
        struct {        // EXPR_IF
            struct Expr* tst;
            struct Expr* true;
            struct Expr* false;
        } Cond;
        struct {        // EXPR_MATCH
            struct Expr* expr;
            struct Patt* patt;
        } Match;
        struct {        // EXPR_CASES
            struct Expr* tst;
            int  size;
            Let* vec;
        } Cases;
        struct {        // EXPR_CASES
            int size;
            If* vec;
        } Ifs;
        struct {        // EXPR_FUNC
            Type type;
            struct Expr* body;
        } Func;
        ////
        struct {        // EXPR_TUPLE_IDX
            struct Expr* tuple;
            int idx;
        } Tuple_Idx;
        struct {        // EXPR_CONS_SUB
            struct Expr* cons;
            const char* sub;
        } Cons_Sub;
    };
} Expr;

typedef struct Glob {
    GLOB sub;
    union {
        Data data;
        Expr expr;
    };
} Glob;

///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int size;
    void* vec;
} List;
typedef void* (*List_F) (Env** env);
int parser_list_line (Env** env, int global, List* ret, List_F f, size_t unit);

void parser_init (void);
int parser_type (Type* ret);
int parser_data (Data* ret);
int parser_patt (Env* env, Patt* ret, int is_match);
int parser_expr (Env** env, Expr* ret);
int parser_prog (void);
