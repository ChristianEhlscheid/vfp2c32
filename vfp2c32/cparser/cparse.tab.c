/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     CHAR = 258,
     DOUBLE = 259,
     FLOAT = 260,
     INT = 261,
     INT8 = 262,
     INT64 = 263,
     LONG = 264,
     SHORT = 265,
     SIGNED = 266,
     UNSIGNED = 267,
     VOID = 268,
     WCHAR = 269,
     IDENTIFIER = 270,
     TYPENAME = 271,
     TYPEDEF = 272,
     ENUM = 273,
     STRUCT = 274,
     UNION = 275,
     ASSIGN = 276,
     COLON = 277,
     CONSTANT = 278
   };
#endif
#define CHAR 258
#define DOUBLE 259
#define FLOAT 260
#define INT 261
#define INT8 262
#define INT64 263
#define LONG 264
#define SHORT 265
#define SIGNED 266
#define UNSIGNED 267
#define VOID 268
#define WCHAR 269
#define IDENTIFIER 270
#define TYPENAME 271
#define TYPEDEF 272
#define ENUM 273
#define STRUCT 274
#define UNION 275
#define ASSIGN 276
#define COLON 277
#define CONSTANT 278




/* Copy the first part of user declarations.  */
#line 1 "cparse.y"

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



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 31 "cparse.y"
typedef union YYSTYPE {
	struct _PARSENODE *node;
	char *string;
	int number;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 150 "cparse.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 162 "cparse.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  31
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   134

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  35
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  30
/* YYNRULES -- Number of rules. */
#define YYNRULES  72
/* YYNRULES -- Number of states. */
#define YYNSTATES  103

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   278

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      29,    30,    34,     2,    25,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    28,    24,
       2,    33,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    31,     2,    32,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    26,     2,    27,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    14,    17,    21,    23,
      25,    27,    31,    33,    35,    37,    39,    41,    43,    45,
      47,    49,    51,    53,    55,    57,    59,    61,    63,    65,
      67,    69,    71,    77,    82,    85,    87,    89,    91,    93,
      96,   100,   103,   107,   108,   113,   114,   119,   121,   125,
     127,   131,   134,   136,   139,   141,   144,   146,   150,   155,
     160,   164,   169,   175,   178,   180,   182,   186,   189,   191,
     195,   197,   199
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      36,     0,    -1,    37,    -1,    38,    -1,    37,    38,    -1,
      39,    40,    24,    -1,    39,    24,    -1,    39,     1,    24,
      -1,    43,    -1,    41,    -1,    42,    -1,    41,    25,    42,
      -1,    57,    -1,    13,    -1,     3,    -1,    14,    -1,     7,
      -1,    10,    -1,     6,    -1,     8,    -1,     9,    -1,    12,
      -1,    11,    -1,     5,    -1,     4,    -1,    45,    -1,    59,
      -1,    47,    -1,    15,    -1,    16,    -1,    44,    -1,    45,
      -1,    48,    46,    26,    49,    27,    -1,    48,    26,    49,
      27,    -1,    48,    46,    -1,    19,    -1,    20,    -1,    50,
      -1,    51,    -1,    50,    51,    -1,    56,    54,    24,    -1,
      56,    24,    -1,    44,    54,    24,    -1,    -1,    56,    52,
       1,    24,    -1,    -1,    56,    54,    53,     1,    -1,    55,
      -1,    54,    25,    55,    -1,    57,    -1,    57,    28,    63,
      -1,    28,    63,    -1,    43,    -1,    56,    43,    -1,    58,
      -1,    64,    58,    -1,    44,    -1,    29,    57,    30,    -1,
      58,    31,    63,    32,    -1,    58,    31,    44,    32,    -1,
      58,    31,    32,    -1,    18,    26,    60,    27,    -1,    18,
      46,    26,    60,    27,    -1,    18,    46,    -1,    61,    -1,
      62,    -1,    61,    25,    62,    -1,    61,    25,    -1,    44,
      -1,    44,    33,    63,    -1,    23,    -1,    34,    -1,    34,
      64,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned char yyrline[] =
{
       0,    73,    73,    77,    78,    82,    83,    84,    88,    92,
      96,    97,   101,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   123,   127,
     131,   132,   136,   137,   138,   142,   143,   147,   151,   152,
     156,   157,   161,   165,   165,   170,   170,   177,   178,   182,
     183,   184,   188,   189,   192,   193,   197,   198,   199,   200,
     203,   207,   208,   209,   213,   217,   218,   219,   223,   224,
     228,   232,   233
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CHAR", "DOUBLE", "FLOAT", "INT", "INT8", 
  "INT64", "LONG", "SHORT", "SIGNED", "UNSIGNED", "VOID", "WCHAR", 
  "IDENTIFIER", "TYPENAME", "TYPEDEF", "ENUM", "STRUCT", "UNION", 
  "ASSIGN", "COLON", "CONSTANT", "';'", "','", "'{'", "'}'", "':'", "'('", 
  "')'", "'['", "']'", "'='", "'*'", "$accept", "goal", 
  "translation_unit", "declaration", "declaration_specifier", 
  "init_declarator_list", "init_declarator_list_ex", "init_declarator", 
  "type_specifier", "identifier", "type_identifier", "any_id", 
  "struct_or_union_specifier", "struct_or_union", 
  "struct_declaration_list", "struct_declaration_list_ex", 
  "struct_declaration", "@1", "@2", "struct_declarator_list", 
  "struct_declarator", "type_specifier_list", "declarator", 
  "direct_declarator", "enum_specifier", "enumerator_list", 
  "enumerator_list_ex", "enumerator", "constant_expression", "pointer", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,    59,    44,   123,   125,    58,    40,
      41,    91,    93,    61,    42
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    35,    36,    37,    37,    38,    38,    38,    39,    40,
      41,    41,    42,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,    43,    43,    44,    45,
      46,    46,    47,    47,    47,    48,    48,    49,    50,    50,
      51,    51,    51,    52,    51,    53,    51,    54,    54,    55,
      55,    55,    56,    56,    57,    57,    58,    58,    58,    58,
      58,    59,    59,    59,    60,    61,    61,    61,    62,    62,
      63,    64,    64
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     2,     3,     2,     3,     1,     1,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     5,     4,     2,     1,     1,     1,     1,     2,
       3,     2,     3,     0,     4,     0,     4,     1,     3,     1,
       3,     2,     1,     2,     1,     2,     1,     3,     4,     4,
       3,     4,     5,     2,     1,     1,     3,     2,     1,     3,
       1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,    14,    24,    23,    18,    16,    19,    20,    17,    22,
      21,    13,    15,    29,     0,    35,    36,     0,     2,     3,
       0,     8,    25,    27,     0,    26,    28,     0,    30,    31,
      63,     1,     4,     0,     6,     0,    71,     0,     9,    10,
      56,    12,    54,     0,     0,    34,    68,     0,    64,    65,
       0,     7,     0,    72,     5,     0,     0,    55,    52,     0,
       0,    37,    38,    43,     0,     0,    61,    67,     0,    57,
      11,    70,    60,     0,     0,     0,     0,    47,    49,    33,
      39,    41,    53,     0,    45,     0,    69,    66,    62,    59,
      58,    51,    42,     0,     0,     0,    40,     0,    32,    48,
      50,    44,    46
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    17,    18,    19,    20,    37,    38,    39,    58,    40,
      22,    30,    23,    24,    60,    61,    62,    83,    97,    76,
      77,    63,    78,    42,    25,    47,    48,    49,    74,    43
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -63
static const yysigned_char yypact[] =
{
     111,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,    18,   -63,   -63,     6,   111,   -63,
       1,   -63,   -63,   -63,    26,   -63,   -63,    -5,   -63,   -63,
     -14,   -63,   -63,     5,   -63,    -7,   -17,    14,    -1,   -63,
     -63,   -63,     9,    -6,    93,    27,    23,    28,    32,   -63,
      -5,   -63,    29,   -63,   -63,    -7,    13,     9,   -63,    -8,
      31,    93,   -63,    61,    93,    37,   -63,    -5,    34,   -63,
     -63,   -63,   -63,    30,    46,    37,    22,   -63,    54,   -63,
     -63,   -63,   -63,    82,    25,    57,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,    -8,    37,    62,   -63,    86,   -63,   -63,
     -63,   -63,   -63
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -63,   -63,   -63,    70,   -63,   -63,   -63,    36,     0,   -13,
      -9,    68,   -63,   -63,    64,   -63,    33,   -63,   -63,    47,
      39,   -63,   -16,    50,   -63,    76,   -63,    66,   -62,    98
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      21,    28,    33,    86,    41,    29,    31,    26,    26,    26,
      26,    28,    50,    91,    46,    29,    26,    36,    21,    52,
      75,    35,    35,    35,    55,    34,    36,    36,    26,    51,
      35,    59,   100,    26,    13,    36,    71,    46,    54,    41,
      56,    26,    13,    73,    27,    72,    92,    93,    59,    96,
      93,    59,    44,    64,    46,    66,    65,    67,    79,    69,
      71,    88,    89,    82,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    26,    13,    90,    14,
      15,    16,    94,    95,    98,    81,   101,   102,    32,    75,
      35,    70,    45,    57,    80,    36,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    26,    13,
      84,    14,    15,    16,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    68,    13,    85,    14,
      15,    16,    99,    87,    53
};

static const unsigned char yycheck[] =
{
       0,    14,     1,    65,    20,    14,     0,    15,    15,    15,
      15,    24,    26,    75,    27,    24,    15,    34,    18,    35,
      28,    29,    29,    29,    25,    24,    34,    34,    15,    24,
      29,    44,    94,    15,    16,    34,    23,    50,    24,    55,
      31,    15,    16,    56,    26,    32,    24,    25,    61,    24,
      25,    64,    26,    26,    67,    27,    33,    25,    27,    30,
      23,    27,    32,    63,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    32,    18,
      19,    20,    28,     1,    27,    24,    24,     1,    18,    28,
      29,    55,    24,    43,    61,    34,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      63,    18,    19,    20,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    50,    16,    64,    18,
      19,    20,    93,    67,    36
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    16,    18,    19,    20,    36,    37,    38,
      39,    43,    45,    47,    48,    59,    15,    26,    44,    45,
      46,     0,    38,     1,    24,    29,    34,    40,    41,    42,
      44,    57,    58,    64,    26,    46,    44,    60,    61,    62,
      26,    24,    57,    64,    24,    25,    31,    58,    43,    44,
      49,    50,    51,    56,    26,    33,    27,    25,    60,    30,
      42,    23,    32,    44,    63,    28,    54,    55,    57,    27,
      51,    24,    43,    52,    54,    49,    63,    62,    27,    32,
      32,    63,    24,    25,    28,     1,    24,    53,    27,    55,
      63,    24,     1
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 77 "cparse.y"
    { add_parsedtype(yyvsp[0].node); ;}
    break;

  case 4:
#line 78 "cparse.y"
    { add_parsedtype(yyvsp[0].node); ;}
    break;

  case 5:
#line 82 "cparse.y"
    { yyval.node = create_pnode(T_DECLARATION,yyvsp[-2].node,yyvsp[-1].node,NULL); gnTypeFlag = 1; ;}
    break;

  case 6:
#line 83 "cparse.y"
    { yyval.node = create_pnode(T_DECLARATION,yyvsp[-1].node,NULL,NULL); gnTypeFlag = 1; ;}
    break;

  case 7:
#line 84 "cparse.y"
    { gnTypeFlag = 1; ;}
    break;

  case 8:
#line 88 "cparse.y"
    { yyval.node = create_pnode(T_TYPESPECIFIER,yyvsp[0].node,NULL,NULL); gnTypeFlag = 0;;}
    break;

  case 9:
#line 92 "cparse.y"
    { yyval.node = yyvsp[0].node->link1; yyvsp[0].node->link1 = NULL; ;}
    break;

  case 10:
#line 96 "cparse.y"
    { yyval.node = yyvsp[0].node; yyvsp[0].node->link1 = yyvsp[0].node; ;}
    break;

  case 11:
#line 97 "cparse.y"
    { yyvsp[0].node->link1 = yyvsp[-2].node->link1; yyvsp[-2].node->link1 = yyvsp[0].node; yyval.node = yyvsp[0].node; ;}
    break;

  case 12:
#line 101 "cparse.y"
    { yyval.node = create_pnode(T_DECLARATOR,NULL,yyvsp[0].node,NULL); ;}
    break;

  case 13:
#line 105 "cparse.y"
    { yyval.node = create_pnode(T_VOID,NULL,NULL,NULL); ;}
    break;

  case 14:
#line 106 "cparse.y"
    { yyval.node = create_pnode(T_CHAR,NULL,NULL,NULL); ;}
    break;

  case 15:
#line 107 "cparse.y"
    { yyval.node = create_pnode(T_WCHAR,NULL,NULL,NULL); ;}
    break;

  case 16:
#line 108 "cparse.y"
    { yyval.node = create_pnode(T_INT8,NULL,NULL,NULL); ;}
    break;

  case 17:
#line 109 "cparse.y"
    { yyval.node = create_pnode(T_SHORT,NULL,NULL,NULL); ;}
    break;

  case 18:
#line 110 "cparse.y"
    { yyval.node = create_pnode(T_INT,NULL,NULL,NULL); ;}
    break;

  case 19:
#line 111 "cparse.y"
    { yyval.node = create_pnode(T_INT64,NULL,NULL,NULL); ;}
    break;

  case 20:
#line 112 "cparse.y"
    { yyval.node = create_pnode(T_LONG,NULL,NULL,NULL); ;}
    break;

  case 21:
#line 113 "cparse.y"
    { yyval.node = create_pnode(T_UNSIGNED,NULL,NULL,NULL); ;}
    break;

  case 22:
#line 114 "cparse.y"
    { yyval.node = create_pnode(T_SIGNED,NULL,NULL,NULL); ;}
    break;

  case 23:
#line 115 "cparse.y"
    { yyval.node = create_pnode(T_FLOAT,NULL,NULL,NULL); ;}
    break;

  case 24:
#line 116 "cparse.y"
    { yyval.node = create_pnode(T_DOUBLE,NULL,NULL,NULL); ;}
    break;

  case 25:
#line 117 "cparse.y"
    { yyval.node = yyvsp[0].node; ;}
    break;

  case 26:
#line 118 "cparse.y"
    { yyval.node = yyvsp[0].node; ;}
    break;

  case 27:
#line 119 "cparse.y"
    { yyval.node = yyvsp[0].node; ;}
    break;

  case 28:
#line 123 "cparse.y"
    { yyval.node = create_string(T_IDENTIFIER,yyvsp[0].string); ;}
    break;

  case 29:
#line 127 "cparse.y"
    { yyval.node = create_string(T_TYPENAME,yyvsp[0].string); ;}
    break;

  case 30:
#line 131 "cparse.y"
    { yyval.node = yyvsp[0].node; ;}
    break;

  case 31:
#line 132 "cparse.y"
    { yyval.node = yyvsp[0].node; ;}
    break;

  case 32:
#line 136 "cparse.y"
    { yyval.node = create_pnode(T_STRUCT,yyvsp[-4].node,yyvsp[-3].node,yyvsp[-1].node); ;}
    break;

  case 33:
#line 137 "cparse.y"
    { yyval.node = create_pnode(T_STRUCT,yyvsp[-3].node,NULL,yyvsp[-1].node); ;}
    break;

  case 34:
#line 138 "cparse.y"
    { yyval.node = create_pnode(T_STRUCT,yyvsp[-1].node,yyvsp[0].node,NULL); ;}
    break;

  case 35:
#line 142 "cparse.y"
    {yyval.node = create_pnode(T_STRUCT,NULL,NULL,NULL); ;}
    break;

  case 36:
#line 143 "cparse.y"
    {yyval.node = create_pnode(T_UNION,NULL,NULL,NULL); ;}
    break;

  case 37:
#line 147 "cparse.y"
    { yyval.node = yyvsp[0].node->link1; yyvsp[0].node->link1 = NULL; ;}
    break;

  case 38:
#line 151 "cparse.y"
    { yyvsp[0].node->link1 = yyvsp[0].node; yyval.node = yyvsp[0].node; ;}
    break;

  case 39:
#line 152 "cparse.y"
    { yyvsp[0].node->link1 = yyvsp[-1].node->link1; yyvsp[-1].node->link1 = yyvsp[0].node; yyval.node = yyvsp[0].node; ;}
    break;

  case 40:
#line 156 "cparse.y"
    { yyval.node = create_pnode(T_STRUCTDECLARATION,NULL,yyvsp[-2].node,yyvsp[-1].node->link1); yyvsp[-1].node->link1 = NULL; ;}
    break;

  case 41:
#line 157 "cparse.y"
    { /* must be an anonymous struct or union */
		if (yyvsp[-1].node->link1->nType != T_STRUCT)
			add_error("%s!","Missing declarator",yyvsp[-1].node->nLineNo);
		yyval.node = create_pnode(T_STRUCTDECLARATION,NULL,yyvsp[-1].node,NULL); ;}
    break;

  case 42:
#line 161 "cparse.y"
    { 
		add_error("Unknown type specifier '%s'!",yyvsp[-2].node->un.String,yyvsp[-2].node->nLineNo);
		free_pnode(yyvsp[-2].node); yyvsp[-1].node->link1 = NULL; free_pnode(yyvsp[-1].node);
		yyval.node = create_pnode(T_STRUCTDECLARATION,NULL,NULL,NULL); ;}
    break;

  case 43:
#line 165 "cparse.y"
    { nHandleError = 0; ;}
    break;

  case 44:
#line 166 "cparse.y"
    {
		add_error("%s!","Invalid declarator",yyvsp[-3].node->nLineNo);
		free_pnode(yyvsp[-3].node); 
		yyval.node = create_pnode(T_STRUCTDECLARATION,NULL,NULL,NULL); ;}
    break;

  case 45:
#line 170 "cparse.y"
    { nHandleError = 0; ;}
    break;

  case 46:
#line 171 "cparse.y"
    {
		add_error("%s!","Missing semicolon",yyvsp[-2].node->nLineNo);
		yyval.node = create_pnode(T_STRUCTDECLARATION,NULL,yyvsp[-3].node,yyvsp[-2].node->link1); yyvsp[-2].node->link1 = NULL;	;}
    break;

  case 47:
#line 177 "cparse.y"
    { yyvsp[0].node->link1 = yyvsp[0].node; yyval.node = yyvsp[0].node; ;}
    break;

  case 48:
#line 178 "cparse.y"
    { yyvsp[0].node->link1 = yyvsp[-2].node->link1; yyvsp[-2].node->link1 = yyvsp[0].node; yyval.node = yyvsp[0].node; ;}
    break;

  case 49:
#line 182 "cparse.y"
    { yyval.node = create_pnode(T_STRUCTDECLARATOR,NULL,yyvsp[0].node,NULL); ;}
    break;

  case 50:
#line 183 "cparse.y"
    { yyval.node = create_pnode(T_STRUCTDECLARATOR,NULL,yyvsp[-2].node,yyvsp[0].node); ;}
    break;

  case 51:
#line 184 "cparse.y"
    { yyval.node = create_pnode(T_STRUCTDECLARATOR,NULL,NULL,yyvsp[0].node); ;}
    break;

  case 52:
#line 188 "cparse.y"
    { yyval.node = create_pnode(T_TYPESPECIFIER,yyvsp[0].node,NULL,NULL); ;}
    break;

  case 53:
#line 189 "cparse.y"
    { yyval.node = create_pnode(T_TYPESPECIFIER,yyvsp[0].node,yyvsp[-1].node,NULL); ;}
    break;

  case 54:
#line 192 "cparse.y"
    { yyval.node = yyvsp[0].node; ;}
    break;

  case 55:
#line 193 "cparse.y"
    { yyval.node = yyvsp[-1].node->link1; yyvsp[-1].node->link1 = yyvsp[0].node; ;}
    break;

  case 56:
#line 197 "cparse.y"
    { yyval.node = yyvsp[0].node; ;}
    break;

  case 57:
#line 198 "cparse.y"
    { yyval.node = yyvsp[-1].node; ;}
    break;

  case 58:
#line 199 "cparse.y"
    { yyval.node = create_pnode(T_SUBSCRIPT,yyvsp[-3].node,yyvsp[-1].node,NULL); ;}
    break;

  case 59:
#line 200 "cparse.y"
    {
		add_error("Preprocessor token '%s' detected.\n Replace it with it's actual value!",yyvsp[-1].node->un.String,yyvsp[-1].node->nLineNo);
		free_pnode(yyvsp[-1].node); yyval.node = yyvsp[-3].node; ;}
    break;

  case 60:
#line 203 "cparse.y"
    { yyval.node = create_pnode(T_SUBSCRIPT,yyvsp[-2].node,NULL,NULL); ;}
    break;

  case 61:
#line 207 "cparse.y"
    { yyval.node = create_pnode(T_ENUM,NULL,yyvsp[-1].node,NULL); ;}
    break;

  case 62:
#line 208 "cparse.y"
    { yyval.node = create_pnode(T_ENUM,yyvsp[-3].node,yyvsp[-1].node,NULL);	;}
    break;

  case 63:
#line 209 "cparse.y"
    { yyval.node = create_pnode(T_ENUM,yyvsp[0].node,NULL,NULL); ;}
    break;

  case 64:
#line 213 "cparse.y"
    { yyval.node = yyvsp[0].node->link1; yyvsp[0].node->link1 = NULL; ;}
    break;

  case 65:
#line 217 "cparse.y"
    { yyvsp[0].node->link1 = yyvsp[0].node; yyval.node = yyvsp[0].node; ;}
    break;

  case 66:
#line 218 "cparse.y"
    { yyvsp[0].node->link1 = yyvsp[-2].node->link1; yyvsp[-2].node->link1 = yyvsp[0].node; yyval.node = yyvsp[0].node; ;}
    break;

  case 67:
#line 219 "cparse.y"
    { yyval.node = yyvsp[-1].node; ;}
    break;

  case 68:
#line 223 "cparse.y"
    { yyval.node = create_pnode(T_ENUMERATOR,NULL,yyvsp[0].node,NULL); ;}
    break;

  case 69:
#line 224 "cparse.y"
    { yyval.node = create_pnode(T_ENUMERATOR,NULL,yyvsp[-2].node,yyvsp[0].node); ;}
    break;

  case 70:
#line 228 "cparse.y"
    { yyval.node = create_const(yyvsp[0].number); ;}
    break;

  case 71:
#line 232 "cparse.y"
    { yyval.node = create_pnode(T_POINTER,NULL,NULL,NULL); yyval.node->link1 = yyval.node; ;}
    break;

  case 72:
#line 233 "cparse.y"
    { yyval.node = create_pnode(T_POINTER,yyvsp[0].node->link1,NULL,NULL);	yyvsp[0].node->link1 = yyval.node; yyval.node = yyvsp[0].node;;}
    break;


    }

/* Line 991 of yacc.c.  */
#line 1510 "cparse.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 236 "cparse.y"


