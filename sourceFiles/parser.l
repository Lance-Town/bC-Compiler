%{
/*
 * @author Lance Townsend
 *
 * @brief Used in lexical analysis
 *
*/


#include "scanType.h"
#include "treeNodes.h"

#include "parser.tab.h" // This has to be the last include

using namespace std;

#define YY_DECL extern "C" int yylex()
#define YYSTYPE int

int line=1;
char *lastToken=(char *)"";
extern int numErrors;

/*
 * @brief initialize the value of the TokenData node
 *
 * @param linenum - line number the terminal was found on
 * @param tokenClass - which type of terminal was found, ex. a '+' or a 'char'
 * @param svalue - string containing the yytext
 *
 * @return tokenClass value
*/
int setValue(int linenum, int tokenClass, char *svalue) {
    yylval.tinfo = new TokenData;

    yylval.tinfo->tokenclass = tokenClass;
    yylval.tinfo->linenum = linenum;
    yylval.tinfo->tokenstr = strdup(svalue);
    yylval.tinfo->cvalue = svalue[0];
    yylval.tinfo->nvalue = atoi(svalue);
    yylval.tinfo->svalue = strdup(svalue);

    yylval.tinfo->tokenstr = lastToken = strdup(svalue);

    switch (tokenClass) {

    case NUMCONST:
         yylval.tinfo->nvalue = atoi(svalue);
         break;

    case '\\' || '*' || '+' || '-' || '=' || 
            ';' || '(' || ')' || '{' || '}' 
            || '<' || '>' || ',' || ':' || 
            '?' || '%' || '[' || ']' || '/':
         yylval.tinfo->cvalue = svalue[0];
         // yylval.tinfo->nvalue = 1;
         break;
    case BOOLCONST:
         if (yylval.tinfo->cvalue == 't') {
            yylval.tinfo->nvalue = 1;
            yylval.tinfo->tokenstr = (char*)"true";
            yylval.tinfo->svalue = (char*)"true";
         } else {
            yylval.tinfo->nvalue = 0;
            yylval.tinfo->tokenstr = (char*)"false";
            yylval.tinfo->svalue = (char*)"false";
         }
         
         break;

    case STRINGCONST:
         yylval.tinfo->nvalue = strlen(svalue)-2;
         yylval.tinfo->tokenstr = strdup(&svalue[1]);
         yylval.tinfo->tokenstr[yylval.tinfo->nvalue] ='\0';
         break;

    case CHARCONST:
         yylval.tinfo->cvalue = svalue[1];
         yylval.tinfo->nvalue = 1;
         if (svalue[1] == '\\') {
            if (svalue[2] == 'n') yylval.tinfo->cvalue ='\n';
            else if (svalue[2] == '0') yylval.tinfo->cvalue = '\0';
         }
    }

    return tokenClass;
}

%}

%option noyywrap

letter      [a-zA-Z]
digit    [0-9]
quote    [\']
underbar [_]
letdig      {digit}|{letter}|{underbar}
whitespace  [\ \t]
op [\;\{\}\(\)\<\>\=\,\:\-\+\?\*\/\%\\]    

%%

   /*  Booleans */
"and"           { return setValue(line, AND, yytext); }
"or"            { return setValue(line, OR, yytext); }

   /* Control Flow */
"if"            { return setValue(line, IF, yytext); }
"else"          { return setValue(line, ELSE, yytext); }
"break"         { return setValue(line, BREAK, yytext); }
"by"         { return setValue(line, BY, yytext); }
"do"         { return setValue(line, DO, yytext); }
"to"         { return setValue(line, TO, yytext); }
"while"         { return setValue(line, WHILE, yytext); }
"for"         { return setValue(line, FOR, yytext); }
"then"         { return setValue(line, THEN, yytext); }
"return"         { return setValue(line, RETURN, yytext); }


   /* Comments */
"//".*          {}

   /* Operands, Equalities */

{op}          { return setValue(line, yytext[0], yytext); }
"["            { return setValue(line, '[', yytext); } 
"]"            { return setValue(line, ']', yytext); } 
"!="            { return setValue(line, NEQ, yytext); }
"=="            { return setValue(line, EQ, yytext); }
"<="            { return setValue(line, LEQ, yytext); }
">="            { return setValue(line, GEQ, yytext); }
"--"            { return setValue(line, DEC, yytext); }
"++"            { return setValue(line, INC, yytext); }
"+="            { return setValue(line, ADDASS, yytext); }
"-="            { return setValue(line, SUBASS, yytext); }
"*="            { return setValue(line, MULASS, yytext); }
"/="            { return setValue(line, DIVASS, yytext); }
":>:"            { return setValue(line, MAX, yytext); }
":<:"            { return setValue(line, MIN, yytext); }
"not"           { return setValue(line, NOT, yytext); }

   /* Types */
"char"          { return setValue(line, CHAR, yytext); }
"int"           { return setValue(line, INT, yytext); }
"static"           { return setValue(line, STATIC, yytext); }
"bool"          { return setValue(line, BOOL, yytext); }
"true"          { return setValue(line, BOOLCONST, yytext); }
"false"          { return setValue(line, BOOLCONST, yytext); }

   /* Strings, Characters */
\'\\?.\' {return setValue(line, CHARCONST, yytext); }
["](\\[^\n]|[^\"\n\\])*["] {return setValue(line, STRINGCONST, yytext); }

   /* Precompilers */
^"#DRBC ".*     { return setValue(line, PRECOMPILER, yytext); }
^"#DRBCRUN ".*     { return setValue(line, PRECOMPILER, yytext); }

   /* Numbers */
{digit}+        { return setValue(line, NUMCONST, yytext); }


   /* Variables */
{letter}{letdig}* { return setValue(line, ID, yytext); }

   /* Whitespace */
\n              {  line++; }
{whitespace}+   ;

   /* Errors */
.               { printf("ERROR(%d): Token error, invalid or misplaced input character: \'%c\'. Character Ignored.\n", line, yytext[0]);}


