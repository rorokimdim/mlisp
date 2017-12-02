#include "mpc.h"

#ifndef MLISP_EVAL_H
#define MLISP_EVAL_H

typedef enum { LVAL_INTEGER, LVAL_REAL, LVAL_ERR,
               LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR,
               LVAL_BOOLEAN, LVAL_STR } lval_t;
typedef enum { LERR_DIV_ZERO, LERR_BAD_OP,
               LERR_BAD_INTEGER, LERR_BAD_REAL, LERR_BAD_TYPE,
               LERR_UNKNOWN } lerr_t;

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;
typedef lval* (*lbuiltin) (lenv* e, lval* a, int level);
struct lval {
    lval_t type;
    long integer;
    double real;
    char* err;
    char* sym;
    char* str;

    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;

    int count;
    struct lval** cell;
};

struct lenv {
    lenv* parent;

    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_copy(lenv* e);
lval* eval(lenv* e, mpc_ast_t* ast);
lval* lval_err(char* fmt, ...);
lval* lval_eval(lenv* e, lval* v, int level);
lval* lval_eval_sexpr(lenv* e, lval* v, int level);
lval* lval_read(mpc_ast_t* ast);
lval* lval_take(lval* v, int i);
void lenv_put(lenv* e, lval* k, lval* v);
void lval_del(lval* v);
void lval_expr_print(lenv* e, lval* v, char open, char close);
void lval_print(lenv* e, lval* v, bool for_builtin_print);
void lval_println(lenv* e, lval* v, bool for_builtin_print);

lenv* lenv_new(void);
void lenv_del(lenv* e);
void lenv_add_builtins(lenv* e);

#define LASSERT(args, cond, fmt, ...) \
    if (!(cond)) { \
        lval* err = lval_err(fmt, ##__VA_ARGS__); \
        lval_del(args); \
        return err; \
    }

#define LASSERT_TYPE(fname, args, actual_type, expected_type) \
    if (actual_type != expected_type) { \
        lval* err = lval_err("Function '%s' passed incorrect type. Got %s, Expected %s.", \
                             fname, ltype_name(actual_type), ltype_name(expected_type)); \
        lval_del(args); \
        return err; \
    }

#define LASSERT_NOT_EMPTY_LIST(fname, args, list) \
    LASSERT(args, list->count != 0, "Function '%s' passed {}.", fname);

#define LASSERT_NUM_ARGUMENTS(fname, args, expected) \
    LASSERT(args, args->count == expected, \
            "Function '%s' passed invalid number of arguments. Got %i, expected %i", \
            fname, args->count, expected);

#endif
