extern int yylex();	/* lexical analyzer generated from clex.l */
extern char	*yytext;	/* last token, defined in clex.l  */
extern void yyrestart(FILE *input_file);
extern FILE *yyin, *yyout;
extern int gnLineNo;
extern int gnTypeFlag;