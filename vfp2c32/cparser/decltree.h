#ifndef DECLTREE_INCLUDED
#define DECLTREE_INCLUDED

typedef struct _TYPEDEF
{
	char *pTypename;
	struct _TYPEDEF *next;
} STYPEDEF, *LPTYPEDEF;

typedef enum _NODETYPE {
T_VOID=1, T_CHAR, T_WCHAR, T_INT8, T_SHORT, T_INT, T_INT64, T_LONG, T_UNSIGNED,
T_SIGNED, T_FLOAT, T_DOUBLE, T_STRUCT, T_UNION, T_ENUM, T_TYPENAME,
T_IDENTIFIER, T_STRUCTDECLARATION, T_STRUCTDECLARATOR, T_TYPESPECIFIER, T_SUBSCRIPT,
T_POINTER, T_CONSTANT, T_DECLARATION, T_DECLARATOR, T_ENUMERATOR
} NODETYPE;

typedef struct _PARSENODE {
	NODETYPE nType;
	union {
		struct _PARSENODE *link[3];
		char *String;
		int Constant;
	} un;
	int nLineNo;
} PARSENODE, *LPPARSENODE;

// defines for easier access to _PARSENODE union members
#define link1	un.link[0]
#define link2	un.link[1]
#define link3	un.link[2]

typedef struct _PARSEDTYPES {
	struct _PARSENODE *node;
	struct _PARSEDTYPES *next;
} PARSEDTYPES, *LPPARSEDTYPES;

typedef struct _PARSEERRORS {
	char *pErrorMes;
	int nLineNo;
	struct _PARSEERRORS *next;
} PARSEERRORS, *LPPARSEERRORS;

// function prototypes
LPTYPEDEF _stdcall add_knowntype(char *pTypename);
void _stdcall remove_knowntype(char *pTypename);
LPTYPEDEF _stdcall find_knowntype(char *pTypename);
void _stdcall clear_knowntypes();

LPPARSENODE _stdcall create_pnode(NODETYPE nType, LPPARSENODE p1, LPPARSENODE p2, LPPARSENODE p3);
LPPARSENODE _stdcall create_string(NODETYPE nType, char *pString);
LPPARSENODE _stdcall create_const(int nConst);

LPPARSEDTYPES _stdcall get_parsedtypehead();
void _stdcall add_parsedtype(LPPARSENODE pN);
void _stdcall clear_parsedtypes();
void _stdcall free_pnode(LPPARSENODE pN);

LPPARSEERRORS _stdcall get_parseerrorhead();
void _stdcall add_error(char *pMask, char *pValue, int nLineNo);
void _stdcall clear_errors();
int _stdcall get_errorcount();

extern int nHandleError;

#endif