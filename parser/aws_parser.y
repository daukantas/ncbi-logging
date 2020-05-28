%define api.pure full
%lex-param { void * scanner }
%parse-param { void * scanner }{ NCBI::Logging::AWS_LogLines * lib }

%define parse.trace
%define parse.error verbose

%name-prefix "aws_"

%{
#define YYDEBUG 1

#include <stdint.h>

#include "log_lines.hpp"
#include "aws_parser.hpp"
#include "aws_scanner.hpp"
#include "helper.hpp"

using namespace std;
using namespace NCBI::Logging;

void aws_error( yyscan_t locp, NCBI::Logging::AWS_LogLines * lib, const char* msg );

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
    t_timepoint tp;
}

%token<s> STR STR1 MONTH IPV4 IPV6 FLOAT METHOD VERS QSTR DASH I64
%token DOT SLASH COLON QUOTE OB CB PORT RL CR LF SPACE QMARK

%type<tp> time
%type<s> ip referer agent agent_list params_opt request
%type<s> aws_owner aws_bucket aws_requester aws_request_id aws_operation aws_key aws_error
%type<s> aws_version_id aws_host_id aws_cipher aws_auth aws_host_hdr aws_tls_vers
%type<s> result_code aws_bytes_sent aws_obj_size aws_total_time aws_turnaround_time

%start line

%%

line
    : log_aws       { return 0; }
    ;

log_aws
    : aws_owner SPACE
      aws_bucket SPACE
      time SPACE
      ip SPACE
      aws_requester SPACE
      aws_request_id SPACE
      aws_operation SPACE
      aws_key SPACE
      request SPACE
      result_code SPACE
      aws_error SPACE
      aws_bytes_sent SPACE
      aws_obj_size SPACE
      aws_total_time SPACE
      aws_turnaround_time SPACE
      referer SPACE
      agent SPACE
      aws_version_id SPACE
      aws_host_id SPACE
      aws_cipher SPACE
      aws_auth SPACE
      aws_host_hdr SPACE
      aws_tls_vers
    {
        LogAWSEvent ev;
        ev . owner = $1;
        ev . bucket = $3;
        ev . time = $5;
        ev . ip = $7;
        ev . requester = $9;
        ev . request_id = $11;
        ev . operation = $13;
        ev . key = $15;
        ev . request = $17;
        ev . res_code = $19;
        ev . error = $21;
        ev . res_len = $23;
        ev . obj_size = $25;
        ev . total_time = $27;
        ev . turnaround_time = $29;
        ev . referer = $31;
        ev . agent = $33;
        ev . version_id = $35;
        ev . host_id = $37;
        ev . cipher_suite = $39;
        ev . auth_type = $41;
        ev . host_header = $43;
        ev . tls_version = $45;
        lib -> acceptLine( ev );
    }
    ;

aws_owner
    : STR
    | STR1
    ;

aws_bucket
    : STR
    ;

aws_requester
    : STR
    | STR1
    | DASH                          { $$.p = NULL; $$.n = 0; }
    ;

aws_request_id
    : STR
    | STR1
    ;

aws_operation
    : STR
    | STR1
    ;

aws_key
    : STR
    | STR1
    | DASH                          { $$.p = NULL; $$.n = 0; }
    ;

aws_error
    : DASH                          { $$.p = NULL; $$.n = 0; }
    | STR
    | STR1
    ;

aws_bytes_sent
    : STR
    | DASH                          { $$.p = NULL; $$.n = 0; }
    ;

aws_obj_size
    : STR
    | DASH                          { $$.p = NULL; $$.n = 0; }
    ;

aws_total_time
    : STR
    | DASH                          { $$.p = NULL; $$.n = 0; }
    ;

aws_turnaround_time
    : STR
    | DASH                          { $$.p = NULL; $$.n = 0; }
    ;

aws_version_id
    : STR
    | STR1
    | DASH                          { $$.p = NULL; $$.n = 0; }
    ;

aws_host_id
    : STR
    | STR1
    ;

aws_cipher
    : STR SPACE STR    { $$ = $1; $$.n += ( $3.n + 1 ); }
    | STR1 SPACE STR   { $$ = $1; $$.n += ( $3.n + 1 ); }
    | STR SPACE STR1   { $$ = $1; $$.n += ( $3.n + 1 ); }
    | STR1 SPACE STR1  { $$ = $1; $$.n += ( $3.n + 1 ); }   
    | DASH             { $$.p = NULL; $$.n = 0; } 
    ;

aws_auth
    : STR
    ;

aws_host_hdr
    : STR
    | STR1
    | DASH                          { $$.p = NULL; $$.n = 0; }
    ;

aws_tls_vers
    : STR
    | STR1
    ;

ip
    : IPV4
    | IPV6
    ;

params_opt
    : QMARK QSTR    { $$ = $2; }
    | %empty        { $$.p = NULL; $$.n = 0; }
    ;

request
    : QUOTE QSTR QUOTE { $$ = $2; }
    ;

result_code
    : STR
    ;

referer
    : QUOTE QSTR QUOTE              { $$ = $2; }
    | DASH                          { $$.p = NULL; $$.n = 0; }
    | STR
    | STR1
    ;

agent
    : QUOTE agent_list QUOTE        { $$ = $2; }
    ;

agent_list
    : QSTR                          { $$ = $1; }
    | agent_list QSTR               { $$.n += $2.n; $$.escaped = $1.escaped || $2.escaped; }
    | agent_list SPACE              { $$.n += 1;    $$.escaped = $1.escaped; }    
    ;

time
    : OB I64 SLASH MONTH SLASH I64 COLON I64 COLON I64 COLON I64 I64 CB
    {
        $$.day = atoi( $2.p );
        $$.month = month_int( &($4) );
        $$.year = atoi( $6.p );
        $$.hour = atoi( $8.p );
        $$.minute = atoi( $10.p );
        $$.second = atoi( $12.p );
        $$.offset = atoi( $13.p );
    }
    ;

%%

void aws_error( yyscan_t locp, NCBI::Logging::AWS_LogLines * lib, const char * msg )
{
    // TODO: find a way to comunicate the syntax error to the consumer...
    // t_str msg_p = { msg, (int)strlen( msg ) };
    // lib -> unrecognized( msg_p );
}
