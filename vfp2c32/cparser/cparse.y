%{
#include "standard.h"

#ifdef _DEBUG

#define YYDEBUG 1
#undef stderr 
FILE *pTrace;
#define stderr pTrace

#endif

void
yyerror(char *s)
{
	if (nHandleError)
		add_error("%s!",s,gnLineNo);
	else
		nHandleError = 1;
}

%}

/* create an extra header file */
%defines

/* enable specific error messages */
%error-verbose

%union
{
	struct _PARSENODE *node;
	char *string;
	int number;
}

/* needed keywords */
%token CHAR DOUBLE FLOAT INT INT8 INT64 LONG SHORT SIGNED UNSIGNED VOID WCHAR
%token IDENTIFIER TYPENAME TYPEDEF
%token ENUM STRUCT UNION
%token ASSIGN COLON

/* bind the token's to members in the union */
%token <number> CONSTANT
%token <string> IDENTIFIER TYPENAME

%type <node> translation_unit declaration declaration_specifier
%type <node> init_declarator_list init_declarator_list_ex init_declarator
%type <node> type_specifier type_specifier_list
%type <node> struct_or_union_specifier struct_or_union identifier type_identifier
%type <node> any_id struct_declaration_list_ex struct_declaration_list
%type <node> struct_declaration struct_declarator_list struct_declarator 
%type <node> declarator direct_declarator pointer 
%type <node> enum_specifier enumerator_list enumerator_list_ex enumerator
%type <node> constant_expression

/*
%destructor { free_pnode($$); } declaration declaration_specifier init_declarator_list
%destructor { free_pnode($$); } init_declarator_list_ex init_declarator
%destructor { free_pnode($$); } type_specifier type_specifier_list struct_or_union_specifier
%destructor { free_pnode($$); } struct_or_union identifier type_identifier
%destructor { free_pnode($$); } any_id struct_declaration_list_ex struct_declaration_list
%destructor { free_pnode($$); } struct_declaration struct_declarator_list struct_declarator
%destructor { free_pnode($$); } declarator direct_declarator pointer
%destructor { free_pnode($$); } enum_specifier enumerator_list enumerator_list_ex enumerator
%destructor { free_pnode($$); } constant_expression
*/

%start goal
%%

goal
	: translation_unit
	;

translation_unit
	: declaration					{ add_parsedtype($1); }
	| translation_unit declaration	{ add_parsedtype($2); }
	;
	
declaration
	: declaration_specifier init_declarator_list ';' { $$ = create_pnode(T_DECLARATION,$1,$2,NULL); gnTypeFlag = 1; }
	| declaration_specifier ';'		{ $$ = create_pnode(T_DECLARATION,$1,NULL,NULL); gnTypeFlag = 1; }
	| declaration_specifier error ';'	{ gnTypeFlag = 1; }
	;

declaration_specifier
	: type_specifier			{ $$ = create_pnode(T_TYPESPECIFIER,$1,NULL,NULL); gnTypeFlag = 0;}
	;	

init_declarator_list
	: init_declarator_list_ex { $$ = $1->link1; $1->link1 = NULL; }
	;

init_declarator_list_ex
	: init_declarator	{ $$ = $1; $1->link1 = $1; }
	| init_declarator_list_ex ',' init_declarator { $3->link1 = $1->link1; $1->link1 = $3; $$ = $3; }
	;

init_declarator
	: declarator	{ $$ = create_pnode(T_DECLARATOR,NULL,$1,NULL); }
	;

type_specifier
	: VOID						{ $$ = create_pnode(T_VOID,NULL,NULL,NULL); }
	| CHAR						{ $$ = create_pnode(T_CHAR,NULL,NULL,NULL); }
	| WCHAR						{ $$ = create_pnode(T_WCHAR,NULL,NULL,NULL); }
	| INT8						{ $$ = create_pnode(T_INT8,NULL,NULL,NULL); }
	| SHORT						{ $$ = create_pnode(T_SHORT,NULL,NULL,NULL); }
	| INT						{ $$ = create_pnode(T_INT,NULL,NULL,NULL); }
	| INT64						{ $$ = create_pnode(T_INT64,NULL,NULL,NULL); }
	| LONG						{ $$ = create_pnode(T_LONG,NULL,NULL,NULL); }
	| UNSIGNED					{ $$ = create_pnode(T_UNSIGNED,NULL,NULL,NULL); }
	| SIGNED					{ $$ = create_pnode(T_SIGNED,NULL,NULL,NULL); }
	| FLOAT						{ $$ = create_pnode(T_FLOAT,NULL,NULL,NULL); }
	| DOUBLE					{ $$ = create_pnode(T_DOUBLE,NULL,NULL,NULL); }
	| type_identifier			{ $$ = $1; }
	| enum_specifier			{ $$ = $1; }
	| struct_or_union_specifier	{ $$ = $1; }
	;

identifier 
	: IDENTIFIER	{ $$ = create_string(T_IDENTIFIER,$1); }
	;

type_identifier
	: TYPENAME		{ $$ = create_string(T_TYPENAME,$1); }
	;

any_id
	: identifier		{ $$ = $1; }
	| type_identifier	{ $$ = $1; }
	;

struct_or_union_specifier
	: struct_or_union any_id '{' struct_declaration_list '}' { $$ = create_pnode(T_STRUCT,$1,$2,$4); }
	| struct_or_union '{' struct_declaration_list '}'		 { $$ = create_pnode(T_STRUCT,$1,NULL,$3); }
	| struct_or_union any_id								 { $$ = create_pnode(T_STRUCT,$1,$2,NULL); }
	;

struct_or_union
	: STRUCT	{$$ = create_pnode(T_STRUCT,NULL,NULL,NULL); }
	| UNION		{$$ = create_pnode(T_UNION,NULL,NULL,NULL); }
	;

struct_declaration_list
	: struct_declaration_list_ex { $$ = $1->link1; $1->link1 = NULL; }
	;

struct_declaration_list_ex
	: struct_declaration							{ $1->link1 = $1; $$ = $1; }
	| struct_declaration_list_ex struct_declaration	{ $2->link1 = $1->link1; $1->link1 = $2; $$ = $2; }
	;
	
struct_declaration
	: type_specifier_list struct_declarator_list ';'	{ $$ = create_pnode(T_STRUCTDECLARATION,NULL,$1,$2->link1); $2->link1 = NULL; }
	| type_specifier_list ';'	{ /* must be an anonymous struct or union */
		if ($1->link1->nType != T_STRUCT)
			add_error("%s!","Missing declarator",$1->nLineNo);
		$$ = create_pnode(T_STRUCTDECLARATION,NULL,$1,NULL); }
	| identifier struct_declarator_list ';'	{ 
		add_error("Unknown type specifier '%s'!",$1->un.String,$1->nLineNo);
		free_pnode($1); $2->link1 = NULL; free_pnode($2);
		$$ = create_pnode(T_STRUCTDECLARATION,NULL,NULL,NULL); }
	| type_specifier_list { nHandleError = 0; }
	error ';' {
		add_error("%s!","Invalid declarator",$1->nLineNo);
		free_pnode($1); 
		$$ = create_pnode(T_STRUCTDECLARATION,NULL,NULL,NULL); }
	| type_specifier_list struct_declarator_list { nHandleError = 0; }
	error {
		add_error("%s!","Missing semicolon",$2->nLineNo);
		$$ = create_pnode(T_STRUCTDECLARATION,NULL,$1,$2->link1); $2->link1 = NULL;	}
	;
	
struct_declarator_list
	: struct_declarator								{ $1->link1 = $1; $$ = $1; }
	| struct_declarator_list ',' struct_declarator	{ $3->link1 = $1->link1; $1->link1 = $3; $$ = $3; }
	;
	
struct_declarator
	: declarator							{ $$ = create_pnode(T_STRUCTDECLARATOR,NULL,$1,NULL); }
	| declarator ':' constant_expression	{ $$ = create_pnode(T_STRUCTDECLARATOR,NULL,$1,$3); }
	| ':' constant_expression				{ $$ = create_pnode(T_STRUCTDECLARATOR,NULL,NULL,$2); }
	;

type_specifier_list
	: type_specifier					 { $$ = create_pnode(T_TYPESPECIFIER,$1,NULL,NULL); }
	| type_specifier_list type_specifier { $$ = create_pnode(T_TYPESPECIFIER,$2,$1,NULL); }

declarator
	: direct_declarator			{ $$ = $1; }
	| pointer direct_declarator	{ $$ = $1->link1; $1->link1 = $2; }
	;

direct_declarator
	: identifier									{ $$ = $1; }
	| '(' declarator ')'							{ $$ = $2; }
	| direct_declarator '[' constant_expression ']'	{ $$ = create_pnode(T_SUBSCRIPT,$1,$3,NULL); }
	| direct_declarator '[' identifier ']'			{
		add_error("Preprocessor token '%s' detected.\n Replace it with it's actual value!",$3->un.String,$3->nLineNo);
		free_pnode($3); $$ = $1; }
	| direct_declarator '[' ']'						{ $$ = create_pnode(T_SUBSCRIPT,$1,NULL,NULL); }
	;

enum_specifier
	: ENUM '{' enumerator_list '}'					{ $$ = create_pnode(T_ENUM,NULL,$3,NULL); }
	| ENUM any_id '{' enumerator_list '}'			{ $$ = create_pnode(T_ENUM,$2,$4,NULL);	}
	| ENUM any_id									{ $$ = create_pnode(T_ENUM,$2,NULL,NULL); }
	;

enumerator_list
	: enumerator_list_ex	{ $$ = $1->link1; $1->link1 = NULL; }
	;

enumerator_list_ex
	: enumerator							{ $1->link1 = $1; $$ = $1; }
	| enumerator_list_ex ',' enumerator		{ $3->link1 = $1->link1; $1->link1 = $3; $$ = $3; }
	| enumerator_list_ex ','				{ $$ = $1; }
	;

enumerator
	: identifier							{ $$ = create_pnode(T_ENUMERATOR,NULL,$1,NULL); }
	| identifier '=' constant_expression	{ $$ = create_pnode(T_ENUMERATOR,NULL,$1,$3); }
	;

constant_expression
	: CONSTANT		{ $$ = create_const($1); }
	;

pointer
	: '*'			{ $$ = create_pnode(T_POINTER,NULL,NULL,NULL); $$->link1 = $$; }
	| '*' pointer	{ $$ = create_pnode(T_POINTER,$2->link1,NULL,NULL);	$2->link1 = $$; $$ = $2;}
	;

%%
