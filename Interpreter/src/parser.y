%{
#include "interpreter.hpp"
#include "ast.hpp"
#include "parser.tab.h"
#include <vector>
#include <cstdlib>
#include <cstring>

extern int yylex();
void yyerror(const char* s);

Program* g_program = nullptr;

static Expr* makeLit(Value v) {
    auto* e = new Expr(Expr::LIT);
    e->lit = std::move(v);
    return e;
}
static Expr* makeVar(char* name) {
    auto* e = new Expr(Expr::VAR);
    e->name = name;
    free(name);
    return e;
}
static Expr* makeBin(Expr* a, Expr* b, char op, CastDirection c) {
    auto* e = new Expr(Expr::BINARY);
    e->lhs = a;
    e->rhs = b;
    e->op = op;
    e->left_dot = c == CastDirection::LEFT;
    e->right_dot = c == CastDirection::RIGHT;
    return e;
}
static Expr* makeCmp(Expr* a, Expr* b, char op, CastDirection c) {
    auto* e = new Expr(Expr::COMPARE);
    e->lhs = a;
    e->rhs = b;
    e->op = op;
    e->left_dot = c == CastDirection::LEFT;
    e->right_dot = c == CastDirection::RIGHT;
    return e;
}
static Expr* makeSubscript(char* name, std::vector<Expr*>* args) {
    auto* e = new Expr(Expr::INDEX);
    e->name = name;
    free(name);
    if (args) {
        if (args->size() == 1 && (*args)[0]->kind == Expr::VAR) {
            e->kind = Expr::BRACKET_ID;
            e->name2 = (*args)[0]->name;
            delete (*args)[0];
        } else {
            e->elems = std::move(*args);
        }
        delete args;
    }
    return e;
}
static Expr* atomToCall(Expr* a) {
    if (!a) return a;
    if (a->kind == Expr::CALL) return a;
    auto* c = new Expr(Expr::CALL);
    c->name = a->name;
    if (a->kind == Expr::BRACKET_ID) {
        auto* v = new Expr(Expr::VAR);
        v->name = a->name2;
        c->elems.push_back(v);
    } else if (a->kind == Expr::INDEX || a->kind == Expr::SLICE) {
        c->elems = std::move(a->elems);
        if (a->lhs) {
            c->elems.insert(c->elems.begin(), a->lhs);
            a->lhs = nullptr;
        }
    }
    a->elems.clear();
    delete a;
    return c;
}
static Stmt* makeCallStmt(Expr* call) {
    auto* s = new Stmt(Stmt::CALL);
    s->expr = call;
    return s;
}
static Stmt* makeAssignStmt(Expr* lhs, Expr* rhs) {
    auto* e = new Expr(Expr::ASSIGN);
    e->lhs = lhs;
    e->rhs = rhs;
    auto* s = new Stmt(Stmt::ASSIGN);
    s->expr = e;
    return s;
}
static Stmt* makeDecl(BaseType type, char* name, long long size, char* type_name = nullptr) {
    auto* s = new Stmt(Stmt::DECL);
    s->decl_type = type;
    s->name = name;
    s->array_size = (int)size;
    if (type_name) {
        s->type_name = type_name;
        free(type_name);
    }
    free(name);
    return s;
}
static ParamInfo* makeParam(BaseType type, char* name, int by_ref, char* type_name = nullptr) {
    auto* p = new ParamInfo();
    p->type = type;
    p->name = name;
    p->by_ref = by_ref != 0;
    if (type_name) {
        p->type_name = type_name;
        free(type_name);
    }
    free(name);
    return p;
}
static Stmt* makeRecordDecl(char* name, std::vector<RecordField>* fields,
                            std::vector<ConversionDef>* to,
                            std::vector<ConversionDef>* from) {
    auto def = std::make_shared<RecordTypeDef>();
    def->name = name;
    def->fields = std::move(*fields);
    if (to) def->to_conversions = std::move(*to);
    if (from) def->from_conversions = std::move(*from);
    auto* s = new Stmt(Stmt::RECORD_DECL);
    s->name = name;
    s->record_def = def;
    delete fields;
    delete to;
    delete from;
    free(name);
    return s;
}
%}

%code requires {
#include "ast.hpp"
}

%define parse.error verbose

%union {
    long long num;
    char* str;
    Expr* expr;
    Stmt* stmt;
    Program* prog;
    std::vector<Stmt*>* stmts;
    std::vector<Expr*>* exprs;
    std::vector<RecordField>* fields;
    std::vector<ConversionDef>* convs;
    std::vector<ParamInfo>* params;
    RecordField* rfld;
    ConversionDef* cdef;
    ParamInfo* param;
    int btype;
    CastDirection cast;
}

%token <num> INTEGER
%token <str> ID STRING_LIT
%token TRUE_LIT FALSE_LIT UNDEF_LIT
%token LOGIC NUMERIC STRING_T RECORD DATA CONVERSION TO FROM
%token BLOCK UNBLOCK PROC
%token MOVEUP MOVEDOWN MOVERIGHT MOVELEFT
%token PINGUP PINGDOWN PINGRIGHT PINGLEFT VISION VOICE
%token LBRACKET RBRACKET LPAREN RPAREN LBRACE RBRACE
%token COMMA SEMICOLON DOT ASSIGN AMP NL
%token <cast> PLUS MINUS STAR SLASH CARET LT GT QMARK EXCL

%type <prog> program item_list
%type <stmt> stmt simple_stmt block_stmt loop_stmt proc_decl assign_stmt call_stmt decl record_decl
%type <expr> expr atom
%type <str> sys_name
%type <btype> type_name
%type <num> opt_array opt_ref
%type <stmts> local_items block_content
%type <exprs> arg_list slice_inner
%type <params> opt_params param_list
%type <param> param_item
%type <fields> field_list
%type <rfld> field_item
%type <convs> conv_list
%type <cdef> conv_item

%right ASSIGN
%left QMARK EXCL
%left LT GT
%left PLUS MINUS
%left STAR SLASH
%left CARET
%right UMINUS

%%

program:
    item_list                       { $$ = $1; g_program = $1; }
  ;

item_list:
    /* empty */                     { $$ = new Program(); }
  | item_list NL                    { $$ = $1; }
  | item_list stmt NL               { $$ = $1; if ($2) $$->stmts.push_back($2); }
  | item_list proc_decl             { $$ = $1; if ($2) $$->stmts.push_back($2); }
  ;

stmt:
    simple_stmt
  | block_stmt
  | loop_stmt
  ;

simple_stmt:
    decl
  | record_decl
  | assign_stmt
  | call_stmt
  ;

opt_array:
    /* empty */                     { $$ = -1; }
  | LBRACKET INTEGER RBRACKET       { $$ = $2; }
  ;

decl:
    type_name ID opt_array SEMICOLON { $$ = makeDecl((BaseType)$1, $2, $3); }
  | ID ID opt_array SEMICOLON       { $$ = makeDecl(BaseType::RECORD, $2, $3, $1); }
  ;

type_name:
    LOGIC                           { $$ = (int)BaseType::LOGIC; }
  | NUMERIC                         { $$ = (int)BaseType::NUMERIC; }
  | STRING_T                        { $$ = (int)BaseType::STRING; }
  ;

record_decl:
    RECORD ID DATA LBRACKET field_list RBRACKET SEMICOLON
        { $$ = makeRecordDecl($2, $5, nullptr, nullptr); }
  | RECORD ID DATA LBRACKET field_list RBRACKET CONVERSION TO conv_list SEMICOLON
        { $$ = makeRecordDecl($2, $5, $9, nullptr); }
  | RECORD ID DATA LBRACKET field_list RBRACKET CONVERSION FROM conv_list SEMICOLON
        { $$ = makeRecordDecl($2, $5, nullptr, $9); }
  | RECORD ID DATA LBRACKET field_list RBRACKET CONVERSION TO conv_list CONVERSION FROM conv_list SEMICOLON
        { $$ = makeRecordDecl($2, $5, $9, $12); }
  ;

field_list:
    field_item                      { $$ = new std::vector<RecordField>{*$1}; delete $1; }
  | field_list COMMA field_item     { $$ = $1; $$->push_back(*$3); delete $3; }
  ;

field_item:
    type_name ID {
        $$ = new RecordField();
        $$->type = (BaseType)$1;
        $$->name = $2;
        free($2);
    }
  | ID ID {
        $$ = new RecordField();
        $$->type = BaseType::RECORD;
        $$->type_name = $1;
        $$->name = $2;
        free($1); free($2);
    }
  ;

conv_list:
    conv_item                       { $$ = new std::vector<ConversionDef>{*$1}; delete $1; }
  | conv_list COMMA conv_item       { $$ = $1; $$->push_back(*$3); delete $3; }
  ;

conv_item:
    type_name ID                    { $$ = new ConversionDef{(BaseType)$1, $2}; free($2); }
  | ID ID                           { $$ = new ConversionDef{BaseType::RECORD, $2}; free($1); free($2); }
  ;

assign_stmt:
    atom ASSIGN expr                { $$ = makeAssignStmt($1, $3); }
  ;

call_stmt:
    atom DOT                        { $$ = makeCallStmt(atomToCall($1)); }
  | VOICE LPAREN expr RPAREN DOT {
        auto* e = new Expr(Expr::CALL);
        e->name = "VOICE";
        e->voice_parens = true;
        e->elems.push_back($3);
        $$ = makeCallStmt(e);
    }
  ;

sys_name:
    MOVEUP                          { $$ = strdup("MOVEUP"); }
  | MOVEDOWN                        { $$ = strdup("MOVEDOWN"); }
  | MOVERIGHT                       { $$ = strdup("MOVERIGHT"); }
  | MOVELEFT                        { $$ = strdup("MOVELEFT"); }
  | PINGUP                          { $$ = strdup("PINGUP"); }
  | PINGDOWN                        { $$ = strdup("PINGDOWN"); }
  | PINGRIGHT                       { $$ = strdup("PINGRIGHT"); }
  | PINGLEFT                        { $$ = strdup("PINGLEFT"); }
  | VISION                          { $$ = strdup("VISION"); }
  | VOICE                           { $$ = strdup("VOICE"); }
  ;

atom:
    ID                              { $$ = makeVar($1); }
  | sys_name                        { $$ = makeVar($1); }
  | ID LBRACKET RBRACKET            { $$ = makeSubscript($1, new std::vector<Expr*>()); }
  | ID LBRACKET arg_list RBRACKET   { $$ = makeSubscript($1, $3); }
  | ID LBRACKET LBRACKET slice_inner RBRACKET RBRACKET {
        auto* e = new Expr(Expr::SLICE);
        e->name = $1;
        free($1);
        e->elems = std::move(*$4);
        delete $4;
        $$ = e;
    }
  | sys_name LBRACKET RBRACKET      { $$ = makeSubscript($1, new std::vector<Expr*>()); }
  | sys_name LBRACKET arg_list RBRACKET { $$ = makeSubscript($1, $3); }
  ;

arg_list:
    expr                            { $$ = new std::vector<Expr*>{$1}; }
  | expr AMP                        { $$ = new std::vector<Expr*>{$1}; }
  | arg_list COMMA expr             { $$ = $1; $$->push_back($3); }
  | arg_list COMMA expr AMP         { $$ = $1; $$->push_back($3); }
  ;

opt_params:
    { $$ = new std::vector<ParamInfo>(); }
  | LBRACKET RBRACKET               { $$ = new std::vector<ParamInfo>(); }
  | LBRACKET param_list RBRACKET    { $$ = $2; }
  ;

param_list:
    param_item                      { $$ = new std::vector<ParamInfo>{*$1}; delete $1; }
  | param_list COMMA param_item     { $$ = $1; $$->push_back(*$3); delete $3; }
  ;

opt_ref:
    /* empty */                     { $$ = 0; }
  | AMP                             { $$ = 1; }
  ;

param_item:
    type_name ID opt_ref            { $$ = makeParam((BaseType)$1, $2, (int)$3); }
  | ID ID opt_ref                   { $$ = makeParam(BaseType::RECORD, $2, (int)$3, $1); }
  ;

proc_decl:
    PROC ID opt_params NL local_items DOT NL {
        auto* s = new Stmt(Stmt::PROC_DEF);
        s->name = $2;
        s->params = std::move(*$3);
        s->body = std::move(*$5);
        delete $3; delete $5;
        free($2);
        $$ = s;
    }
  | PROC ID opt_params stmt DOT NL {
        auto* s = new Stmt(Stmt::PROC_DEF);
        s->name = $2;
        s->params = std::move(*$3);
        s->body.push_back($4);
        delete $3;
        free($2);
        $$ = s;
    }
  ;

local_items:
    { $$ = new std::vector<Stmt*>(); }
  | local_items NL                  { $$ = $1; }
  | local_items stmt NL             { $$ = $1; $$->push_back($2); }
  ;

loop_stmt:
    LBRACE expr RBRACE stmt {
        auto* s = new Stmt(Stmt::LOOP);
        s->cond = $2;
        s->body.push_back($4);
        $$ = s;
    }
  | LBRACE expr RBRACE NL stmt {
        auto* s = new Stmt(Stmt::LOOP);
        s->cond = $2;
        s->body.push_back($5);
        $$ = s;
    }
  ;

block_stmt:
    BLOCK UNBLOCK                   { $$ = new Stmt(Stmt::BLOCK); }
  | BLOCK NL UNBLOCK                { $$ = new Stmt(Stmt::BLOCK); }
  | BLOCK block_content UNBLOCK {
        auto* s = new Stmt(Stmt::BLOCK);
        s->body = std::move(*$2);
        delete $2;
        $$ = s;
    }
  | BLOCK NL block_content UNBLOCK {
        auto* s = new Stmt(Stmt::BLOCK);
        s->body = std::move(*$3);
        delete $3;
        $$ = s;
    }
  ;

block_content:
    stmt                            { $$ = new std::vector<Stmt*>{$1}; }
  | block_content COMMA stmt        { $$ = $1; $$->push_back($3); }
  | block_content COMMA NL stmt     { $$ = $1; $$->push_back($4); }
  | block_content NL stmt           { $$ = $1; $$->push_back($3); }
  | block_content NL                { $$ = $1; }
  ;

slice_inner:
    expr                            { $$ = new std::vector<Expr*>{$1}; }
  | slice_inner COMMA expr          { $$ = $1; $$->push_back($3); }
  ;

expr:
    INTEGER                         { $$ = makeLit(Value::makeNumeric($1)); }
  | TRUE_LIT                        { $$ = makeLit(Value::makeLogic(true)); }
  | FALSE_LIT                       { $$ = makeLit(Value::makeLogic(false)); }
  | UNDEF_LIT                       { $$ = makeLit(Value::makeUndef()); }
  | STRING_LIT                      { $$ = makeLit(Value::makeString($1)); free($1); }
  | atom                            { $$ = $1; }
  | MINUS expr %prec UMINUS {
        auto* e = new Expr(Expr::UNARY);
        e->op = '-';
        e->rhs = $2;
        $$ = e;
    }
  | atom ASSIGN expr {
        auto* e = new Expr(Expr::ASSIGN);
        e->lhs = $1;
        e->rhs = $3;
        $$ = e;
    }
  | expr PLUS expr                  { $$ = makeBin($1, $3, '+', $2); }
  | expr MINUS expr                 { $$ = makeBin($1, $3, '-', $2); }
  | expr STAR expr                  { $$ = makeBin($1, $3, '*', $2); }
  | expr SLASH expr                 { $$ = makeBin($1, $3, '/', $2); }
  | expr CARET expr                 { $$ = makeBin($1, $3, '^', $2); }
  | expr LT expr                    { $$ = makeCmp($1, $3, '<', $2); }
  | expr GT expr                    { $$ = makeCmp($1, $3, '>', $2); }
  | expr QMARK expr                 { $$ = makeCmp($1, $3, '?', $2); }
  | expr EXCL expr                  { $$ = makeCmp($1, $3, '!', $2); }
  ;

%%

void yyerror(const char* s) {
    g_parse_error = s;
}