/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     K_FUNCTION = 1,
     K_NEW = 2,
     K_DELETE = 3,
     K_VAR = 4,
     K_DO = 10,
     K_WHILE = 11,
     K_FOR = 12,
     K_IF = 13,
     K_ELSE = 14,
     K_SWITCH = 15,
     K_CASE = 16,
     K_DEFAULT = 17,
     K_BREAK = 18,
     K_CONTINUE = 19,
     K_RETURN = 20,
     K_UNKNOWN = 21,
     K_COMMENT = 22,
     T_FLOAT = 48,
     T_IDENTIFIER = 49,
     T_STRING = 50,
     T_INTEGER = 51,
     O_RSHIFTEQ = 65,
     O_RSHIFTFILLEQ = 66,
     O_LSHIFTEQ = 67,
     O_ADDEQ = 68,
     O_SUBEQ = 69,
     O_MULEQ = 70,
     O_DIVEQ = 71,
     O_MODEQ = 72,
     O_ANDEQ = 73,
     O_XOREQ = 74,
     O_OREQ = 75,
     O_RSHIFT = 76,
     O_RSHIFTFILL = 77,
     O_LSHIFT = 78,
     O_INC = 79,
     O_DEC = 80,
     O_LAND = 81,
     O_LOR = 82,
     O_LE = 83,
     O_GE = 84,
     O_EQ = 85,
     O_NE = 86,
     E_ERROR = 191,
     C_EOF = 255
   };
#endif
/* Tokens.  */
#define K_FUNCTION 1
#define K_NEW 2
#define K_DELETE 3
#define K_VAR 4
#define K_DO 10
#define K_WHILE 11
#define K_FOR 12
#define K_IF 13
#define K_ELSE 14
#define K_SWITCH 15
#define K_CASE 16
#define K_DEFAULT 17
#define K_BREAK 18
#define K_CONTINUE 19
#define K_RETURN 20
#define K_UNKNOWN 21
#define K_COMMENT 22
#define T_FLOAT 48
#define T_IDENTIFIER 49
#define T_STRING 50
#define T_INTEGER 51
#define O_RSHIFTEQ 65
#define O_RSHIFTFILLEQ 66
#define O_LSHIFTEQ 67
#define O_ADDEQ 68
#define O_SUBEQ 69
#define O_MULEQ 70
#define O_DIVEQ 71
#define O_MODEQ 72
#define O_ANDEQ 73
#define O_XOREQ 74
#define O_OREQ 75
#define O_RSHIFT 76
#define O_RSHIFTFILL 77
#define O_LSHIFT 78
#define O_INC 79
#define O_DEC 80
#define O_LAND 81
#define O_LOR 82
#define O_LE 83
#define O_GE 84
#define O_EQ 85
#define O_NE 86
#define E_ERROR 191
#define C_EOF 255




/* Copy the first part of user declarations.  */
#line 13 "parse.y"

#include "Atom.h"
#include "Function.h"
#include "Parser.h"

#define YYERROR_VERBOSE

inline void yyerror(m8r::Parser* parser, const char* s)
{
    parser->printError(s);
}

int yylex(YYSTYPE* token, m8r::Parser* parser)
{
    uint8_t t = parser->getToken(token);
    return (t == C_EOF) ? 0 : t;
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
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 93 "parse.y"
{
    m8r::Op             op;
    m8r::Label          label;
    m8r::Function*      function;
    const char*         string;
    float				number;
    uint32_t            integer;
	m8r::Atom           atom;
    uint32_t            argcount;
}
/* Line 193 of yacc.c.  */
#line 221 "parse.tab.cpp"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 234 "parse.tab.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
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
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  93
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   525

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  72
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  59
/* YYNRULES -- Number of rules.  */
#define YYNRULES  147
/* YYNRULES -- Number of states.  */
#define YYNSTATES  246

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   257

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     3,     4,     5,     6,     2,     2,     2,     2,     2,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    57,     2,     2,     2,    60,    63,     2,
      48,    49,    58,    54,    53,    55,    52,    59,    20,    21,
      22,    23,     2,     2,     2,     2,     2,     2,    67,    69,
      61,    68,    62,    66,     2,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,     2,     2,     2,
       2,    50,     2,    51,    64,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    70,    65,    71,    56,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    46,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    47,     1,     2
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    12,    14,    16,    18,
      20,    22,    24,    28,    30,    32,    37,    41,    45,    47,
      50,    53,    56,    61,    65,    67,    69,    71,    74,    77,
      80,    84,    86,    90,    92,    95,    97,    99,   101,   103,
     105,   107,   109,   111,   115,   119,   123,   125,   129,   133,
     135,   139,   143,   147,   149,   153,   157,   161,   165,   167,
     171,   175,   177,   181,   183,   187,   189,   193,   195,   199,
     201,   205,   207,   213,   215,   219,   221,   223,   225,   227,
     229,   231,   233,   235,   237,   239,   241,   243,   245,   249,
     253,   255,   259,   261,   262,   266,   269,   271,   273,   275,
     277,   279,   281,   283,   286,   290,   292,   295,   297,   300,
     306,   314,   320,   324,   330,   332,   333,   335,   338,   343,
     347,   348,   349,   357,   366,   375,   378,   381,   384,   388,
     392,   395,   396,   398,   402,   403,   411,   412,   414,   417,
     421,   423,   427,   431,   433,   435,   437,   439
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      73,     0,    -1,    74,    -1,    75,    -1,    74,    75,    -1,
     105,    -1,   120,    -1,   130,    -1,    20,    -1,    23,    -1,
      22,    -1,   126,    -1,    48,    99,    49,    -1,    76,    -1,
     121,    -1,    77,    50,    99,    51,    -1,    77,    52,   130,
      -1,     4,    77,    82,    -1,    77,    -1,     4,    78,    -1,
      77,    82,    -1,    79,    82,    -1,    79,    50,    99,    51,
      -1,    79,    52,   130,    -1,    78,    -1,    79,    -1,    80,
      -1,    80,    38,    -1,    80,    39,    -1,    48,    49,    -1,
      48,    83,    49,    -1,    97,    -1,    83,    53,    97,    -1,
      81,    -1,    85,    84,    -1,    54,    -1,    55,    -1,    56,
      -1,    57,    -1,     5,    -1,    38,    -1,    39,    -1,    84,
      -1,    86,    58,    84,    -1,    86,    59,    84,    -1,    86,
      60,    84,    -1,    86,    -1,    87,    54,    86,    -1,    87,
      55,    86,    -1,    87,    -1,    88,    37,    87,    -1,    88,
      35,    87,    -1,    88,    36,    87,    -1,    88,    -1,    89,
      61,    88,    -1,    89,    62,    88,    -1,    89,    42,    88,
      -1,    89,    43,    88,    -1,    89,    -1,    90,    44,    89,
      -1,    90,    45,    89,    -1,    90,    -1,    91,    63,    90,
      -1,    91,    -1,    92,    64,    91,    -1,    92,    -1,    93,
      65,    92,    -1,    93,    -1,    94,    40,    93,    -1,    94,
      -1,    95,    41,    94,    -1,    95,    -1,    95,    66,    99,
      67,    96,    -1,    96,    -1,    84,    98,    97,    -1,    68,
      -1,    29,    -1,    30,    -1,    31,    -1,    27,    -1,    28,
      -1,    26,    -1,    24,    -1,    25,    -1,    32,    -1,    33,
      -1,    34,    -1,    97,    -1,    99,    53,    97,    -1,     6,
     101,    69,    -1,   102,    -1,   101,    53,   102,    -1,   130,
      -1,    -1,   130,   103,   104,    -1,    68,    97,    -1,   106,
      -1,   100,    -1,   108,    -1,   109,    -1,   110,    -1,   117,
      -1,   119,    -1,    70,    71,    -1,    70,   107,    71,    -1,
     105,    -1,   107,   105,    -1,    69,    -1,    99,    69,    -1,
      10,    48,    99,    49,   105,    -1,    10,    48,    99,    49,
     105,    11,   105,    -1,    12,    48,    99,    49,   111,    -1,
      70,   112,    71,    -1,    70,   112,   115,   112,    71,    -1,
     113,    -1,    -1,   114,    -1,   113,   114,    -1,    13,    99,
      67,   105,    -1,    14,    67,   105,    -1,    -1,    -1,     8,
     116,    48,   118,    99,    49,   105,    -1,     7,   116,   105,
       8,    48,    99,    49,    69,    -1,     9,    48,   108,   116,
     108,    99,    49,   105,    -1,    16,    69,    -1,    15,    69,
      -1,    17,    69,    -1,    17,    99,    69,    -1,     3,    21,
     123,    -1,     3,   123,    -1,    -1,    21,    -1,   122,    53,
      21,    -1,    -1,    48,   124,   122,    49,    70,   125,    71,
      -1,    -1,    74,    -1,    50,    51,    -1,    50,   127,    51,
      -1,   128,    -1,   127,    53,   128,    -1,   129,    67,    97,
      -1,   130,    -1,    22,    -1,    20,    -1,    23,    -1,    21,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   125,   125,   129,   130,   134,   135,   139,   140,   141,
     142,   143,   144,   148,   149,   150,   151,   152,   156,   157,
     161,   162,   163,   164,   168,   169,   173,   174,   175,   179,
     180,   184,   185,   189,   190,   194,   195,   196,   197,   198,
     199,   200,   204,   205,   206,   207,   211,   212,   213,   217,
     218,   219,   220,   224,   225,   226,   227,   228,   232,   233,
     234,   238,   239,   243,   244,   248,   249,   253,   254,   258,
     259,   263,   264,   268,   269,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   288,   289,   293,
     297,   298,   302,   303,   303,   307,   311,   312,   313,   314,
     315,   316,   317,   321,   322,   326,   327,   331,   332,   336,
     337,   341,   345,   346,   350,   351,   355,   356,   360,   364,
     367,   370,   370,   371,   372,   376,   377,   378,   379,   382,
     384,   386,   388,   389,   393,   393,   397,   399,   403,   404,
     408,   409,   413,   417,   418,   419,   420,   424
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "K_FUNCTION", "K_NEW", "K_DELETE",
  "K_VAR", "K_DO", "K_WHILE", "K_FOR", "K_IF", "K_ELSE", "K_SWITCH",
  "K_CASE", "K_DEFAULT", "K_BREAK", "K_CONTINUE", "K_RETURN", "K_UNKNOWN",
  "K_COMMENT", "T_FLOAT", "T_IDENTIFIER", "T_STRING", "T_INTEGER",
  "O_RSHIFTEQ", "O_RSHIFTFILLEQ", "O_LSHIFTEQ", "O_ADDEQ", "O_SUBEQ",
  "O_MULEQ", "O_DIVEQ", "O_MODEQ", "O_ANDEQ", "O_XOREQ", "O_OREQ",
  "O_RSHIFT", "O_RSHIFTFILL", "O_LSHIFT", "O_INC", "O_DEC", "O_LAND",
  "O_LOR", "O_LE", "O_GE", "O_EQ", "O_NE", "E_ERROR", "\"end of file\"",
  "'('", "')'", "'['", "']'", "'.'", "','", "'+'", "'-'", "'~'", "'!'",
  "'*'", "'/'", "'%'", "'<'", "'>'", "'&'", "'^'", "'|'", "'?'", "':'",
  "'='", "';'", "'{'", "'}'", "$accept", "program", "source_elements",
  "source_element", "primary_expression", "member_expression",
  "new_expression", "call_expression", "left_hand_side_expression",
  "postfix_expression", "arguments", "argument_list", "unary_expression",
  "unary_operator", "multiplicative_expression", "additive_expression",
  "shift_expression", "relational_expression", "equality_expression",
  "and_expression", "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_or_expression",
  "conditional_expression", "assignment_expression", "assignment_operator",
  "expression", "declaration_statement", "variable_declaration_list",
  "variable_declaration", "@1", "initializer", "statement",
  "compound_statement", "statement_list", "expression_statement",
  "selection_statement", "switch_statement", "case_block",
  "case_clauses_opt", "case_clauses", "case_clause", "default_clause",
  "iteration_start", "iteration_statement", "@2", "jump_statement",
  "function_declaration", "function_expression", "formal_parameter_list",
  "function", "@3", "function_body", "object_literal",
  "property_name_and_value_list", "property_assignment", "property_name",
  "identifier", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,     1,     2,     3,     4,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      48,    49,    50,    51,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,   191,   255,    40,    41,
      91,    93,    46,    44,    43,    45,   126,    33,    42,    47,
      37,    60,    62,    38,    94,   124,    63,    58,    61,    59,
     123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    72,    73,    74,    74,    75,    75,    76,    76,    76,
      76,    76,    76,    77,    77,    77,    77,    77,    78,    78,
      79,    79,    79,    79,    80,    80,    81,    81,    81,    82,
      82,    83,    83,    84,    84,    85,    85,    85,    85,    85,
      85,    85,    86,    86,    86,    86,    87,    87,    87,    88,
      88,    88,    88,    89,    89,    89,    89,    89,    90,    90,
      90,    91,    91,    92,    92,    93,    93,    94,    94,    95,
      95,    96,    96,    97,    97,    98,    98,    98,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    99,    99,   100,
     101,   101,   102,   103,   102,   104,   105,   105,   105,   105,
     105,   105,   105,   106,   106,   107,   107,   108,   108,   109,
     109,   110,   111,   111,   112,   112,   113,   113,   114,   115,
     116,   118,   117,   117,   117,   119,   119,   119,   119,   120,
     121,   122,   122,   122,   124,   123,   125,   125,   126,   126,
     127,   127,   128,   129,   129,   129,   129,   130
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     3,     1,     1,     4,     3,     3,     1,     2,
       2,     2,     4,     3,     1,     1,     1,     2,     2,     2,
       3,     1,     3,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     1,     3,     3,     3,     3,     1,     3,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     5,     1,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       1,     3,     1,     0,     3,     2,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     1,     2,     1,     2,     5,
       7,     5,     3,     5,     1,     0,     1,     2,     4,     3,
       0,     0,     7,     8,     8,     2,     2,     2,     3,     3,
       2,     0,     1,     3,     0,     7,     0,     1,     2,     3,
       1,     3,     3,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,    39,     0,   120,   120,     0,     0,     0,
       0,     0,     0,     8,   147,    10,     9,    40,    41,     0,
       0,    35,    36,    37,    38,   107,     0,     0,     2,     3,
      13,    18,    24,    25,    26,    33,    42,     0,    46,    49,
      53,    58,    61,    63,    65,    67,    69,    71,    73,    87,
       0,    97,     5,    96,    98,    99,   100,   101,   102,     6,
      14,    11,     7,     0,   134,   130,     0,    18,    19,     0,
      90,    92,     0,     0,     0,     0,     0,   126,   125,   127,
       0,     0,   145,   144,   146,   138,     0,   140,     0,   143,
     103,   105,     0,     1,     4,     0,     0,     0,    20,     0,
       0,    21,    27,    28,    82,    83,    81,    79,    80,    76,
      77,    78,    84,    85,    86,    75,     0,    34,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   108,
     129,   131,    17,     0,    89,     0,     0,   121,   120,     0,
       0,   128,    12,   139,     0,     0,   104,   106,    29,     0,
      31,     0,    16,     0,    23,    74,    43,    44,    45,    42,
      47,    48,    51,    52,    50,    56,    57,    54,    55,    59,
      60,    62,    64,    66,    68,    70,     0,    88,   132,     0,
      91,     0,    94,     0,     0,     0,     0,     0,   141,   142,
      30,     0,    15,    22,     0,     0,     0,    95,     0,     0,
       0,   109,   115,   111,    32,    72,   136,   133,     0,     0,
       0,     0,     0,     0,   114,   116,   137,     0,     0,   122,
       0,   110,     0,     0,   112,   115,   117,   135,   123,   124,
       0,     0,     0,   118,   119,   113
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      98,   159,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,   116,    50,    51,    69,
      70,   145,   192,    52,    53,    92,    54,    55,    56,   213,
     223,   224,   225,   235,    72,    57,   194,    58,    59,    60,
     189,    65,   141,   227,    61,    86,    87,    88,    62
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -82
static const yytype_int16 yypact[] =
{
     351,   -14,     8,   -82,    -6,   -82,   -82,   -38,   -10,     4,
     -24,    -8,   125,   -82,   -82,   -82,   -82,   -82,   -82,   468,
      -3,   -82,   -82,   -82,   -82,   -82,   213,    68,   351,   -82,
     -82,    31,   -82,    32,   150,   -82,   109,   468,    92,   137,
     132,    11,   158,    23,    52,    56,   114,   -19,   -82,   -82,
     -32,   -82,   -82,   -82,   -82,   -82,   -82,   -82,   -82,   -82,
     -82,   -82,   -82,   117,   -82,   -82,   117,    31,   -82,   -30,
     -82,   103,   407,   130,   430,   468,   468,   -82,   -82,   -82,
     -20,     2,   -82,   -82,   -82,   -82,    36,   -82,   123,   -82,
     -82,   -82,   282,   -82,   -82,    21,   468,    -6,   -82,   468,
      -6,   -82,   -82,   -82,   -82,   -82,   -82,   -82,   -82,   -82,
     -82,   -82,   -82,   -82,   -82,   -82,   468,   -82,   468,   468,
     468,   468,   468,   468,   468,   468,   468,   468,   468,   468,
     468,   468,   468,   468,   468,   468,   468,   468,   468,   -82,
     -82,   111,   -82,    -6,   -82,   128,   190,   -82,   -82,    14,
      62,   -82,   -82,   -82,   136,   468,   -82,   -82,   -82,    64,
     -82,    71,   -82,   121,   -82,   -82,   -82,   -82,   -82,   -82,
      92,    92,   137,   137,   137,   132,   132,   132,   132,    11,
      11,   158,    23,    52,    56,   114,   -40,   -82,   -82,    65,
     -82,   468,   -82,   152,   468,   430,   407,   139,   -82,   -82,
     -82,   468,   -82,   -82,   468,   140,   191,   -82,   468,    70,
     468,   202,   211,   -82,   -82,   -82,   351,   -82,    78,   407,
     100,   407,   468,    -9,   211,   -82,   351,   155,   162,   -82,
     407,   -82,   -17,   160,   -82,   211,   -82,   -82,   -82,   -82,
     407,   407,   161,   -82,   -82,   -82
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -82,   -82,    22,   -25,   -82,   235,   237,   -82,   -82,   -82,
     -27,   -82,   -28,   -82,    84,    37,    58,    77,   108,   110,
     107,   112,   106,   -82,    40,   -81,   -82,   -11,   -82,   -82,
     102,   -82,   -82,   -26,   -82,   -82,   -70,   -82,   -82,   -82,
      13,   -82,    25,   -82,    -4,   -82,   -82,   -82,   -82,   -82,
     -82,   183,   -82,   -82,   -82,   -82,    96,   -82,    12
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -94
static const yytype_int16 yytable[] =
{
      91,    80,    73,    94,   148,   233,   101,    63,    81,   117,
      74,    66,     2,   138,   160,    14,    71,    82,    14,    83,
      84,   138,   136,   143,    66,     2,     3,   204,    13,    14,
      15,    16,    89,   138,    64,   165,   138,   139,    75,   144,
     142,    13,    14,    15,    16,    77,   146,   137,    85,   151,
     240,   152,    76,   126,   127,   138,    19,   187,    20,    17,
      18,    78,   234,   196,   149,   150,   157,   138,    93,    19,
     158,    20,   128,   129,   199,    21,    22,    23,    24,    95,
      95,    96,    99,    97,   100,   161,   132,   153,   163,   154,
     166,   167,   168,   169,   169,   169,   169,   169,   169,   169,
     169,   169,   169,   169,   169,   169,   169,   169,   169,   162,
     207,   197,   164,   200,   205,   138,   133,   201,   206,   219,
     214,   134,   202,   138,   138,   210,   186,   228,    66,     2,
       3,   138,   188,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   195,    13,    14,    15,    16,   230,
     118,   119,   120,   138,   135,    71,    82,    14,    83,    84,
     172,   173,   174,    17,    18,    64,    89,   123,   124,   125,
     211,   -93,   203,    19,   138,    20,   169,   115,   147,    21,
      22,    23,    24,   209,   175,   176,   177,   178,   102,   103,
     155,   121,   122,   229,    79,   231,   191,   218,   193,   220,
     208,    94,   130,   131,   239,   170,   171,   179,   180,   212,
     216,   232,   217,   221,   243,   244,    66,     2,     3,     4,
       5,     6,     7,     8,   222,     9,   237,   241,    10,    11,
      12,   238,   245,    13,    14,    15,    16,    67,   226,    68,
     181,   183,   185,   182,   215,   190,   140,   184,   242,   236,
     198,    17,    18,     0,     0,     0,     0,     0,     0,     0,
       0,    19,     0,    20,     0,     0,     0,    21,    22,    23,
      24,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    25,    26,    90,    66,     2,     3,     4,     5,
       6,     7,     8,     0,     9,     0,     0,    10,    11,    12,
       0,     0,    13,    14,    15,    16,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      17,    18,     0,     0,     0,     0,     0,     0,     0,     0,
      19,     0,    20,     0,     0,     0,    21,    22,    23,    24,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    25,    26,   156,     1,     2,     3,     4,     5,     6,
       7,     8,     0,     9,     0,     0,    10,    11,    12,     0,
       0,    13,    14,    15,    16,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    17,
      18,     0,     0,     0,     0,     0,     0,     0,     0,    19,
       0,    20,     0,     0,     0,    21,    22,    23,    24,     0,
      66,     2,     3,     4,     5,     6,     7,     8,     0,     9,
      25,    26,    10,    11,    12,     0,     0,    13,    14,    15,
      16,     0,     0,    66,     2,     3,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    17,    18,     0,     0,     0,
      13,    14,    15,    16,     0,    19,     0,    20,     0,     0,
       0,    21,    22,    23,    24,     0,     0,     0,    17,    18,
       0,    66,     2,     3,     0,     0,    25,    26,    19,     0,
      20,     0,     0,     0,    21,    22,    23,    24,    13,    14,
      15,    16,     0,     0,     0,     0,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,    17,    18,     0,     0,
       0,     0,     0,     0,     0,     0,    19,     0,    20,     0,
       0,     0,    21,    22,    23,    24
};

static const yytype_int16 yycheck[] =
{
      26,    12,     6,    28,    74,    14,    33,    21,    19,    37,
      48,     3,     4,    53,    95,    21,     4,    20,    21,    22,
      23,    53,    41,    53,     3,     4,     5,    67,    20,    21,
      22,    23,    20,    53,    48,   116,    53,    69,    48,    69,
      67,    20,    21,    22,    23,    69,    72,    66,    51,    69,
      67,    49,    48,    42,    43,    53,    48,   138,    50,    38,
      39,    69,    71,    49,    75,    76,    92,    53,     0,    48,
      49,    50,    61,    62,   155,    54,    55,    56,    57,    48,
      48,    50,    50,    52,    52,    96,    63,    51,    99,    53,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,    97,
     191,    49,   100,    49,    49,    53,    64,    53,    53,    49,
     201,    65,    51,    53,    53,   195,   137,    49,     3,     4,
       5,    53,    21,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,   148,    20,    21,    22,    23,    49,
      58,    59,    60,    53,    40,   143,    20,    21,    22,    23,
     123,   124,   125,    38,    39,    48,   154,    35,    36,    37,
     196,    68,    51,    48,    53,    50,   204,    68,    48,    54,
      55,    56,    57,   194,   126,   127,   128,   129,    38,    39,
      67,    54,    55,   219,    69,   221,    68,   208,     8,   210,
      48,   226,    44,    45,   230,   121,   122,   130,   131,    70,
      70,   222,    21,    11,   240,   241,     3,     4,     5,     6,
       7,     8,     9,    10,    13,    12,    71,    67,    15,    16,
      17,    69,    71,    20,    21,    22,    23,     2,   216,     2,
     132,   134,   136,   133,   204,   143,    63,   135,   235,   224,
     154,    38,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    48,    -1,    50,    -1,    -1,    -1,    54,    55,    56,
      57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    69,    70,    71,     3,     4,     5,     6,     7,
       8,     9,    10,    -1,    12,    -1,    -1,    15,    16,    17,
      -1,    -1,    20,    21,    22,    23,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      38,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      48,    -1,    50,    -1,    -1,    -1,    54,    55,    56,    57,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    69,    70,    71,     3,     4,     5,     6,     7,     8,
       9,    10,    -1,    12,    -1,    -1,    15,    16,    17,    -1,
      -1,    20,    21,    22,    23,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,
      -1,    50,    -1,    -1,    -1,    54,    55,    56,    57,    -1,
       3,     4,     5,     6,     7,     8,     9,    10,    -1,    12,
      69,    70,    15,    16,    17,    -1,    -1,    20,    21,    22,
      23,    -1,    -1,     3,     4,     5,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    39,    -1,    -1,    -1,
      20,    21,    22,    23,    -1,    48,    -1,    50,    -1,    -1,
      -1,    54,    55,    56,    57,    -1,    -1,    -1,    38,    39,
      -1,     3,     4,     5,    -1,    -1,    69,    70,    48,    -1,
      50,    -1,    -1,    -1,    54,    55,    56,    57,    20,    21,
      22,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    48,    -1,    50,    -1,
      -1,    -1,    54,    55,    56,    57
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    12,
      15,    16,    17,    20,    21,    22,    23,    38,    39,    48,
      50,    54,    55,    56,    57,    69,    70,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      99,   100,   105,   106,   108,   109,   110,   117,   119,   120,
     121,   126,   130,    21,    48,   123,     3,    77,    78,   101,
     102,   130,   116,   116,    48,    48,    48,    69,    69,    69,
      99,    99,    20,    22,    23,    51,   127,   128,   129,   130,
      71,   105,   107,     0,    75,    48,    50,    52,    82,    50,
      52,    82,    38,    39,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    68,    98,    84,    58,    59,
      60,    54,    55,    35,    36,    37,    42,    43,    61,    62,
      44,    45,    63,    64,    65,    40,    41,    66,    53,    69,
     123,   124,    82,    53,    69,   103,   105,    48,   108,    99,
      99,    69,    49,    51,    53,    67,    71,   105,    49,    83,
      97,    99,   130,    99,   130,    97,    84,    84,    84,    84,
      86,    86,    87,    87,    87,    88,    88,    88,    88,    89,
      89,    90,    91,    92,    93,    94,    99,    97,    21,   122,
     102,    68,   104,     8,   118,   116,    49,    49,   128,    97,
      49,    53,    51,    51,    67,    49,    53,    97,    48,    99,
     108,   105,    70,   111,    97,    96,    70,    21,    99,    49,
      99,    11,    13,   112,   113,   114,    74,   125,    49,   105,
      49,   105,    99,    14,    71,   115,   114,    71,    69,   105,
      67,    67,   112,   105,   105,    71
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


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
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (parser, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, parser)
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, parser); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, m8r::Parser* parser)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, parser)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    m8r::Parser* parser;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (parser);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, m8r::Parser* parser)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, parser)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    m8r::Parser* parser;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, parser);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, m8r::Parser* parser)
#else
static void
yy_reduce_print (yyvsp, yyrule, parser)
    YYSTYPE *yyvsp;
    int yyrule;
    m8r::Parser* parser;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , parser);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, parser); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, m8r::Parser* parser)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, parser)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    m8r::Parser* parser;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (parser);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (m8r::Parser* parser);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (m8r::Parser* parser)
#else
int
yyparse (parser)
    m8r::Parser* parser;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
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

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
        case 2:
#line 125 "parse.y"
    { parser->programEnd(); ;}
    break;

  case 8:
#line 140 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].number)); ;}
    break;

  case 9:
#line 141 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].integer)); ;}
    break;

  case 10:
#line 142 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].string)); ;}
    break;

  case 16:
#line 151 "parse.y"
    { parser->emit(m8r::Op::DEREF); ;}
    break;

  case 17:
#line 152 "parse.y"
    { parser->emitCallOrNew(false, (yyvsp[(3) - (3)].argcount)); ;}
    break;

  case 20:
#line 161 "parse.y"
    { parser->emitCallOrNew(true, (yyvsp[(2) - (2)].argcount)); ;}
    break;

  case 21:
#line 162 "parse.y"
    { parser->emitCallOrNew(true, (yyvsp[(2) - (2)].argcount)); ;}
    break;

  case 23:
#line 164 "parse.y"
    { parser->emit(m8r::Op::DEREF); ;}
    break;

  case 27:
#line 174 "parse.y"
    { parser->emit(m8r::Op::POSTINC); ;}
    break;

  case 28:
#line 175 "parse.y"
    { parser->emit(m8r::Op::POSTDEC); ;}
    break;

  case 29:
#line 179 "parse.y"
    { (yyval.argcount) = 0; ;}
    break;

  case 30:
#line 180 "parse.y"
    { (yyval.argcount) = (yyvsp[(2) - (3)].argcount); ;}
    break;

  case 31:
#line 184 "parse.y"
    { (yyval.argcount) = 1; ;}
    break;

  case 32:
#line 185 "parse.y"
    { (yyval.argcount)++; ;}
    break;

  case 34:
#line 190 "parse.y"
    { parser->emit((yyvsp[(1) - (2)].op)); ;}
    break;

  case 35:
#line 194 "parse.y"
    { (yyval.op) = m8r::Op::UPLUS; ;}
    break;

  case 36:
#line 195 "parse.y"
    { (yyval.op) = m8r::Op::UMINUS; ;}
    break;

  case 37:
#line 196 "parse.y"
    { (yyval.op) = m8r::Op::UNEG; ;}
    break;

  case 38:
#line 197 "parse.y"
    { (yyval.op) = m8r::Op::UNOT; ;}
    break;

  case 39:
#line 198 "parse.y"
    { (yyval.op) = m8r::Op::DEL; ;}
    break;

  case 40:
#line 199 "parse.y"
    { (yyval.op) = m8r::Op::PREINC; ;}
    break;

  case 41:
#line 200 "parse.y"
    { (yyval.op) = m8r::Op::PREDEC; ;}
    break;

  case 43:
#line 205 "parse.y"
    { parser->emit(m8r::Op::MUL); ;}
    break;

  case 44:
#line 206 "parse.y"
    { parser->emit(m8r::Op::DIV); ;}
    break;

  case 45:
#line 207 "parse.y"
    { parser->emit(m8r::Op::MOD); ;}
    break;

  case 47:
#line 212 "parse.y"
    { parser->emit(m8r::Op::ADD); ;}
    break;

  case 48:
#line 213 "parse.y"
    { parser->emit(m8r::Op::SUB); ;}
    break;

  case 50:
#line 218 "parse.y"
    { parser->emit(m8r::Op::SHL); ;}
    break;

  case 51:
#line 219 "parse.y"
    { parser->emit(m8r::Op::SHR); ;}
    break;

  case 52:
#line 220 "parse.y"
    { parser->emit(m8r::Op::SAR); ;}
    break;

  case 54:
#line 225 "parse.y"
    { parser->emit(m8r::Op::LT); ;}
    break;

  case 55:
#line 226 "parse.y"
    { parser->emit(m8r::Op::GT); ;}
    break;

  case 56:
#line 227 "parse.y"
    { parser->emit(m8r::Op::LE); ;}
    break;

  case 57:
#line 228 "parse.y"
    { parser->emit(m8r::Op::GE); ;}
    break;

  case 59:
#line 233 "parse.y"
    { parser->emit(m8r::Op::EQ); ;}
    break;

  case 60:
#line 234 "parse.y"
    { parser->emit(m8r::Op::NE); ;}
    break;

  case 62:
#line 239 "parse.y"
    { parser->emit(m8r::Op::AND); ;}
    break;

  case 64:
#line 244 "parse.y"
    { parser->emit(m8r::Op::XOR); ;}
    break;

  case 66:
#line 249 "parse.y"
    { parser->emit(m8r::Op::OR); ;}
    break;

  case 68:
#line 254 "parse.y"
    { parser->emit(m8r::Op::LAND); ;}
    break;

  case 70:
#line 259 "parse.y"
    { parser->emit(m8r::Op::LOR); ;}
    break;

  case 74:
#line 269 "parse.y"
    { parser->emit((yyvsp[(2) - (3)].op)); ;}
    break;

  case 75:
#line 273 "parse.y"
    { (yyval.op) = m8r::Op::STO; ;}
    break;

  case 76:
#line 274 "parse.y"
    { (yyval.op) = m8r::Op::STOMUL; ;}
    break;

  case 77:
#line 275 "parse.y"
    { (yyval.op) = m8r::Op::STODIV; ;}
    break;

  case 78:
#line 276 "parse.y"
    { (yyval.op) = m8r::Op::STOMOD; ;}
    break;

  case 79:
#line 277 "parse.y"
    { (yyval.op) = m8r::Op::STOADD; ;}
    break;

  case 80:
#line 278 "parse.y"
    { (yyval.op) = m8r::Op::STOSUB; ;}
    break;

  case 81:
#line 279 "parse.y"
    { (yyval.op) = m8r::Op::STOSHL; ;}
    break;

  case 82:
#line 280 "parse.y"
    { (yyval.op) = m8r::Op::STOSHR; ;}
    break;

  case 83:
#line 281 "parse.y"
    { (yyval.op) = m8r::Op::STOSAR; ;}
    break;

  case 84:
#line 282 "parse.y"
    { (yyval.op) = m8r::Op::STOAND; ;}
    break;

  case 85:
#line 283 "parse.y"
    { (yyval.op) = m8r::Op::STOXOR; ;}
    break;

  case 86:
#line 284 "parse.y"
    { (yyval.op) = m8r::Op::STOOR; ;}
    break;

  case 92:
#line 302 "parse.y"
    { parser->emit(m8r::Op::NEWID); ;}
    break;

  case 93:
#line 303 "parse.y"
    { parser->emit(m8r::Op::NEWID); ;}
    break;

  case 95:
#line 307 "parse.y"
    { parser->emit(m8r::Op::STO); ;}
    break;

  case 120:
#line 367 "parse.y"
    { (yyval.label) = parser->label(); ;}
    break;

  case 121:
#line 370 "parse.y"
    { parser->loopStart(false, (yyvsp[(2) - (3)].label)); ;}
    break;

  case 122:
#line 370 "parse.y"
    { parser->loopEnd((yyvsp[(2) - (7)].label)); ;}
    break;

  case 129:
#line 382 "parse.y"
    { parser->addNamedFunction((yyvsp[(3) - (3)].function), (yyvsp[(2) - (3)].atom)); ;}
    break;

  case 130:
#line 384 "parse.y"
    { parser->addObject((yyvsp[(2) - (2)].function)); ;}
    break;

  case 132:
#line 388 "parse.y"
    { parser->functionAddParam((yyvsp[(1) - (1)].atom)); ;}
    break;

  case 134:
#line 393 "parse.y"
    { parser->functionStart(); ;}
    break;

  case 135:
#line 394 "parse.y"
    { parser->emit(m8r::Op::END); (yyval.function) = parser->functionEnd(); ;}
    break;

  case 144:
#line 418 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].string)); ;}
    break;

  case 145:
#line 419 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].number)); ;}
    break;

  case 146:
#line 420 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].integer)); ;}
    break;

  case 147:
#line 424 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].atom)); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 2089 "parse.tab.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (parser, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (parser, yymsg);
	  }
	else
	  {
	    yyerror (parser, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, parser);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, parser);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (parser, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, parser);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, parser);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 427 "parse.y"


