#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

FunctionList *g_program = NULL;

static char *strdup_s(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *p = (char *)malloc(len);
    if (!p) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memcpy(p, s, len);
    return p;
}

Expr *new_int_expr(int value) {
    Expr *e = (Expr *)calloc(1, sizeof(Expr));
    e->kind = EXPR_INT;
    e->u.int_value = value;
    return e;
}

Expr *new_var_expr(const char *name) {
    Expr *e = (Expr *)calloc(1, sizeof(Expr));
    e->kind = EXPR_VAR;
    e->u.var_name = strdup_s(name);
    return e;
}

Expr *new_binop_expr(BinOpKind op, Expr *lhs, Expr *rhs) {
    Expr *e = (Expr *)calloc(1, sizeof(Expr));
    e->kind = EXPR_BINOP;
    e->u.binop.op = op;
    e->u.binop.lhs = lhs;
    e->u.binop.rhs = rhs;
    return e;
}

Expr *new_call_expr(const char *func_name, ExprList *args) {
    Expr *e = (Expr *)calloc(1, sizeof(Expr));
    e->kind = EXPR_CALL;
    e->u.call.func_name = strdup_s(func_name);
    e->u.call.args = args;
    return e;
}

ExprList *expr_list_append(ExprList *list, Expr *expr) {
    ExprList *node = (ExprList *)calloc(1, sizeof(ExprList));
    node->expr = expr;
    node->next = NULL;

    if (!list) return node;

    ExprList *cur = list;
    while (cur->next) cur = cur->next;
    cur->next = node;
    return list;
}

Stmt *new_expr_stmt(Expr *e) {
    Stmt *s = (Stmt *)calloc(1, sizeof(Stmt));
    s->kind = STMT_EXPR;
    s->u.expr = e;
    return s;
}

Stmt *new_return_stmt(Expr *e) {
    Stmt *s = (Stmt *)calloc(1, sizeof(Stmt));
    s->kind = STMT_RETURN;
    s->u.expr = e;
    return s;
}

Stmt *new_vardecl_stmt(const char *name) {
    Stmt *s = (Stmt *)calloc(1, sizeof(Stmt));
    s->kind = STMT_VARDECL;
    s->u.vardecl.var_name = strdup_s(name);
    return s;
}

Stmt *new_assign_stmt(const char *name, Expr *value) {
    Stmt *s = (Stmt *)calloc(1, sizeof(Stmt));
    s->kind = STMT_ASSIGN;
    s->u.assign.var_name = strdup_s(name);
    s->u.assign.value = value;
    return s;
}

StmtList *stmt_list_append(StmtList *list, Stmt *stmt) {
    if (!stmt) return list;
    if (!list) {
        list = (StmtList *)calloc(1, sizeof(StmtList));
        list->head = list->tail = NULL;
    }
    if (!list->head) {
        list->head = list->tail = stmt;
    } else {
        list->tail->next = stmt;
        list->tail = stmt;
    }
    return list;
}

ParamList *param_list_append(ParamList *list, const char *name) {
    Param *p = (Param *)calloc(1, sizeof(Param));
    p->name = strdup_s(name);
    p->next = NULL;

    if (!list) {
        list = (ParamList *)calloc(1, sizeof(ParamList));
        list->head = list->tail = NULL;
    }
    if (!list->head) {
        list->head = list->tail = p;
    } else {
        list->tail->next = p;
        list->tail = p;
    }
    return list;
}

Function *new_function(const char *name, ParamList *params, StmtList *body) {
    Function *f = (Function *)calloc(1, sizeof(Function));
    f->name = strdup_s(name);
    f->params = params;
    f->body = body;
    f->next = NULL;
    return f;
}

FunctionList *function_list_append(FunctionList *list, Function *func) {
    if (!func) return list;
    if (!list) {
        list = (FunctionList *)calloc(1, sizeof(FunctionList));
        list->head = list->tail = NULL;
    }
    if (!list->head) {
        list->head = list->tail = func;
    } else {
        list->tail->next = func;
        list->tail = func;
    }
    return list;
}
