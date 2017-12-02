#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mpc.h"
#include "eval.h"

char* ltype_name (int t) {
    switch (t) {
        case LVAL_FUN:
            return "Function";
        case LVAL_INTEGER:
            return "Integer";
        case LVAL_REAL:
            return "Real";
        case LVAL_ERR:
            return "Error";
        case LVAL_SYM:
            return "Symbol";
        case LVAL_STR:
            return "String";
        case LVAL_SEXPR:
            return "S-Expression";
        case LVAL_QEXPR:
            return "Q-Expression";
        case LVAL_BOOLEAN:
            return "Boolean";
        default:
            return "Unknown";
    }
}

lval* lval_integer(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_INTEGER;
    v->integer = x;
    return v;
}

lval* lval_boolean(bool x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_BOOLEAN;
    v->integer = x;
    return v;
}

lval* lval_real(double x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_REAL;
    v->real = x;
    return v;
}

lval* lval_sym(char* sym) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(sym) + 1);
    strcpy(v->sym, sym);
    return v;
}

lval* lval_str(char* str) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(str) + 1);
    strcpy(v->str, str);
    return v;
}

lval* lval_fun(lbuiltin fun) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = fun;
    return v;
}

lval* lval_lambda(lval* formals, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;

    v->builtin = NULL;

    v->env = lenv_new();

    v->formals = formals;
    v->body = body;
    return v;
}

lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);

    v->err = malloc(512);
    vsnprintf(v->err, 511, fmt, va);
    v->err = realloc(v->err, strlen(v->err) + 1);

    va_end(va);
    return v;
}

lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

int lval_eq(lval* x, lval* y) {
    if (x->type != y->type) return 0;

    switch (x->type) {
        case LVAL_INTEGER:
            return x->integer == y->integer;
        case LVAL_REAL:
            return x->real == y->real;
        case LVAL_ERR:
            return strcmp(x->err, y->err) == 0;
        case LVAL_SYM:
            return strcmp(x->sym, y->sym) == 0;
        case LVAL_STR:
            return strcmp(x->str, y->str) == 0;
        case LVAL_FUN:
            if (x->builtin || y->builtin) return x->builtin == y->builtin;
            else return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
        case LVAL_BOOLEAN:
            return x->integer == y->integer;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count) return 0;
            for (int i=0; i < x->count; i++) {
                if (!lval_eq(x->cell[i], y->cell[i])) return 0;
            }
            return 1;
            break;
    }
    return 0;
}

void lval_del(lval* v) {
    switch (v->type) {
        case LVAL_INTEGER:
            break;
        case LVAL_REAL:
            break;
        case LVAL_BOOLEAN:
            break;
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_SYM:
            free(v->sym);
            break;
        case LVAL_STR:
            free(v->str);
            break;
        case LVAL_FUN:
            if (!v->builtin) {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            for (int i=0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }
    free(v);
}

lval* lval_read_integer(mpc_ast_t* ast) {
    errno = 0;
    long value = strtol(ast->contents, NULL, 10);
    return errno != ERANGE ? lval_integer(value) : lval_err("Bad Integer");
}

lval* lval_read_real(mpc_ast_t* ast) {
    errno = 0;
    double value = strtod(ast->contents, NULL);
    return errno != ERANGE ? lval_real(value) : lval_err("Bad real");
}

lval* lval_read_boolean(mpc_ast_t* ast) {
    if (strcmp(ast->contents, "true") == 0) return lval_boolean(true);
    else if (strcmp(ast->contents, "false") == 0) return lval_boolean(false);
    else return lval_err("Invalid boolean");
}

lval* lval_read_str(mpc_ast_t* ast) {
    ast->contents[strlen(ast->contents) - 1] = '\0';
    char* unescaped = malloc(strlen(ast->contents + 1) + 1);
    strcpy(unescaped, ast->contents + 1);
    unescaped = mpcf_unescape(unescaped);
    lval* str = lval_str(unescaped);
    free(unescaped);
    return str;
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count -1] = x;
    return v;
}

lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        case LVAL_FUN:
            if (v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_INTEGER:
            x->integer = v->integer;
            break;
        case LVAL_REAL:
            x->real = v->real;
            break;
        case LVAL_BOOLEAN:
            x->integer = v->integer;
            break;
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;
        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;
        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i=0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }

    return x;
}

lval* lval_read(mpc_ast_t* ast) {
    if (strstr(ast->tag, "integer")) {
        return lval_read_integer(ast);
    } else if (strstr(ast->tag, "real")) {
        return lval_read_real(ast);
    } else if (strstr(ast->tag, "bool")) {
        return lval_read_boolean(ast);
    } else if (strstr(ast->tag, "symbol")) {
        return lval_sym(ast->contents);
    } else if (strstr(ast->tag, "string")) {
        return lval_read_str(ast);
    }

    lval* x = NULL;
    if (strcmp(ast->tag, ">") == 0) {
        x = lval_sexpr();
    }
    if (strstr(ast->tag, "sexpr")) {
        x = lval_sexpr();
    }
    if (strstr(ast->tag, "qexpr")) {
        x = lval_qexpr();
    }

    for (int i=0; i < ast->children_num; i++) {
        if (strcmp(ast->children[i]->contents, "(") == 0) continue;
        if (strcmp(ast->children[i]->contents, ")") == 0) continue;
        if (strcmp(ast->children[i]->contents, "{") == 0) continue;
        if (strcmp(ast->children[i]->contents, "}") == 0) continue;
        if (strcmp(ast->children[i]->tag, "regex") == 0) continue;
        x = lval_add(x, lval_read(ast->children[i]));
    }
    return x;
}

lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];
    memmove(&v->cell[i],
            &v->cell[i + 1],
            sizeof(lval*) * (v->count - i - 1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));

    e->parent = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

lval* lenv_get(lenv* e, lval* k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    if (e->parent) {
        return lenv_get(e->parent, k);
    } else {
        return lval_err("Unbound symbol '%s'", k->sym);
    }
}

lenv* lenv_copy(lenv* e) {
    lenv* n = malloc(sizeof(lenv));

    n->parent = e->parent;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);

    for (int i=0; i < e->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }

    return n;
}

void lenv_def(lenv* e, lval* k, lval* v) {
    while (e->parent) e = e->parent;
    lenv_put(e, k, v);
}

char* lenv_get_function_name(lenv* e, lval* v) {
    for (int i = 0; i < e->count; i++) {
        if(e->vals[i]->builtin == v->builtin) {
            return e->syms[i];
        }
    }
    return "Unknown";
}

void lenv_put(lenv* e, lval* k, lval* v) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);
    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

char* apply_op_unary_as_integers(lval* x, char* op) {
    if (strcmp(op, "-") == 0) x->integer = -x->integer;
    else if(strcmp(op, "inc") == 0) x->integer++;
    else if(strcmp(op, "dec") == 0) x->integer--;
    else return "Bad unary operation";
    return NULL;
}

char* apply_op_unary_as_reals(lval* x, char* op) {
    if (strcmp(op, "-") == 0) x->real = -x->real;
    else if(strcmp(op, "inc") == 0) x->real++;
    else if(strcmp(op, "dec") == 0) x->real--;
    else return "Bad unary operation";
    return NULL;

}

char* apply_op_unary(lval* x, char* op) {
    if (x->type == LVAL_INTEGER) {
        return apply_op_unary_as_integers(x, op);
    } else if (x->type == LVAL_REAL) {
        return apply_op_unary_as_reals(x, op);
    } else return "Invalid type";

    return NULL;
}

char* apply_to_binary_as_integers(lval* x, char* op, lval* y) {
    if (strcmp(op, "+") == 0) x->integer += y->integer;
    else if (strcmp(op, "-") == 0) x->integer -= y->integer;
    else if (strcmp(op, "*") == 0) x->integer *= y->integer;
    else if (strcmp(op, "^") == 0) x->integer = pow(x->integer, y->integer);
    else if (strcmp(op, "min") == 0) x->integer = x->integer <= y->integer ? x->integer : y->integer;
    else if (strcmp(op, "max") == 0) x->integer = x->integer >= y->integer ? x->integer : y->integer;
    else if (strcmp(op, "/") == 0) {
        if (y->integer == 0) return "Division by zero";
        x->integer /= y->integer;
    } else if (strcmp(op, "%") == 0) {
        if (y->integer == 0) return "Division by zero";
        x->integer = x->integer % y->integer;
    }
    else return "Invalid binary operation";
    return NULL;
}

char* apply_to_binary_as_reals(lval* x, char* op, lval* y) {
    if (strcmp(op, "+") == 0) x->real += y->real;
    else if (strcmp(op, "-") == 0) x->real -= y->real;
    else if (strcmp(op, "*") == 0) x->real *= y->real;
    else if (strcmp(op, "^") == 0) x->real = pow(x->real, y->real);
    else if (strcmp(op, "min") == 0) x->real = x->real <= y->real ? x->real : y->real;
    else if (strcmp(op, "max") == 0) x->real = x->real >= y->real ? x->real : y->real;
    else if (strcmp(op, "/") == 0) {
        if (y->real == 0) return "Division by zero";
        x->real /= y->real;
    } else if (strcmp(op, "%") == 0) {
        if (y->real == 0) return "Division by zero";
        x->real = fmod(x->real, y->real);
    }

    else return "Invalid binary operation";
    return NULL;
}

char* apply_op_binary(lval* x, char* op, lval* y) {
    if (x->type == LVAL_INTEGER && y->type == LVAL_INTEGER) {
        return apply_to_binary_as_integers(x, op, y);
    } else if (x->type == LVAL_REAL && y->type == LVAL_REAL) {
        return apply_to_binary_as_reals(x, op, y);
    } else if (x->type == LVAL_REAL && y->type == LVAL_INTEGER) {
        y->real = y->integer;
        y->type = LVAL_REAL;
        return apply_to_binary_as_reals(x, op, y);
    } else if (x->type == LVAL_INTEGER && y->type == LVAL_REAL) {
        x->type = LVAL_REAL;
        x->real = x->integer;
        return apply_to_binary_as_reals(x, op, y);
    } else return "Invalid type";
    return NULL;
}

lval* builtin_head(lenv* e, lval* a, int level) {
    LASSERT_NUM_ARGUMENTS("head", a, 1);
    LASSERT_TYPE("head", a, a->cell[0]->type, LVAL_QEXPR);
    LASSERT_NOT_EMPTY_LIST("head", a, a->cell[0]);

    lval* v = lval_take(a, 0);
    while (v->count > 1) lval_del(lval_pop(v, 1));
    return v;
}

lval* builtin_tail(lenv* e, lval* a, int level) {
    LASSERT_NUM_ARGUMENTS("tail", a, 1);
    LASSERT_TYPE("tail", a, a->cell[0]->type, LVAL_QEXPR);
    LASSERT_NOT_EMPTY_LIST("tail", a, a->cell[0]);

    lval* v = lval_take(a, 0);

    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_ord_integers(lval* x, char* op, lval* y) {
    if (strcmp(op, ">") == 0) return lval_boolean(x->integer > y->integer);
    else if (strcmp(op, "<") == 0) return lval_boolean(x->integer < y->integer);
    else if (strcmp(op, ">=") == 0) return lval_boolean(x->integer >= y->integer);
    else if (strcmp(op, "<=") == 0) return lval_boolean(x->integer <= y->integer);
    else return lval_err("Invalid ordering op '%s'", op);
}

lval* builtin_ord_reals(lval* x, char* op, lval* y) {
    if (strcmp(op, ">") == 0) return lval_boolean(x->real > y->real);
    else if (strcmp(op, "<") == 0) return lval_boolean(x->real < y->real);
    else if (strcmp(op, ">=") == 0) return lval_boolean(x->real >= y->real);
    else if (strcmp(op, "<=") == 0) return lval_boolean(x->real <= y->real);
    else return lval_err("Invalid ordering op '%s'", op);
}

lval* builtin_ord(lenv* e, lval* a, char* op, int level) {
    LASSERT(a, a->count >= 2, "Function '%s' passed in < 2 arguments", op);

    lval* rv;

    for (int i=0; i < a->count - 1; i++) {
        lval* x = a->cell[i];
        lval* y = a->cell[i + 1];

        if (x->type == LVAL_INTEGER && y->type == LVAL_INTEGER) {
            rv = builtin_ord_integers(x, op, y);
        } else if (x->type == LVAL_REAL && y->type == LVAL_REAL) {
            rv = builtin_ord_reals(x, op, y);
        } else if (x->type == LVAL_REAL && y->type == LVAL_INTEGER) {
            y->real = y->integer;
            y->type = LVAL_REAL;
            rv = builtin_ord_reals(x, op, y);
        } else if (x->type == LVAL_INTEGER && y->type == LVAL_REAL) {
            x->type = LVAL_REAL;
            x->real = x->integer;
            rv = builtin_ord_reals(x, op, y);
        } else {
            rv = lval_err("Invalid types for '%s': %s, %s",
                          ltype_name(x->type),
                          ltype_name(y->type));
            break;
        }
    }

    lval_del(a);
    return rv;
}

lval* builtin_list(lenv* e, lval* a, int level) {
    a->type = LVAL_QEXPR;
    return a;
}

lval* builtin_gt(lenv* e, lval* a, int level) {
    return builtin_ord(e, a ,">", level);
}

lval* builtin_lt(lenv* e, lval* a, int level) {
    return builtin_ord(e, a, "<", level);
}

lval* builtin_gte(lenv* e, lval* a, int level) {
    return builtin_ord(e, a, ">=", level);
}

lval* builtin_lte(lenv* e, lval* a, int level) {
    return builtin_ord(e, a, "<=", level);
}

lval* builtin_eq(lenv* e, lval* a, int level) {
    LASSERT(a, a->count >= 2, "Function '==' passed in < 2 arguments");

    int rc = true;
    for (int i=1; i < a->count; i++) {
        if (!lval_eq(a->cell[i - 1], a->cell[i])) {
            rc = false;
            break;
        }
    }

    lval_del(a);
    return lval_boolean(rc);
}

lval* builtin_neq(lenv* e, lval* a, int level) {
    LASSERT(a, a->count >= 2, "Function '!=' passed in < 2 arguments");

    bool rc = true;
    for (int i=0; i < a->count; i++) {
        for (int j=i + 1; j < a->count; j++) {
            if(lval_eq(a->cell[i], a->cell[j])) {
                rc = false;
                break;
            }
        }
    }
    lval_del(a);
    return lval_boolean(rc);
}

bool to_bool(lval* v) {
    switch (v->type) {
        case LVAL_INTEGER:
            return v->integer != 0;
        case LVAL_REAL:
            return v->real != 0;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            return v->count != 0;
        default:
            break;
    }
    return true;
}

lval* builtin_and(lenv* e, lval* a, int level) {
    LASSERT(a, a->count >= 2, "Function 'and' passed in < 2 arguments");

    if (!to_bool(a->cell[0])) return lval_boolean(false);

    lval* rv = a->cell[0];
    for (int i=0; i < a->count; i++) {
        if (!to_bool(a->cell[i])) return lval_boolean(false);
        rv = a->cell[i];
    }
    return rv;
}

lval* builtin_or(lenv* e, lval* a, int level) {
    LASSERT(a, a->count >= 2, "Function 'or' passed in < 2 arguments");

    for (int i=0; i < a->count; i++) {
        if (to_bool(a->cell[i])) return a->cell[i];
    }
    return lval_boolean(0);
}

lval* builtin_not(lenv* e, lval* a, int level) {
    LASSERT_NUM_ARGUMENTS("not", a, 1);
    return lval_integer(!to_bool(a->cell[0]));
}

lval* builtin_if(lenv* e, lval* a, int level) {
    LASSERT_NUM_ARGUMENTS("if", a, 3);
    LASSERT_TYPE("if", a, a->cell[0]->type, LVAL_BOOLEAN);
    LASSERT_TYPE("if", a, a->cell[1]->type, LVAL_QEXPR);
    LASSERT_TYPE("if", a, a->cell[2]->type, LVAL_QEXPR);

    lval* x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    if (a->cell[0]->integer) {
        x = lval_eval(e, lval_pop(a, 1), 0);
    } else {
        x = lval_eval(e, lval_pop(a, 2), 0);
    }

    lval_del(a);
    return x;
}

lval* builtin_print(lenv* e, lval* a, int level) {
    for (int i=0; i < a->count; i++) {
        lval_print(e, a->cell[i], true);
    }
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_println(lenv* e, lval* a, int level) {
    lval* rv = builtin_print(e, a, level);
    putchar('\n');
    return rv;
}

lval* builtin_error(lenv* e, lval* a, int level) {
    LASSERT_NUM_ARGUMENTS("error", a, 1);
    LASSERT_TYPE("error", a, a->cell[0]->type, LVAL_STR);

    lval* err = lval_err(a->cell[0]->str);
    lval_del(a);
    return err;
}

lval* builtin_eval(lenv* e, lval* a, int level) {
    LASSERT_NUM_ARGUMENTS("eval", a, 1);
    LASSERT_TYPE("eval", a, a->cell[0]->type, LVAL_QEXPR);

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x, 0);
}

lval* lval_join(lval* x, lval* y) {
    while (y->count) x = lval_add(x, lval_pop(y, 0));
    lval_del(y);
    return x;
}

lval* builtin_join(lenv* e, lval* a, int level) {
    for (int i=0; i < a->count; i++) {
        LASSERT_TYPE("join", a, a->cell[i]->type, LVAL_QEXPR);
    }

    lval* x = lval_pop(a, 0);
    while (a->count) x = lval_join(x, lval_pop(a, 0));

    lval_del(a);
    return x;
}

lval* builtin_cons(lenv* e, lval* a, int level) {
    lval* x = lval_pop(a, 0);
    lval* y = lval_take(a, 0);

    LASSERT_TYPE("cons", x, x->type, LVAL_QEXPR);

    x->count += 1;
    x->cell = realloc(x->cell, sizeof(lval*) * x->count);
    memmove(&x->cell[1],
            &x->cell[0],
            sizeof(lval*) * (x->count - 1));
    x->cell[0] = y;
    return x;
}

lval* builtin_len(lenv* e, lval* a, int level) {
    lval* x = lval_pop(a, 0);

    LASSERT_TYPE("len", x, x->type, LVAL_QEXPR);

    long length = x->count;

    lval_del(x);
    return lval_integer(length);
}

lval* builtin_init(lenv* e, lval* a, int level) {
    lval* x = lval_pop(a, 0);
    LASSERT_TYPE("init", x, x->type, LVAL_QEXPR);

    free(x->cell[x->count - 1]);
    x->count -= 1;

    lval_del(a);
    return x;
}

lval* builtin_lambda(lenv* e, lval* a, int level) {
    LASSERT_NUM_ARGUMENTS("lambda", a, 2);
    LASSERT_TYPE("lambda", a->cell[0], a->cell[0]->type, LVAL_QEXPR);
    LASSERT_TYPE("lambda", a->cell[1], a->cell[1]->type, LVAL_QEXPR);

    for (int i=0; i < a->cell[0]->count; i++) {
        LASSERT(a,
                (a->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, expected %s.",
                ltype_name(a->cell[0]->cell[i]->type),
                ltype_name(LVAL_SYM));
    }

    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

lval* builtin_op(lenv* e, lval* a, char* op, int level) {
    if (!a->count) {
        lval_del(a);
        return lval_err("No arguments passed to %s", op);
    }

    for (int i=0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_INTEGER && a->cell[i]->type != LVAL_REAL) {
            int type = a->cell[i]->type;
            lval_del(a);
            return lval_err("Cannot operate on %s", ltype_name(type));
        }
    }

    lval* x = lval_pop(a, 0);
    if (a->count == 0) {
        char* error = apply_op_unary(x, op);
        if (error) {
            lval_del(a);
            return lval_err(error);
        }
    }

    while (a->count > 0) {
        lval* y = lval_pop(a, 0);

        char* error = apply_op_binary(x, op, y);
        if (error) {
            lval_del(x);
            lval_del(y);
            x = lval_err(error);
        }
        lval_del(y);
    }
    lval_del(a);
    return x;
}

lval* builtin_add(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "+", level);
}

lval* builtin_sub(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "-", level);
}

lval* builtin_mul(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "*", level);
}

lval* builtin_div(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "/", level);
}

lval* builtin_mod(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "%", level);
}

lval* builtin_pow(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "^", level);
}

lval* builtin_min(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "min", level);
}

lval* builtin_max(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "max", level);
}

lval* builtin_inc(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "inc", level);
}

lval* builtin_dec(lenv* e, lval* a, int level) {
    return builtin_op(e, a, "dec", level);
}

lval* builtin_var(lenv* e, lval* a, char* func) {
    LASSERT_TYPE(func, a, a->cell[0]->type, LVAL_QEXPR);

    lval* syms = a->cell[0];
    for (int i=0; i < syms->count; i ++) {
        LASSERT(a,
                syms->cell[i]->type == LVAL_SYM,
                "Function '%s' cannot define non-symbol. "
                "Got %s, expected %s.", func,
                ltype_name(syms->cell[i]->type),
                ltype_name(LVAL_SYM));
    }

    LASSERT(a,
            syms->count == a->count - 1,
            "Function '%s' cannot define incorrect number of values to symbols. "
            "%d != %d.", func, a->count, syms->count);

    for (int i=0; i < syms->count; i++) {
        if (strcmp(func, "def") == 0) {
            lenv_def(e, syms->cell[i], a->cell[i + 1]);
        } else if (strcmp(func, "=") == 0) {
            lenv_put(e, syms->cell[i], a->cell[i + 1]);
        }
    }
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a, int level) {
    return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a, int level) {
    return builtin_var(e, a, "=");
}

lval* builtin_stable(lenv* e, lval* a, int level) {
    LASSERT_NUM_ARGUMENTS("stable", a, 0);

    lval_del(a);

    lval* table = lval_qexpr();

    for (int i = 0; i < e->count; i++) {
        lval_add(table, lval_sym(e->syms[i]));
        lval_add(table, lval_copy(e->vals[i]));
    }
    return table;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* lval_eval(lenv* e, lval* v, int level) {
    if (v->type == LVAL_SYM) {
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }

    if(v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(e, v, level + 1);
    }
    return v;
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin fun) {
    lval* k = lval_sym(name);
    lval* v = lval_fun(fun);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv* e) {
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "len", builtin_len);
    lenv_add_builtin(e, "init", builtin_init);

    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);

    lenv_add_builtin(e, "min", builtin_min);
    lenv_add_builtin(e, "max", builtin_max);

    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "^", builtin_pow);

    lenv_add_builtin(e, "and", builtin_and);
    lenv_add_builtin(e, "or", builtin_or);
    lenv_add_builtin(e, "not", builtin_not);

    lenv_add_builtin(e, "inc", builtin_inc);
    lenv_add_builtin(e, "dec", builtin_dec);

    lenv_add_builtin(e, "if", builtin_if);

    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_neq);
    lenv_add_builtin(e, ">", builtin_gt);
    lenv_add_builtin(e, "<", builtin_lt);
    lenv_add_builtin(e, ">=", builtin_gte);
    lenv_add_builtin(e, "<=", builtin_lte);

    lenv_add_builtin(e, "stable", builtin_stable);
    lenv_add_builtin(e, "lambda", builtin_lambda);

    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "println", builtin_println);
    lenv_add_builtin(e, "error", builtin_error);
}

lval* lval_call(lenv* e, lval* f, lval* a, int level) {
    if (f->builtin) return f->builtin(e, a, level);

    int given = a->count;
    int total = f->formals->count;

    while (a->count) {
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err("Function passed too many arguments. "
                            "Got %i, expected %i.", given, total);
        }

        lval* sym = lval_pop(f->formals, 0);
        if (strcmp(sym->sym, "&") == 0) {
            if (f->formals->count != 1) {
                lval_del(a);
                return lval_err("Function format invalid. "
                                "Symbol '&' not followed by single symbol.");
            }

            lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a, level));
            lval_del(sym);
            lval_del(nsym);
            break;
        }

        lval* val = lval_pop(a, 0);

        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    lval_del(a);
    if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
        if (f->formals->count != 2) {
            return lval_err("Function format invalid. "
                            "Symbol '&' not following by single symbol.");
        }

        lval_del(lval_pop(f->formals, 0));

        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();
        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    if (f->formals->count == 0) {
        f->env->parent = e;

        return builtin_eval(f->env,
                            lval_add(lval_sexpr(), lval_copy(f->body)),
                            level);
    } else {
        return lval_copy(f);
    }
}

lval* lval_eval_sexpr(lenv* e, lval* v, int level) {
    for (int i=0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i], level + 1);
        if (v->cell[i]->type == LVAL_ERR) return lval_take(v, i);
    }

    if (v->count == 0) return v;
    if (v->count == 1 && level == 1) {
        return lval_take(v, 0);
    }

    lval* first = lval_pop(v, 0);
    if (first->type != LVAL_FUN) {
        lval_del(v);
        lval_del(first);
        return lval_err("s-exp does not start with function");
    }

    lval* result = lval_call(e, first, v, level);
    lval_del(first);
    return result;
}

lval* eval(lenv* e, mpc_ast_t* ast) {
    return lval_eval(e, lval_read(ast), 0);
}

void lval_print_expr(lenv* e, lval* v, char open, char close) {
    putchar(open);
    for(int i=0; i < v->count; i++) {
        lval_print(e, v->cell[i], false);

        if (i != (v->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print_str(lval* v) {
    char* escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    free(escaped);
}

void lval_print(lenv* e, lval* v, bool for_builtin_print) {
    switch (v->type) {
        case LVAL_INTEGER:
            printf("%li", v->integer);
            break;
        case LVAL_REAL:
            printf("%lf", v->real);
            break;
        case LVAL_BOOLEAN:
            if (v->integer) printf("true");
            else printf("false");
            break;
        case LVAL_SYM:
            printf("%s", v->sym);
            break;
        case LVAL_STR:
            if (for_builtin_print) {
                printf("%s", v->str);
                break;
            }
            lval_print_str(v);
            break;
        case LVAL_FUN:
            if (v->builtin) {
                printf("<builtin %s>", lenv_get_function_name(e, v));
            } else {
                printf("(lambda ");
                lval_print(e, v->formals, false);
                putchar(' ');
                lval_print(e, v->body, false);
                putchar(')');
            }
            break;
        case LVAL_SEXPR:
            if (for_builtin_print && !v->count) break;
            lval_print_expr(e, v, '(', ')');
            break;
        case LVAL_QEXPR:
            if (for_builtin_print && !v->count) break;
            lval_print_expr(e, v, '{', '}');
            break;
        case LVAL_ERR:
            printf("Error: %s", v->err);
            break;
    }
}

void lval_println(lenv* e, lval* v, bool for_builtin_print) {
    lval_print(e, v, for_builtin_print);
    putchar('\n');
}
