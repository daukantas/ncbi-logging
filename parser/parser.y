%define api.pure full
%lex-param { void * scanner }
%parse-param { void * scanner }{ NCBI::Logging::LogLines * lib }

%define parse.trace
%define parse.error verbose

%name-prefix "log1_"

%{
#define YYDEBUG 1

#include <stdint.h>
#include "parser-functions.h"
#include "log_lines.hpp"
#include "log1_parser.hpp"
#include "log1_scanner.hpp"

using namespace std;
using namespace NCBI::Logging;

void log1_error( yyscan_t locp, NCBI::Logging::LogLines * lib, const char* msg );

uint8_t month_int( const t_str * s );

string ToString( const t_str & in )
{
    return string( in.p == nullptr ? "" : in.p, in.n );
}

%}

%code requires
{
#include "types.h"
#include "log_lines.hpp"
using namespace NCBI::Logging;
}

%union
{
    t_str s;
    int64_t i64;
    t_timepoint tp;
    t_request req;
}

%token<s> STR MONTH IPV4 IPV6 FLOAT METHOD VERS QSTR
%token<i64> I64
%token DOT DASH SLASH COLON QUOTE OB CB PORT RL CR LF SPACE QMARK

%type<tp> time
%type<req> request
%type<s> ip user req_time referer agent agent_list forwarded params_opt
%type<i64> result_code result_len port req_len

%start line

%%

line
    : log_v1
    | error
    ;

log_v1
    : ip DASH user time request result_code result_len req_time referer agent forwarded port req_len
    {
        LogEvent ev;
        ev.ip = ToString( $1 );
        ev . user = ToString( $3 );
        ev . time = $4;
        ev . request = $5;
        ev . res_code = $6;
        ev . res_len = $7;
        ev . req_time = ToString( $8 );
        ev . referer = ToString( $9 );
        ev . agent = ToString( $10 );
        ev . forwarded = ToString( $11 );
        ev . port = $12;
        ev . req_len = $13;

        lib->acceptLine(ev);
    }
    ;

ip
    : IPV4
    | IPV6
    ;

user
    : DASH                          { $$.p = NULL; $$.n = 0;}
    | STR                           { $$ = $1; }
    ;

params_opt
    : QMARK QSTR    { $$ = $2; }
    | %empty        { $$.p = NULL; $$.n = 0; }
    ;

request :
    QUOTE QSTR QUOTE QUOTE METHOD SPACE QSTR params_opt SPACE VERS QUOTE
    {
        $$.server = $2;
        $$.method = $5;
        $$.path   = $7;
        $$.params = $8;
        $$.vers   = $10;
    }
    |
    STR QUOTE METHOD SPACE QSTR params_opt SPACE VERS QUOTE
    {
        $$.server = $1;
        $$.method = $3;
        $$.path   = $5;
        $$.params = $6;
        $$.vers   = $8;
    }
    |
    QUOTE METHOD SPACE QSTR params_opt SPACE VERS QUOTE
    {
        $$.server.p = NULL; $$.server.n = 0;
        $$.method = $2;
        $$.path   = $4;
        $$.params = $5;
        $$.vers   = $7;
    }
    ;

result_code
    : I64
    ;

result_len
    : I64
    ;

req_time
    : FLOAT
    ;

referer
    : QUOTE QSTR QUOTE              { $$ = $2; }
    ;

agent
    : QUOTE agent_list QUOTE        { $$ = $2; }
    ;

agent_list
    : QSTR                          { $$ = $1; }
    | agent_list QSTR               { $$.n += $2.n; }
    | agent_list SPACE              { $$.n += 1; }
    ;

forwarded
    : QUOTE QSTR QUOTE              { $$ = $2; }
    ;

port
    : PORT I64                      { $$ = $2; }
    ;

req_len
    : RL I64                        { $$ = $2; }
    ;

time
    : OB I64 SLASH MONTH SLASH I64 COLON I64 COLON I64 COLON I64 I64 CB
    {
        $$.day = $2;
        $$.month = month_int( &($4) );
        $$.year = $6;
        $$.hour = $8;
        $$.minute = $10;
        $$.second = $12;
        $$.offset = $13;
    }
    ;

%%

void log1_error( yyscan_t locp, NCBI::Logging::LogLines * lib, const char * msg )
{
    lib->unrecognized( string( msg ) );
    //TODO: use locp
}

uint8_t month_int( const t_str * s )
{
    char s0 = s -> p[ 0 ];
    char s1 = s -> p[ 1 ];
    char s2 = s -> p[ 2 ];

    if ( s -> n != 3 ) return 0;

    if ( s0 == 'A' ) /* Apr, Aug */
    {
        if ( s1 == 'p' && s2 == 'r' ) return 4;
        if ( s1 == 'u' && s2 == 'g' ) return 8;
    }
    else if ( s0 == 'D' ) /* Dec */
    {
        if ( s1 == 'e' && s2 == 'c' ) return 12;
    }
    else if ( s0 == 'F' ) /* Feb */
    {
        if ( s1 == 'e' && s2 == 'b' ) return 2;
    }
    else if ( s0 == 'J' ) /* Jan, Jun, Jul */
    {
        if ( s1 == 'a' && s2 == 'n' ) return 1;
        if ( s1 == 'u' )
        {
            if ( s2 == 'n' ) return 6;
            if ( s2 == 'l' ) return 7;
        }
    }
    else if ( s0 == 'M' ) /* Mar, May */
    {
        if ( s1 == 'a' )
        {
            if ( s2 == 'r' ) return 3;
            if ( s2 == 'y' ) return 5;
        }
    }
    else if ( s0 == 'N') /* Nov */
    {
        if ( s1 == 'o' && s2 == 'v' ) return 11;
    }
    else if ( s0 == 'O' ) /* Oct */
    {
        if ( s1 == 'c' && s2 == 't' ) return 10;
    }
    else if ( s0 == 'S' ) /* Sep */
    {
        if ( s1 == 'e' && s2 == 'p' ) return 9;
    }
    return 0;
}