%{
/* needed headers for flex to work correclty */
#include "standard.h"
#include "cparse.tab.h"

static int gnBraceCnt = 0;
static int gnPreproc = 0;
static int gnPreFlag = 0;
int gnTypeFlag = 1;
int gnLineNo;

static int handle_identifier()
{
	yylval.string = yytext;
	if (gnTypeFlag)
	{
	if (find_knowntype(yytext))
		return TYPENAME;	
	else
		return IDENTIFIER;
	}
	else
		return IDENTIFIER;
}

%}

%option stack
%option noyywrap
%option pointer
%option stack
%option 8bit
%option caseful
%option warn
%option never-interactive

%x SCTYPE PREPROC

ws			[ \t\f\v\a]+
anyws		[ \t\n\r\f\v\a]*
identifier	[a-zA-Z_][a-zA-Z_0-9]*
preline		^#
digit		[0-9]
hexdigit	0[xX][a-fA-F0-9]
octdigit	0[0-7]
newline		\n\r|\n|\r
restofline	[^\n]*
backslash	[\\]
aggrtype    struct|union|enum
%%

"typedef"?{anyws}{aggrtype}{anyws}{identifier}?{anyws}"{" { yyless(0); BEGIN(SCTYPE); }

{preline}		{ gnPreproc = INITIAL; BEGIN(PREPROC); }
{ws}			/* eat up whitespace */
{newline}		{ ++gnLineNo; }
.				/* eat up anything else */

<SCTYPE>{
void			{ return VOID; }
char			{ return CHAR; }
wchar_t			{ return WCHAR; }
__wchar_t		{ return WCHAR; }
__int8			{ return INT8; /* handled as a 1 byte number */ }
short			{ return SHORT; }
__int16			{ return SHORT; /* identical to short */ }
int				{ return INT; }
__int32			{ return INT; /* identical to int */ }
long			{ return LONG; }
float			{ return FLOAT; }
double			{ return DOUBLE; }
signed			{ return SIGNED; }
unsigned		{ return UNSIGNED; }
__int64			{ return INT64; }
struct			{ return STRUCT; }
union			{ return UNION; }
enum			{ return ENUM; }
typedef			/*ignore in here*/
const			/*ignore type-qualifiers*/
volatile		/*ignore type-qualifiers*/
auto			/*ignore storage-class qualifiers*/
extern			/*ignore storage-class qualifiers*/
register		/*ignore storage-class qualifiers*/
static			/*ignore storage-class qualifiers*/
FAR				/*ignore FAR pointer's*/
NEAR			/*ignore NEAR pointer's*/
"="				{ return '='; }
"("				{ return '('; }
")"				{ return ')'; }
"{"				{ gnBraceCnt++; return '{'; }
"}"				{ gnBraceCnt--; return '}'; }
"["				{ return '['; }
"]"				{ return ']'; }
"*"				{ return '*'; }
";"				{ 	if (gnBraceCnt == 0)
						BEGIN(INITIAL);
					return ';';
				}
","			{ return ','; }
":"			{ return ':'; }

{hexdigit}+			{ sscanf(yytext+2,"%lx",&yylval.number);
						return CONSTANT; }
{octdigit}+			{ sscanf(yytext+1,"%lo",&yylval.number);
						return CONSTANT; }
{octdigit}+[uU][lL]	{ sscanf(yytext+1,"%lo",&yylval.number);
						return CONSTANT; }
{digit}+			{ sscanf(yytext,"%ld",&yylval.number);
						return CONSTANT; }

{identifier}		{ return handle_identifier(); }
{ws}				/* ignore whitespace */
{preline}			{ gnPreproc = SCTYPE; BEGIN(PREPROC); gnPreFlag = 0; }
{newline}			{ ++gnLineNo; }
.					{ /* ignore anything else */ }
}

<PREPROC>{
{backslash}${newline}	{ ++gnLineNo; gnPreFlag = 1; }
{newline}				{ ++gnLineNo; if (gnPreFlag-- == 0) BEGIN(gnPreproc); }
.						{ /* ignore anything else */ }
}

%%