%{
/* needed headers for flex to work correclty */
#include "standard.h"
%}

%option stack
%option prefix="cc"
%option noyywrap
%option pointer
%option stack
%option 8bit
%option caseful
%option warn
%option never-interactive

%x CCOMMENT CPPCOMMENT

newline		\n
ccomment	"//"
cppcomment	"/*"
restofline	[^\n]*

%%

{ccomment}		{ BEGIN(CCOMMENT); }
{cppcomment}	{ BEGIN(CPPCOMMENT); }
.				{ ECHO; }

<CCOMMENT>{
{restofline}		{ /* ignore rest of line */ }
{newline}			{ ECHO; BEGIN(INITIAL); }
}

<CPPCOMMENT>{
[^*]*			/* eat anything that's not a '*' */
"*"+[^*/\n]*    /* eat up '*'s not followed by '/'s */
"*/"			{ BEGIN(INITIAL); }
}

%%