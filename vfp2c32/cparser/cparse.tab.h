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




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 31 "cparse.y"
typedef union YYSTYPE {
	struct _PARSENODE *node;
	char *string;
	int number;
} YYSTYPE;
/* Line 1248 of yacc.c.  */
#line 88 "cparse.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



