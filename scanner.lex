%{
/* Definition section */
#include "Types.h"
#include <iostream>
#include "parser.tab.hpp"
%}

%option yylineno
%option noyywrap

digit                         ([0-9])
letter                        ([a-zA-Z])
whitespace                    ([\n\r \t])
printableChar                 ((\\[nrt\\"0])|(\\(x[0-7][0-9a-fA-F]))|([ -!#-\[\]-~\t]))
string                         ([^\n\r\"\\]|\\[rnt"\\])+
%%
int                         {yylval = new Node();
                                return INT;}
byte                        {yylval = new Node();
                                return BYTE;}
b                           {yylval = new Node();
                                return B;}
bool                        {yylval = new Node();
                                return BOOL;}
and                         {yylval = new Node();
                                return AND;}
or                          {yylval = new Node();
                                return OR;}
not                         {yylval = new Node();
                                return NOT;}
true                        {yylval = new Node();
                                return TRUE;}
false                       {yylval = new Node();
                                return FALSE;}
return                      {yylval = new Node();
                                return RETURN;}
if                          {yylval = new Node();
                                return IF;}
else                        {yylval = new Node();
                                return ELSE;}
while                       {yylval = new Node();
                                return WHILE;}
break                       {yylval = new Node();
                                return BREAK;}
continue                    {yylval = new Node();
                                return CONTINUE;}
;                           {yylval = new Node();
                                return SC;}
\(                          {yylval = new Node();
                                return LPAREN;}
\)                          {yylval = new Node();
                                return RPAREN;}
\{                          {yylval = new Node();
                                return LBRACE;}
\}                          {yylval = new Node();
                                return RBRACE;}
=                           {yylval = new Node();
                                return ASSIGN;}
(<=|>=|<|>)                 {yylval = new Node();
                                return RELOP;}
(==|!=)                     {yylval = new Node();
                                return EQUAL_N;}
[+\-]                       {yylval = new Node();
                                return PLUSMINUS;}
[*\/]                       {yylval = new Node();
                                return MULTDIV;}
{letter}({letter}|{digit})* {yylval = new Node();
                                return ID;}
(([1-9]{digit}*)|0)         {yylval = new Node();
                                return NUM;}
\"{string}\"                {yylval = new Node();
                                return STRING;}
{whitespace}                 ;
\/\/[^\r\n]*[\r|\n|\r\n]?    ;
.                            { output::errorLex(yylineno);
                                    exit(1);}

%%
