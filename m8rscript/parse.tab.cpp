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
     C_EOF = 255,
     UNARY = 258
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
#define UNARY 258




/* Copy the first part of user declarations.  */
#line 13 "parse.y"

#include "Atom.h"
#include "Function.h"
#include "Program.h"
#include "Parser.h"

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
#line 92 "parse.y"
{
    m8r::Op             op;
    m8r::Label          label;
    m8r::Function*      function;
    m8r::StringId       string;
    float				number;
    uint32_t            integer;
	m8r::Atom           atom;
    uint32_t            argcount;
}
/* Line 193 of yacc.c.  */
#line 222 "parse.tab.cpp"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 235 "parse.tab.cpp"

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
#define YYFINAL  89
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   807

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  73
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  60
/* YYNRULES -- Number of rules.  */
#define YYNRULES  156
/* YYNRULES -- Number of states.  */
#define YYNSTATES  272

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   258

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     3,     4,     5,     6,     2,     2,     2,     2,     2,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    68,     2,     2,     2,    59,    52,     2,
      61,    62,    57,    55,    66,    56,    65,    58,    20,    21,
      22,    23,     2,     2,     2,     2,     2,     2,    49,    70,
      53,    69,    54,    48,     2,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,     2,     2,     2,
       2,    63,     2,    64,    51,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    71,    50,    72,    67,     2,     2,     2,
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
       2,     2,     2,     2,     2,    47,     1,     2,    60
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    12,    14,    16,    18,
      20,    22,    24,    26,    30,    32,    34,    39,    43,    47,
      49,    52,    55,    58,    63,    67,    69,    71,    74,    77,
      80,    83,    86,    90,    92,    96,    98,   100,   102,   104,
     106,   108,   111,   115,   119,   123,   127,   131,   135,   139,
     143,   147,   151,   155,   159,   163,   167,   171,   175,   179,
     183,   187,   193,   197,   201,   203,   205,   207,   209,   211,
     213,   215,   217,   219,   221,   223,   225,   227,   229,   231,
     235,   237,   238,   242,   245,   247,   249,   253,   257,   258,
     262,   263,   267,   268,   272,   274,   276,   278,   280,   283,
     287,   289,   292,   298,   306,   312,   316,   322,   324,   325,
     327,   330,   335,   339,   340,   341,   342,   347,   348,   352,
     354,   358,   359,   367,   376,   377,   378,   391,   394,   397,
     400,   404,   408,   411,   412,   414,   418,   419,   420,   429,
     430,   432,   433,   437,   438,   443,   445,   449,   453,   454,
     456,   460,   464,   466,   468,   470,   472
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      74,     0,    -1,    75,    -1,    76,    -1,    75,    76,    -1,
      94,    -1,   117,    -1,   132,    -1,    20,    -1,    23,    -1,
      22,    -1,   128,    -1,   124,    -1,    61,    89,    62,    -1,
      77,    -1,   118,    -1,    78,    63,    89,    64,    -1,    78,
      65,   132,    -1,     4,    78,    83,    -1,    78,    -1,     4,
      79,    -1,    78,    83,    -1,    80,    83,    -1,    80,    63,
      89,    64,    -1,    80,    65,   132,    -1,    79,    -1,    80,
      -1,    81,    38,    -1,    81,    39,    -1,    38,    81,    -1,
      39,    81,    -1,    61,    62,    -1,    61,    84,    62,    -1,
      89,    -1,    84,    66,    89,    -1,    55,    -1,    56,    -1,
      67,    -1,    68,    -1,    82,    -1,    81,    -1,    85,    86,
      -1,    86,    57,    86,    -1,    86,    58,    86,    -1,    86,
      59,    86,    -1,    86,    55,    86,    -1,    86,    56,    86,
      -1,    86,    37,    86,    -1,    86,    35,    86,    -1,    86,
      36,    86,    -1,    86,    53,    86,    -1,    86,    54,    86,
      -1,    86,    42,    86,    -1,    86,    43,    86,    -1,    86,
      44,    86,    -1,    86,    45,    86,    -1,    86,    52,    86,
      -1,    86,    51,    86,    -1,    86,    50,    86,    -1,    86,
      40,    86,    -1,    86,    41,    86,    -1,    86,    48,    86,
      49,    86,    -1,    81,    88,    86,    -1,    81,    88,    87,
      -1,    69,    -1,    29,    -1,    30,    -1,    31,    -1,    27,
      -1,    28,    -1,    26,    -1,    24,    -1,    25,    -1,    32,
      -1,    33,    -1,    34,    -1,    87,    -1,    86,    -1,    91,
      -1,    90,    66,    91,    -1,    21,    -1,    -1,    21,    92,
      93,    -1,    69,    89,    -1,    70,    -1,    98,    -1,     6,
      90,    70,    -1,     5,    81,    70,    -1,    -1,    80,    95,
      70,    -1,    -1,    82,    96,    70,    -1,    -1,    87,    97,
      70,    -1,   100,    -1,   101,    -1,   112,    -1,   116,    -1,
      71,    72,    -1,    71,    99,    72,    -1,    94,    -1,    99,
      94,    -1,    10,    61,    89,    62,    94,    -1,    10,    61,
      89,    62,    94,    11,    94,    -1,    12,    61,    89,    62,
     102,    -1,    71,   103,    72,    -1,    71,   103,   106,   103,
      72,    -1,   104,    -1,    -1,   105,    -1,   104,   105,    -1,
      13,    89,    49,    94,    -1,    14,    49,    94,    -1,    -1,
      -1,    -1,     6,    21,   109,    93,    -1,    -1,    21,   110,
      93,    -1,   108,    -1,   111,    66,   108,    -1,    -1,     8,
      61,   107,    89,   113,    62,    94,    -1,     7,   107,    94,
       8,    61,    89,    62,    70,    -1,    -1,    -1,     9,    61,
     111,    70,   107,    89,   114,    70,    89,   115,    62,    94,
      -1,    16,    70,    -1,    15,    70,    -1,    17,    70,    -1,
      17,    89,    70,    -1,     3,    21,   120,    -1,     3,   120,
      -1,    -1,    21,    -1,   119,    66,    21,    -1,    -1,    -1,
      61,   121,   119,   122,    62,    71,   123,    72,    -1,    -1,
      75,    -1,    -1,    63,   125,    64,    -1,    -1,    63,   126,
     127,    64,    -1,    89,    -1,   127,    66,    89,    -1,    71,
     129,    72,    -1,    -1,   130,    -1,   129,    66,   130,    -1,
     131,    49,    89,    -1,   132,    -1,    22,    -1,    20,    -1,
      23,    -1,    21,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   140,   140,   144,   145,   149,   150,   154,   155,   156,
     157,   158,   159,   160,   164,   165,   166,   167,   168,   172,
     173,   177,   178,   179,   180,   184,   185,   189,   190,   191,
     192,   196,   197,   201,   202,   206,   207,   208,   209,   213,
     214,   215,   216,   217,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,   231,   232,   233,
     234,   235,   239,   240,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   259,   260,   264,   265,
     269,   270,   270,   274,   278,   279,   280,   281,   282,   282,
     283,   283,   284,   284,   285,   286,   287,   288,   292,   293,
     297,   298,   302,   303,   307,   311,   312,   316,   317,   321,
     322,   326,   330,   333,   335,   337,   337,   338,   338,   342,
     343,   361,   360,   364,   366,   368,   365,   374,   375,   376,
     377,   380,   382,   384,   386,   387,   391,   393,   391,   398,
     400,   404,   404,   405,   405,   409,   410,   414,   417,   419,
     420,   424,   428,   429,   430,   431,   435
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
  "'?'", "':'", "'|'", "'^'", "'&'", "'<'", "'>'", "'+'", "'-'", "'*'",
  "'/'", "'%'", "UNARY", "'('", "')'", "'['", "']'", "'.'", "','", "'~'",
  "'!'", "'='", "';'", "'{'", "'}'", "$accept", "program",
  "source_elements", "source_element", "primary_expression",
  "member_expression", "new_expression", "call_expression",
  "left_hand_side_expression", "mutation_expression", "arguments",
  "argument_list", "unary_operator", "arithmetic_expression",
  "assignment_expression", "assignment_operator", "expression",
  "variable_declaration_list", "variable_declaration", "@1", "initializer",
  "statement", "@2", "@3", "@4", "compound_statement", "statement_list",
  "selection_statement", "switch_statement", "case_block",
  "case_clauses_opt", "case_clauses", "case_clause", "default_clause",
  "iteration_start", "for_loop_initializer", "@5", "@6",
  "for_loop_initializers", "iteration_statement", "@7", "@8", "@9",
  "jump_statement", "function_declaration", "function_expression",
  "formal_parameter_list", "function", "@10", "@11", "function_body",
  "array_literal", "@12", "@13", "array_value_list", "object_literal",
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
      81,    82,    83,    84,    85,    86,   191,   255,    63,    58,
     124,    94,    38,    60,    62,    43,    45,    42,    47,    37,
     258,    40,    41,    91,    93,    46,    44,   126,    33,    61,
      59,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    73,    74,    75,    75,    76,    76,    77,    77,    77,
      77,    77,    77,    77,    78,    78,    78,    78,    78,    79,
      79,    80,    80,    80,    80,    81,    81,    82,    82,    82,
      82,    83,    83,    84,    84,    85,    85,    85,    85,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    87,    87,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    89,    89,    90,    90,
      91,    92,    91,    93,    94,    94,    94,    94,    95,    94,
      96,    94,    97,    94,    94,    94,    94,    94,    98,    98,
      99,    99,   100,   100,   101,   102,   102,   103,   103,   104,
     104,   105,   106,   107,   108,   109,   108,   110,   108,   111,
     111,   113,   112,   112,   114,   115,   112,   116,   116,   116,
     116,   117,   118,   119,   119,   119,   121,   122,   120,   123,
     123,   125,   124,   126,   124,   127,   127,   128,   129,   129,
     129,   130,   131,   131,   131,   131,   132
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     1,     1,     4,     3,     3,     1,
       2,     2,     2,     4,     3,     1,     1,     2,     2,     2,
       2,     2,     3,     1,     3,     1,     1,     1,     1,     1,
       1,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     5,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       1,     0,     3,     2,     1,     1,     3,     3,     0,     3,
       0,     3,     0,     3,     1,     1,     1,     1,     2,     3,
       1,     2,     5,     7,     5,     3,     5,     1,     0,     1,
       2,     4,     3,     0,     0,     0,     4,     0,     3,     1,
       3,     0,     7,     8,     0,     0,    12,     2,     2,     2,
       3,     3,     2,     0,     1,     3,     0,     0,     8,     0,
       1,     0,     3,     0,     4,     1,     3,     3,     0,     1,
       3,     3,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,   113,     0,     0,     0,     0,
       0,     0,     0,     8,   156,    10,     9,     0,     0,     0,
     143,    84,   148,     0,     2,     3,    14,    19,    25,    26,
       0,    90,    92,     5,    85,    94,    95,    96,    97,     6,
      15,    12,    11,     7,     0,   136,   132,     0,   148,    19,
      20,    26,     0,    80,     0,    78,     0,   113,   114,     0,
       0,   128,   127,    35,    36,    37,    38,   129,    40,    39,
       0,    77,    76,     0,    29,    30,     0,     0,     0,     8,
      10,     9,    98,   100,     0,     0,   149,     0,     7,     1,
       4,     0,     0,     0,    21,     0,     0,    22,     0,    71,
      72,    70,    68,    69,    65,    66,    67,    73,    74,    75,
      27,    28,    64,     0,     0,     0,   131,   133,   154,   153,
     155,   152,    18,    87,     0,     0,    86,     0,     0,     0,
     117,   119,     0,     0,     0,    40,    41,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   130,    13,   142,
     145,     0,    99,   101,     0,   147,     0,    31,     0,    33,
       0,    17,     0,    24,    89,    62,    63,    91,    93,   134,
     137,     0,    82,    79,     0,   121,   115,     0,   114,   113,
       0,     0,    48,    49,    47,    59,    60,    52,    53,    54,
      55,     0,    58,    57,    56,    50,    51,    45,    46,    42,
      43,    44,   144,     0,   150,   151,    32,     0,    16,    23,
       0,     0,    83,     0,     0,     0,   118,   120,     0,   102,
     108,   104,     0,   146,    34,   135,     0,     0,     0,   116,
     124,     0,     0,     0,   107,   109,    61,   139,     0,   122,
       0,   103,     0,     0,   105,   108,   110,   140,     0,   123,
       0,     0,     0,     0,   138,   125,   111,   112,   106,     0,
       0,   126
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    23,    24,    25,    26,    27,    28,    51,   135,    69,
      97,   168,    70,    71,    72,   113,    73,    54,    55,   124,
     182,    33,    98,   114,   115,    34,    84,    35,    36,   231,
     243,   244,   245,   255,    56,   131,   225,   187,   132,    37,
     224,   250,   269,    38,    39,    40,   180,    46,   117,   221,
     258,    41,    77,    78,   161,    42,    85,    86,    87,    43
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -174
static const yytype_int16 yypact[] =
{
     472,   -15,   164,   164,    11,  -174,   -48,   -18,   -16,   -13,
     -29,   -12,    84,  -174,  -174,  -174,  -174,   164,   164,   561,
     -10,  -174,   365,    68,   472,  -174,  -174,   -28,  -174,   -34,
     167,  -174,  -174,  -174,  -174,  -174,  -174,  -174,  -174,  -174,
    -174,  -174,  -174,  -174,    13,  -174,  -174,    13,    80,   -28,
    -174,    -8,     2,     8,   -50,  -174,   509,  -174,     5,   561,
     561,  -174,  -174,  -174,  -174,  -174,  -174,  -174,   167,  -174,
     561,   623,  -174,     9,  -174,  -174,    18,    21,   561,    40,
      41,    44,  -174,  -174,   402,   -57,  -174,    45,    47,  -174,
    -174,   535,   561,    87,  -174,   561,    87,  -174,    50,  -174,
    -174,  -174,  -174,  -174,  -174,  -174,  -174,  -174,  -174,  -174,
    -174,  -174,  -174,   561,    51,    54,  -174,    89,  -174,  -174,
    -174,  -174,  -174,  -174,    43,    11,  -174,   103,   561,   104,
    -174,  -174,   -32,    65,    67,    32,  -174,   561,   561,   561,
     561,   561,   561,   561,   561,   561,   561,   561,   561,   561,
     561,   561,   561,   561,   561,   561,   561,  -174,  -174,  -174,
    -174,   -43,  -174,  -174,    80,  -174,   561,  -174,   -22,  -174,
      66,  -174,    69,  -174,  -174,   623,  -174,  -174,  -174,  -174,
      71,   561,  -174,  -174,    73,  -174,  -174,    43,     5,  -174,
     509,    61,    59,    59,    59,   673,   648,   106,   106,   272,
     272,   598,   698,   723,   748,   106,   106,     4,     4,  -174,
    -174,  -174,  -174,   561,  -174,  -174,  -174,   561,  -174,  -174,
     114,    86,  -174,   561,    88,    43,  -174,  -174,   561,   135,
     136,  -174,   561,  -174,  -174,  -174,    82,    94,   509,  -174,
    -174,   509,   561,    -7,   136,  -174,   623,   472,    90,  -174,
      99,  -174,   109,   121,  -174,   136,  -174,   472,   102,  -174,
     561,   509,   509,   105,  -174,  -174,  -174,  -174,  -174,   113,
     509,  -174
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -174,  -174,   -76,   -23,  -174,   174,   176,    42,     0,    75,
     -19,  -174,  -174,   211,    25,  -174,    -9,  -174,    55,  -174,
    -173,   -17,  -174,  -174,  -174,  -174,  -174,  -174,  -174,  -174,
     -73,  -174,   -65,  -174,   -53,    -5,  -174,  -174,  -174,  -174,
    -174,  -174,  -174,  -174,  -174,  -174,  -174,   144,  -174,  -174,
    -174,  -174,  -174,  -174,  -174,  -174,  -174,    38,  -174,   -20
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -156
static const yytype_int16 yytable[] =
{
      30,    90,    88,    52,   128,    83,    44,   253,    94,   164,
      76,   129,    68,    57,   226,   165,   125,    74,    75,    68,
     126,   212,    30,   213,    30,    32,   130,    91,   121,    95,
     122,    96,    53,    91,   188,    92,   -88,    93,   189,   127,
     216,    61,    29,    58,   217,    59,    45,    32,    60,    32,
     133,   134,   239,    91,  -141,    95,    30,    96,    62,    68,
      68,   154,   155,   156,    29,   254,    29,   163,    89,   160,
     110,   111,   123,   171,    45,    31,   173,   -81,    68,   157,
     158,    32,   169,   170,    30,   159,   172,    47,     2,  -154,
    -153,    68,    68,  -155,   166,    68,  -152,    31,    29,    31,
     118,    14,   119,   120,    13,    14,    15,    16,    14,    32,
     179,   184,   181,    68,   152,   153,   154,   155,   156,   185,
     174,   177,    17,    18,   178,   186,    29,   190,    68,   191,
     218,    31,   230,   219,   223,   235,   228,   220,   176,    63,
      64,   137,   138,   139,   121,    19,   241,    20,   236,   242,
     238,    65,    66,   247,    67,    48,   248,   215,   261,    31,
     259,   152,   153,   154,   155,   156,    68,    47,     2,   260,
     262,   257,   222,   229,   264,   270,    49,   268,    50,   256,
     183,    68,   263,   227,    13,    14,    15,    16,   116,     0,
      30,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   214,     0,   233,   110,   111,     0,   234,     0,
       0,     0,     0,    68,   237,    32,     0,    68,     0,   240,
       0,   249,     0,    68,   251,    19,     0,    20,    68,     0,
       0,     0,    29,   252,    90,    48,   112,     0,    30,     0,
       0,    30,    68,     0,   266,   267,     0,    30,     0,     0,
       0,   265,     0,   271,     0,     0,     0,    30,     0,     0,
      68,    30,    30,    32,     0,    31,    32,     0,     0,     0,
      30,     0,    32,     0,     0,     0,     0,     0,     0,     0,
      29,   136,    32,    29,     0,     0,    32,    32,     0,    29,
       0,     0,     0,     0,     0,    32,     0,     0,     0,    29,
       0,     0,     0,    29,    29,     0,     0,   137,   138,   139,
       0,     0,    29,    31,   142,   143,    31,     0,     0,     0,
       0,     0,    31,     0,   175,   150,   151,   152,   153,   154,
     155,   156,    31,     0,     0,     0,    31,    31,     0,     0,
       0,     0,     0,     0,     0,    31,     0,     0,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,    47,     2,
       3,     4,     5,     6,     7,     8,     0,     9,     0,     0,
      10,    11,    12,     0,     0,    79,    14,    80,    81,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    17,    18,    47,     2,     3,     4,     5,
       6,     7,     8,     0,     9,     0,     0,    10,    11,    12,
       0,     0,    13,    14,    15,    16,    19,     0,    20,     0,
       0,     0,     0,     0,     0,    21,    22,    82,     0,     0,
      17,    18,     0,   246,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    19,     0,    20,     0,     0,     0,     0,
       0,     0,    21,    22,   162,     1,     2,     3,     4,     5,
       6,     7,     8,     0,     9,     0,     0,    10,    11,    12,
       0,     0,    13,    14,    15,    16,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      17,    18,    47,     2,     3,     4,     5,     6,     7,     8,
       0,     9,     0,     0,    10,    11,    12,     0,     0,    13,
      14,    15,    16,    19,     0,    20,     0,     0,    47,     2,
       0,     0,    21,    22,     0,     0,     0,    17,    18,     0,
       0,     0,     0,     0,     0,    13,    14,    15,    16,     0,
       0,     0,     0,     0,    47,     2,     0,     0,     0,     0,
      19,     0,    20,    17,    18,     0,     0,     0,     0,    21,
      22,    13,    14,    15,    16,     0,     0,     0,     0,     0,
      63,    64,     0,     0,     0,     0,    19,   167,    20,    17,
      18,     0,    65,    66,     0,     0,    48,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    63,    64,     0,     0,
       0,     0,    19,     0,    20,     0,     0,     0,    65,    66,
       0,     0,    48,   137,   138,   139,     0,     0,   140,   141,
     142,   143,   144,   145,     0,     0,   146,   232,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   137,   138,
     139,     0,     0,   140,   141,   142,   143,   144,   145,     0,
       0,   146,     0,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   137,   138,   139,     0,     0,   140,     0,
     142,   143,   144,   145,     0,     0,     0,     0,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   137,   138,
     139,     0,     0,     0,     0,   142,   143,   144,   145,     0,
       0,     0,     0,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   137,   138,   139,     0,     0,     0,     0,
     142,   143,   144,   145,     0,     0,     0,     0,     0,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   137,   138,
     139,     0,     0,     0,     0,   142,   143,   144,   145,     0,
       0,     0,     0,     0,     0,   149,   150,   151,   152,   153,
     154,   155,   156,   137,   138,   139,     0,     0,     0,     0,
     142,   143,   144,   145,     0,     0,     0,     0,     0,     0,
       0,   150,   151,   152,   153,   154,   155,   156
};

static const yytype_int16 yycheck[] =
{
       0,    24,    22,     3,    57,    22,    21,    14,    27,    66,
      19,     6,    12,    61,   187,    72,    66,    17,    18,    19,
      70,    64,    22,    66,    24,     0,    21,    61,    48,    63,
      49,    65,    21,    61,    66,    63,    70,    65,    70,    56,
      62,    70,     0,    61,    66,    61,    61,    22,    61,    24,
      59,    60,   225,    61,    64,    63,    56,    65,    70,    59,
      60,    57,    58,    59,    22,    72,    24,    84,     0,    78,
      38,    39,    70,    93,    61,     0,    96,    69,    78,    70,
      62,    56,    91,    92,    84,    64,    95,     3,     4,    49,
      49,    91,    92,    49,    49,    95,    49,    22,    56,    24,
      20,    21,    22,    23,    20,    21,    22,    23,    21,    84,
      21,     8,    69,   113,    55,    56,    57,    58,    59,   128,
      70,    70,    38,    39,    70,    21,    84,    62,   128,    62,
      64,    56,    71,    64,    61,    21,   189,    66,   113,    55,
      56,    35,    36,    37,   164,    61,    11,    63,    62,    13,
      62,    67,    68,    71,    70,    71,    62,   166,    49,    84,
      70,    55,    56,    57,    58,    59,   166,     3,     4,    70,
      49,   247,   181,   190,    72,    62,     2,    72,     2,   244,
     125,   181,   255,   188,    20,    21,    22,    23,    44,    -1,
     190,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,   164,    -1,   213,    38,    39,    -1,   217,    -1,
      -1,    -1,    -1,   213,   223,   190,    -1,   217,    -1,   228,
      -1,   238,    -1,   223,   241,    61,    -1,    63,   228,    -1,
      -1,    -1,   190,   242,   257,    71,    69,    -1,   238,    -1,
      -1,   241,   242,    -1,   261,   262,    -1,   247,    -1,    -1,
      -1,   260,    -1,   270,    -1,    -1,    -1,   257,    -1,    -1,
     260,   261,   262,   238,    -1,   190,   241,    -1,    -1,    -1,
     270,    -1,   247,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     238,    70,   257,   241,    -1,    -1,   261,   262,    -1,   247,
      -1,    -1,    -1,    -1,    -1,   270,    -1,    -1,    -1,   257,
      -1,    -1,    -1,   261,   262,    -1,    -1,    35,    36,    37,
      -1,    -1,   270,   238,    42,    43,   241,    -1,    -1,    -1,
      -1,    -1,   247,    -1,   113,    53,    54,    55,    56,    57,
      58,    59,   257,    -1,    -1,    -1,   261,   262,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   270,    -1,    -1,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,     3,     4,
       5,     6,     7,     8,     9,    10,    -1,    12,    -1,    -1,
      15,    16,    17,    -1,    -1,    20,    21,    22,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    39,     3,     4,     5,     6,     7,
       8,     9,    10,    -1,    12,    -1,    -1,    15,    16,    17,
      -1,    -1,    20,    21,    22,    23,    61,    -1,    63,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,    -1,
      38,    39,    -1,   232,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    61,    -1,    63,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,     3,     4,     5,     6,     7,
       8,     9,    10,    -1,    12,    -1,    -1,    15,    16,    17,
      -1,    -1,    20,    21,    22,    23,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      38,    39,     3,     4,     5,     6,     7,     8,     9,    10,
      -1,    12,    -1,    -1,    15,    16,    17,    -1,    -1,    20,
      21,    22,    23,    61,    -1,    63,    -1,    -1,     3,     4,
      -1,    -1,    70,    71,    -1,    -1,    -1,    38,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    20,    21,    22,    23,    -1,
      -1,    -1,    -1,    -1,     3,     4,    -1,    -1,    -1,    -1,
      61,    -1,    63,    38,    39,    -1,    -1,    -1,    -1,    70,
      71,    20,    21,    22,    23,    -1,    -1,    -1,    -1,    -1,
      55,    56,    -1,    -1,    -1,    -1,    61,    62,    63,    38,
      39,    -1,    67,    68,    -1,    -1,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    -1,
      -1,    -1,    61,    -1,    63,    -1,    -1,    -1,    67,    68,
      -1,    -1,    71,    35,    36,    37,    -1,    -1,    40,    41,
      42,    43,    44,    45,    -1,    -1,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    35,    36,
      37,    -1,    -1,    40,    41,    42,    43,    44,    45,    -1,
      -1,    48,    -1,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    35,    36,    37,    -1,    -1,    40,    -1,
      42,    43,    44,    45,    -1,    -1,    -1,    -1,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    35,    36,
      37,    -1,    -1,    -1,    -1,    42,    43,    44,    45,    -1,
      -1,    -1,    -1,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    35,    36,    37,    -1,    -1,    -1,    -1,
      42,    43,    44,    45,    -1,    -1,    -1,    -1,    -1,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    35,    36,
      37,    -1,    -1,    -1,    -1,    42,    43,    44,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,
      57,    58,    59,    35,    36,    37,    -1,    -1,    -1,    -1,
      42,    43,    44,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    53,    54,    55,    56,    57,    58,    59
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    12,
      15,    16,    17,    20,    21,    22,    23,    38,    39,    61,
      63,    70,    71,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    87,    94,    98,   100,   101,   112,   116,   117,
     118,   124,   128,   132,    21,    61,   120,     3,    71,    78,
      79,    80,    81,    21,    90,    91,   107,    61,    61,    61,
      61,    70,    70,    55,    56,    67,    68,    70,    81,    82,
      85,    86,    87,    89,    81,    81,    89,   125,   126,    20,
      22,    23,    72,    94,    99,   129,   130,   131,   132,     0,
      76,    61,    63,    65,    83,    63,    65,    83,    95,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      38,    39,    69,    88,    96,    97,   120,   121,    20,    22,
      23,   132,    83,    70,    92,    66,    70,    94,   107,     6,
      21,   108,   111,    89,    89,    81,    86,    35,    36,    37,
      40,    41,    42,    43,    44,    45,    48,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    70,    62,    64,
      89,   127,    72,    94,    66,    72,    49,    62,    84,    89,
      89,   132,    89,   132,    70,    86,    87,    70,    70,    21,
     119,    69,    93,    91,     8,    89,    21,   110,    66,    70,
      62,    62,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    64,    66,   130,    89,    62,    66,    64,    64,
      66,   122,    89,    61,   113,   109,    93,   108,   107,    94,
      71,   102,    49,    89,    89,    21,    62,    89,    62,    93,
      89,    11,    13,   103,   104,   105,    86,    71,    62,    94,
     114,    94,    89,    14,    72,   106,   105,    75,   123,    70,
      70,    49,    49,   103,    72,    89,    94,    94,    72,   115,
      62,    94
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
#line 140 "parse.y"
    { parser->programEnd(); ;}
    break;

  case 8:
#line 155 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].number)); ;}
    break;

  case 9:
#line 156 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].integer)); ;}
    break;

  case 10:
#line 157 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].string)); ;}
    break;

  case 16:
#line 166 "parse.y"
    { parser->emit(m8r::Op::DEREF); ;}
    break;

  case 17:
#line 167 "parse.y"
    { parser->emit(m8r::Op::DEREF); ;}
    break;

  case 18:
#line 168 "parse.y"
    { parser->emitWithCount(m8r::Op::NEW, (yyvsp[(3) - (3)].argcount)); ;}
    break;

  case 21:
#line 177 "parse.y"
    { parser->emitWithCount(m8r::Op::CALL, (yyvsp[(2) - (2)].argcount)); ;}
    break;

  case 22:
#line 178 "parse.y"
    { parser->emitWithCount(m8r::Op::CALL, (yyvsp[(2) - (2)].argcount)); ;}
    break;

  case 24:
#line 180 "parse.y"
    { parser->emit(m8r::Op::DEREF); ;}
    break;

  case 27:
#line 189 "parse.y"
    { parser->emit(m8r::Op::POSTINC); ;}
    break;

  case 28:
#line 190 "parse.y"
    { parser->emit(m8r::Op::POSTDEC); ;}
    break;

  case 29:
#line 191 "parse.y"
    { parser->emit(m8r::Op::PREINC); ;}
    break;

  case 30:
#line 192 "parse.y"
    { parser->emit(m8r::Op::PREDEC); ;}
    break;

  case 31:
#line 196 "parse.y"
    { (yyval.argcount) = 0; ;}
    break;

  case 32:
#line 197 "parse.y"
    { (yyval.argcount) = (yyvsp[(2) - (3)].argcount); ;}
    break;

  case 33:
#line 201 "parse.y"
    { (yyval.argcount) = 1; ;}
    break;

  case 34:
#line 202 "parse.y"
    { (yyval.argcount)++; ;}
    break;

  case 35:
#line 206 "parse.y"
    { (yyval.op) = m8r::Op::UPLUS; ;}
    break;

  case 36:
#line 207 "parse.y"
    { (yyval.op) = m8r::Op::UMINUS; ;}
    break;

  case 37:
#line 208 "parse.y"
    { (yyval.op) = m8r::Op::UNEG; ;}
    break;

  case 38:
#line 209 "parse.y"
    { (yyval.op) = m8r::Op::UNOT; ;}
    break;

  case 41:
#line 215 "parse.y"
    { parser->emit((yyvsp[(1) - (2)].op)); ;}
    break;

  case 42:
#line 216 "parse.y"
    { parser->emit(m8r::Op::MUL); ;}
    break;

  case 43:
#line 217 "parse.y"
    { parser->emit(m8r::Op::DIV); ;}
    break;

  case 44:
#line 218 "parse.y"
    { parser->emit(m8r::Op::MOD); ;}
    break;

  case 45:
#line 219 "parse.y"
    { parser->emit(m8r::Op::ADD); ;}
    break;

  case 46:
#line 220 "parse.y"
    { parser->emit(m8r::Op::SUB); ;}
    break;

  case 47:
#line 221 "parse.y"
    { parser->emit(m8r::Op::SHL); ;}
    break;

  case 48:
#line 222 "parse.y"
    { parser->emit(m8r::Op::SHR); ;}
    break;

  case 49:
#line 223 "parse.y"
    { parser->emit(m8r::Op::SAR); ;}
    break;

  case 50:
#line 224 "parse.y"
    { parser->emit(m8r::Op::LT); ;}
    break;

  case 51:
#line 225 "parse.y"
    { parser->emit(m8r::Op::GT); ;}
    break;

  case 52:
#line 226 "parse.y"
    { parser->emit(m8r::Op::LE); ;}
    break;

  case 53:
#line 227 "parse.y"
    { parser->emit(m8r::Op::GE); ;}
    break;

  case 54:
#line 228 "parse.y"
    { parser->emit(m8r::Op::EQ); ;}
    break;

  case 55:
#line 229 "parse.y"
    { parser->emit(m8r::Op::NE); ;}
    break;

  case 56:
#line 230 "parse.y"
    { parser->emit(m8r::Op::AND); ;}
    break;

  case 57:
#line 231 "parse.y"
    { parser->emit(m8r::Op::XOR); ;}
    break;

  case 58:
#line 232 "parse.y"
    { parser->emit(m8r::Op::OR); ;}
    break;

  case 59:
#line 233 "parse.y"
    { parser->emit(m8r::Op::LAND); ;}
    break;

  case 60:
#line 234 "parse.y"
    { parser->emit(m8r::Op::LOR); ;}
    break;

  case 62:
#line 239 "parse.y"
    { parser->emit((yyvsp[(2) - (3)].op)); ;}
    break;

  case 63:
#line 240 "parse.y"
    { parser->emit((yyvsp[(2) - (3)].op)); ;}
    break;

  case 64:
#line 244 "parse.y"
    { (yyval.op) = m8r::Op::STO; ;}
    break;

  case 65:
#line 245 "parse.y"
    { (yyval.op) = m8r::Op::STOMUL; ;}
    break;

  case 66:
#line 246 "parse.y"
    { (yyval.op) = m8r::Op::STODIV; ;}
    break;

  case 67:
#line 247 "parse.y"
    { (yyval.op) = m8r::Op::STOMOD; ;}
    break;

  case 68:
#line 248 "parse.y"
    { (yyval.op) = m8r::Op::STOADD; ;}
    break;

  case 69:
#line 249 "parse.y"
    { (yyval.op) = m8r::Op::STOSUB; ;}
    break;

  case 70:
#line 250 "parse.y"
    { (yyval.op) = m8r::Op::STOSHL; ;}
    break;

  case 71:
#line 251 "parse.y"
    { (yyval.op) = m8r::Op::STOSHR; ;}
    break;

  case 72:
#line 252 "parse.y"
    { (yyval.op) = m8r::Op::STOSAR; ;}
    break;

  case 73:
#line 253 "parse.y"
    { (yyval.op) = m8r::Op::STOAND; ;}
    break;

  case 74:
#line 254 "parse.y"
    { (yyval.op) = m8r::Op::STOXOR; ;}
    break;

  case 75:
#line 255 "parse.y"
    { (yyval.op) = m8r::Op::STOOR; ;}
    break;

  case 80:
#line 269 "parse.y"
    { parser->addVar((yyvsp[(1) - (1)].atom)); ;}
    break;

  case 81:
#line 270 "parse.y"
    { parser->addVar((yyvsp[(1) - (1)].atom)); parser->emit((yyvsp[(1) - (1)].atom)); ;}
    break;

  case 83:
#line 274 "parse.y"
    { parser->emit(m8r::Op::STOPOP); ;}
    break;

  case 88:
#line 282 "parse.y"
    { parser->emit(m8r::Op::POP); ;}
    break;

  case 90:
#line 283 "parse.y"
    { parser->emit(m8r::Op::POP); ;}
    break;

  case 92:
#line 284 "parse.y"
    { parser->emit(m8r::Op::POP); ;}
    break;

  case 113:
#line 333 "parse.y"
    { (yyval.label) = parser->label(); ;}
    break;

  case 115:
#line 337 "parse.y"
    { parser->addVar((yyvsp[(2) - (2)].atom)); parser->emit((yyvsp[(2) - (2)].atom)); ;}
    break;

  case 117:
#line 338 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].atom)); ;}
    break;

  case 121:
#line 361 "parse.y"
    { parser->addMatchedJump(m8r::Op::JF, (yyvsp[(3) - (4)].label)); ;}
    break;

  case 122:
#line 363 "parse.y"
    { parser->matchJump((yyvsp[(3) - (7)].label)); ;}
    break;

  case 124:
#line 366 "parse.y"
    { parser->addMatchedJump(m8r::Op::JF, (yyvsp[(5) - (6)].label)); parser->startDeferred(); ;}
    break;

  case 125:
#line 368 "parse.y"
    { parser->emit(m8r::Op::POP); parser->endDeferred(); ;}
    break;

  case 126:
#line 370 "parse.y"
    { parser->emitDeferred(); parser->matchJump((yyvsp[(5) - (12)].label)); ;}
    break;

  case 129:
#line 376 "parse.y"
    { parser->emitWithCount(m8r::Op::RET, 0); ;}
    break;

  case 130:
#line 377 "parse.y"
    { parser->emitWithCount(m8r::Op::RET, 1); ;}
    break;

  case 131:
#line 380 "parse.y"
    { parser->addNamedFunction((yyvsp[(3) - (3)].function), (yyvsp[(2) - (3)].atom)); ;}
    break;

  case 132:
#line 382 "parse.y"
    { parser->emit((yyvsp[(2) - (2)].function)); ;}
    break;

  case 134:
#line 386 "parse.y"
    { parser->functionAddParam((yyvsp[(1) - (1)].atom)); ;}
    break;

  case 135:
#line 387 "parse.y"
    { parser->functionAddParam((yyvsp[(3) - (3)].atom)); ;}
    break;

  case 136:
#line 391 "parse.y"
    { parser->functionStart(); ;}
    break;

  case 137:
#line 393 "parse.y"
    { parser->functionParamsEnd(); ;}
    break;

  case 138:
#line 395 "parse.y"
    { parser->emit(m8r::Op::END); (yyval.function) = parser->functionEnd(); ;}
    break;

  case 141:
#line 404 "parse.y"
    { parser->emitArrayLiteral(); ;}
    break;

  case 143:
#line 405 "parse.y"
    { parser->emitArrayLiteral(); ;}
    break;

  case 145:
#line 409 "parse.y"
    { parser->emit(m8r::Op::STOA); ;}
    break;

  case 146:
#line 410 "parse.y"
    { parser->emit(m8r::Op::STOA); ;}
    break;

  case 147:
#line 414 "parse.y"
    { parser->emitObjectLiteral(); ;}
    break;

  case 149:
#line 419 "parse.y"
    { parser->emit(m8r::Op::STOO); ;}
    break;

  case 150:
#line 420 "parse.y"
    { parser->emit(m8r::Op::STOO); ;}
    break;

  case 153:
#line 429 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].string)); ;}
    break;

  case 154:
#line 430 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].number)); ;}
    break;

  case 155:
#line 431 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].integer)); ;}
    break;

  case 156:
#line 435 "parse.y"
    { parser->emit((yyvsp[(1) - (1)].atom)); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 2260 "parse.tab.cpp"
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


#line 438 "parse.y"


