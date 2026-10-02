%option noyywrap
%option never-interactive
%option nounistd

%{
#include <iostream>
#include <string>
#include "FlexRecognizer.hpp"

static bool g_valid = false;
static int  g_pathLen = 0;
static bool g_needSeg = false;
static std::string g_serverName;
%}

LETTER     [a-zA-Z]
SEG        {LETTER}+

%x SCHEME SERVER PATH

%%

^"nfs://" {
    g_valid = false;
    g_pathLen = 0;
    g_needSeg = false;
    g_serverName.clear();
    BEGIN(SERVER);
}

^[^n] {
    g_valid = false;
    g_serverName.clear();
    BEGIN(INITIAL);
}

<SERVER>{SEG} {
    g_pathLen += yyleng;
    g_serverName = yytext;
    g_needSeg = false;
    BEGIN(PATH);
}

<SERVER>. {
    g_valid = false;
    g_serverName.clear();
    BEGIN(INITIAL);
}

<PATH>"/" {
    if (g_needSeg) {
        g_valid = false;
        BEGIN(INITIAL);
    } else {
        g_pathLen++;
        g_needSeg = true;
    }
}

<PATH>{SEG} {
    g_pathLen += yyleng;
    g_needSeg = false;
    g_valid = (g_pathLen <= 63);
}

<PATH>(\n|$) {
    if (g_needSeg) {
        g_valid = false;
    }
    BEGIN(INITIAL);
}

<PATH>. {
    g_valid = false;
    BEGIN(INITIAL);
}

\n   { g_valid = false; }
.    { g_valid = false; }

<<EOF>> { return 0; }

%%

std::pair<bool, std::string> FlexRecognizer::test(std::string_view input) {
    g_valid = false;
    g_pathLen = 0;
    g_serverName.clear();

    std::string line(input);
    if (!line.empty() && line.back() == '\n')
        line.pop_back();

    if (line.empty()) return {false, ""};

    line += '\n';

    YY_BUFFER_STATE buf = yy_scan_string(line.c_str());
    BEGIN(INITIAL);
    yylex();
    yy_delete_buffer(buf);
    yyrestart(yyin);

    if (g_valid) {
        return {true, g_serverName};
    } else {
        return {false, ""};
    }
}