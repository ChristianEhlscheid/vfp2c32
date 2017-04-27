%{
/* needed headers for flex to work correclty */
#include "standard.h"
#define ECHOUPPER (void) fwrite( _strupr(yytext), yyleng, 1, yyout )
#define ECHOSTRING(pString) (void) fwrite(#pString,sizeof(#pString)-1, 1, yyout)
#define ECHOAFTER(pChar)	tmpText = strchr(yytext,##pChar)+1; (void) fwrite(tmpText, strlen(tmpText), 1, yyout)
#define ECHOREST  (void) fwrite( yytext, strlen(yytext), 1, yyout )
#define TRUNCRTEXT(pString) truncrtext(yytext,##pString)
#define REMOVEPARENS()		tmpText = yytext+strlen(yytext); \
							while (*yytext == '(') { yytext++; } \
							while (*tmpText == ')') { *tmpText = '\0'; tmpText--; }

void _stdcall truncrtext(char *pText,char *pRemove)
{
	char *tmpString, *tmpBegin;
	int nStrLen, xj, nFound = 0;
	nStrLen = strlen(pRemove);
	tmpString = pText + strlen(pText)-1;
	while (1)
	{
		tmpBegin = pRemove;
		for (xj=0; xj<nStrLen; xj++)
		{
			if (*tmpString == *tmpBegin)
			{
				nFound = 1;
				*tmpString = '\0';
				tmpString--;
				break;
			}
			else
				tmpBegin++;
		}
		if (nFound == 0)
			break;
		else
			nFound = 0;
	}
}

static char *tmpText;
static int gnPreFlag = 0;
%}

%option stack
%option prefix="de"
%option noyywrap
%option pointer
%option stack
%option 8bit
%option caseful
%option warn
%option never-interactive

%x PREPROC

preline		^#{precommand}
precommand  define|ifndef|ifdef|if|else|elif|endif|undef
identifier	[a-zA-Z_][a-zA-Z_0-9]*
newline		\n\r|\n|\r
backslash	[\\]
digit		{integer}|{hexdigit}
integer		[0-9]+
hexdigit	0[xX][a-fA-F0-9]+
digitpf		[uU][lL]|[uU]|[lL]
ws			[ \t\f\v\a]+
%%

{preline}	{ ECHOUPPER; BEGIN(PREPROC); gnPreFlag = 0; }
{newline}	{ /* ignore */ }
.			{ /* ignore */ }

<PREPROC>{
{backslash}$						{ ECHO; gnPreFlag = 1; }
{newline}							{ ECHO; if (gnPreFlag-- == 0) BEGIN(INITIAL); }
{ws}								{ ECHO; }
"//"								{ ECHOSTRING(&&); }
"("{identifier}")"{digit}			{ ECHOAFTER(')'); }
"("{identifier}")"{digit}{digitpf}	{ TRUNCRTEXT("uUlL"); ECHOAFTER(')'); }
{digit}{digitpf}					{ TRUNCRTEXT("uUlL"); ECHOREST; }
{identifier}"("{digit}")"				{ TRUNCRTEXT(")"); ECHOAFTER('('); }
{identifier}"("{digit}{digitpf}")"	{ TRUNCRTEXT(")uUlL"); ECHOAFTER('('); }
{identifier}						{ ECHO; }
.									{ ECHO; }
}

%%