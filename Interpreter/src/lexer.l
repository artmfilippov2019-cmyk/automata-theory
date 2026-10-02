%{
#include "interpreter.hpp"
#include "parser.tab.h"
#include <string>
#include <cstring>

static std::string string_buf;

static int op(int token, CastDirection dir) {
    yylval.cast = dir;
    return token;
}
%}

%option noyywrap
%option nounput
%option noinput

%x STR

DIGIT   [0-9]
ID      [A-Za-z_][A-Za-z0-9_]*
WS      [ \t]

%%

{WS}+           ;
\r              ;
"#"[^\n]*       ;

\n              { return NL; }

"TRUE"          { return TRUE_LIT; }
"FALSE"         { return FALSE_LIT; }
"UNDEF"         { return UNDEF_LIT; }
"LOGIC"         { return LOGIC; }
"NUMERIC"       { return NUMERIC; }
"STRING"        { return STRING_T; }
"RECORD"        { return RECORD; }
"DATA"          { return DATA; }
"CONVERSION"    { return CONVERSION; }
"TO"            { return TO; }
"FROM"          { return FROM; }
"BLOCK"         { return BLOCK; }
"UNBLOCK"       { return UNBLOCK; }
"PROC"          { return PROC; }
"MOVEUP"        { return MOVEUP; }
"MOVEDOWN"      { return MOVEDOWN; }
"MOVERIGHT"     { return MOVERIGHT; }
"MOVELEFT"      { return MOVELEFT; }
"PINGUP"        { return PINGUP; }
"PINGDOWN"      { return PINGDOWN; }
"PINGRIGHT"     { return PINGRIGHT; }
"PINGLEFT"      { return PINGLEFT; }
"VISION"        { return VISION; }
"VOICE"         { return VOICE; }

{DIGIT}+        { yylval.num = std::strtoll(yytext, nullptr, 10); return INTEGER; }

\"              { BEGIN(STR); string_buf.clear(); }
<STR>\\n        { string_buf += '\n'; }
<STR>\\t        { string_buf += '\t'; }
<STR>\\r        { string_buf += '\r'; }
<STR>\\\\       { string_buf += '\\'; }
<STR>\\\"       { string_buf += '"'; }
<STR>\\0        { string_buf += '\0'; }
<STR>\\.        { string_buf += yytext[1]; }
<STR>[^\\\n\"]+ { string_buf += yytext; }
<STR>\"         { BEGIN(INITIAL); yylval.str = strdup(string_buf.c_str()); return STRING_LIT; }
<STR>\n         { g_parse_error = "Unterminated string"; return 0; }

{ID}            { yylval.str = strdup(yytext); return ID; }

"."{WS}*"+"({WS}*".")?  { return op(PLUS, CastDirection::LEFT); }
"."{WS}*"-"({WS}*".")?  { return op(MINUS, CastDirection::LEFT); }
"."{WS}*"*"({WS}*".")?  { return op(STAR, CastDirection::LEFT); }
"."{WS}*"/"({WS}*".")?  { return op(SLASH, CastDirection::LEFT); }
"."{WS}*"^"({WS}*".")?  { return op(CARET, CastDirection::LEFT); }
"."{WS}*"<"({WS}*".")?  { return op(LT, CastDirection::LEFT); }
"."{WS}*">"({WS}*".")?  { return op(GT, CastDirection::LEFT); }
"."{WS}*"?"({WS}*".")?  { return op(QMARK, CastDirection::LEFT); }
"."{WS}*"!"({WS}*".")?  { return op(EXCL, CastDirection::LEFT); }

"+"{WS}*"."     { return op(PLUS, CastDirection::RIGHT); }
"-"{WS}*"."     { return op(MINUS, CastDirection::RIGHT); }
"*"{WS}*"."     { return op(STAR, CastDirection::RIGHT); }
"/"{WS}*"."     { return op(SLASH, CastDirection::RIGHT); }
"^"{WS}*"."     { return op(CARET, CastDirection::RIGHT); }
"<"{WS}*"."     { return op(LT, CastDirection::RIGHT); }
">"{WS}*"."     { return op(GT, CastDirection::RIGHT); }
"?"{WS}*"."     { return op(QMARK, CastDirection::RIGHT); }
"!"{WS}*"."     { return op(EXCL, CastDirection::RIGHT); }

"["             { return LBRACKET; }
"]"             { return RBRACKET; }
"("             { return LPAREN; }
")"             { return RPAREN; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
","             { return COMMA; }
";"             { return SEMICOLON; }
"."             { return DOT; }
"="             { return ASSIGN; }
"&"             { return AMP; }
"+"             { return op(PLUS, CastDirection::NONE); }
"-"             { return op(MINUS, CastDirection::NONE); }
"*"             { return op(STAR, CastDirection::NONE); }
"/"             { return op(SLASH, CastDirection::NONE); }
"^"             { return op(CARET, CastDirection::NONE); }
"<"             { return op(LT, CastDirection::NONE); }
">"             { return op(GT, CastDirection::NONE); }
"?"             { return op(QMARK, CastDirection::NONE); }
"!"             { return op(EXCL, CastDirection::NONE); }

.               { g_parse_error = std::string("Unknown character: ") + yytext; return 0; }

%%

static YY_BUFFER_STATE lexer_buf = nullptr;

void lexer_set_input(const std::string& src) {
    if (lexer_buf) {
        yy_delete_buffer(lexer_buf);
        lexer_buf = nullptr;
    }
    std::string s = src;
    if (s.empty() || s.back() != '\n') s += '\n';
    lexer_buf = yy_scan_string(s.c_str());
    BEGIN(INITIAL);
    string_buf.clear();
}
