%define api.pure full
%lex-param { void * scanner }
%parse-param { void * scanner }{ NCBI::Logging::GCP_LogLines * lib }

%define parse.trace
%define parse.error verbose

%name-prefix "gcp_"

%{
#define YYDEBUG 1

#include <stdint.h>
#include "parser-functions.h"
#include "log_lines.hpp"
#include "gcp_parser.hpp"
#include "gcp_scanner.hpp"
#include "helper.hpp"

using namespace std;
using namespace NCBI::Logging;

void gcp_error( yyscan_t locp, NCBI::Logging::GCP_LogLines * lib, const char* msg );

%}

%code requires
{
#include "types.h"
#include "log_lines.hpp"

extern void gcp_get_scanner_input( void * yyscanner, t_str & str );
extern void gcp_start_URL( void * yyscanner );
extern void gcp_stop_URL( void * yyscanner );

using namespace NCBI::Logging;

}

%union
{
    t_str s;
    t_request req;
}

%token<s> IPV4 IPV6 QSTR I64 PATHSTR PATHEXT ACCESSION SLASH
%token<s> EQUAL AMPERSAND QMARK PERCENT
%token QUOTE COMMA UNRECOGNIZED

%type<s> ip ip_region method host referrer agent 
%type<s> ext_opt file_opt url_token
%type<s> req_id operation bucket hdr_item hdr_item_text
%type<s> q_i64 time ip_type status req_bytes res_bytes time_taken
%type<req> object uri url_list url_list_comp

%start line

%%

line
    :
    | log_gcp       { YYACCEPT; }
    | log_hdr       { lib -> headerLine(); YYACCEPT; }
    | log_err       { YYACCEPT; }
    ;

log_err
    : time error
    {
        LogGCPEvent ev;
        ev . time = ( $1 . p == nullptr ) ? 0 : atol( $1 . p );
        gcp_get_scanner_input( scanner, ev . unparsed );
        lib -> rejectLine( ev );
    }
    ;

log_hdr
    : hdr_item_text             {} 
    | log_hdr COMMA hdr_item    {} 
    ;

hdr_item_text
    : QUOTE QSTR QUOTE   {}   
    | QUOTE QUOTE        {} 
    ;

hdr_item
    : QUOTE QSTR QUOTE   {} 
    | QUOTE I64 QUOTE    {} 
    | QUOTE QUOTE        {} 
    ;

log_gcp
    : time COMMA ip COMMA ip_type COMMA ip_region COMMA method COMMA
      { gcp_start_URL( scanner ); } 
      uri
      { gcp_stop_URL( scanner ); }
      COMMA status COMMA req_bytes COMMA res_bytes COMMA time_taken COMMA host COMMA
      referrer COMMA agent COMMA req_id COMMA operation COMMA bucket COMMA 
      { gcp_start_URL( scanner ); } 
      object
      { gcp_stop_URL( scanner ); } 
    {
        LogGCPEvent ev;
        ev . time = ( $1 . p == nullptr ) ? 0 : atol( $1 . p );
        ev . ip = $3;
        ev . ip_type = ( $5 . p == nullptr ) ? 0 : atoi( $5 . p );
        ev . ip_region = $7;
        // $9 (method) goes into ev.request below
        ev . uri = $12 . path;
        ev . status = ( $15 . p == nullptr ) ? 0 : atol( $15 . p );
        ev . request_bytes = ( $17 . p == nullptr ) ? 0 : atol( $17 . p );
        ev . result_bytes = ( $19 . p == nullptr ) ? 0 : atol( $19 . p );
        ev . time_taken = ( $21 . p == nullptr ) ? 0 : atol( $21 . p );
        ev . host = $23;
        ev . referer = $25;
        ev . agent = $27;
        ev . request_id = $29;
        ev . operation = $31;
        ev . bucket = $33;

        if ( $36 . path . n > 0 )
        {
            // the cloud did populate the object-field
            ev . request = $36;
        }
        else
        {
            // accession, file-name and extension may be found in parameters of uri ($12)
            ev . request = $12;
        }
        ev . request . method = $9;

        lib -> acceptLine( ev );
    }
    ;

time
    : QUOTE I64 QUOTE       { $$ = $2; } /* if empty, would conflict with the log_hdr rule */
    ;

ip
    : QUOTE IPV4 QUOTE      { $$ = $2; }
    | QUOTE IPV6 QUOTE      { $$ = $2; }
    | QUOTE QUOTE           { EMPTY_TSTR($$); }
    ;

ip_type
    : q_i64
    ;

ip_region
    : QUOTE QSTR QUOTE      { $$ = $2; }
    | QUOTE QUOTE           { EMPTY_TSTR($$); }
    ;

method
    : QUOTE QSTR QUOTE      { $$ = $2; }
    ;

uri
    : QUOTE url_list_comp QUOTE
    {
        $$ = $2;
    }
    | QUOTE QUOTE
    {
        InitRequest( $$ );
    }
    ;

status
    : q_i64
    ;

req_bytes
    : q_i64
    ;

res_bytes
    : q_i64
    ;

time_taken
    : q_i64
    ;

host
    : QUOTE QSTR QUOTE      { $$ = $2; }
    | QUOTE QUOTE           { EMPTY_TSTR($$); }
    ;

referrer
    : QUOTE QSTR QUOTE      { $$ = $2; }
    | QUOTE QUOTE           { EMPTY_TSTR($$); }
    ;

agent
    : QUOTE QSTR QUOTE      { $$ = $2; }
    | QUOTE QUOTE           { EMPTY_TSTR($$); }
    ;

req_id
    : QUOTE QSTR QUOTE      { $$ = $2; }
    | QUOTE I64 QUOTE       { $$ = $2; }
    | QUOTE QUOTE           { EMPTY_TSTR($$); }
    ;

operation
    : QUOTE QSTR QUOTE      { $$ = $2; }
    | QUOTE QUOTE           { EMPTY_TSTR($$); }
    ;

bucket
    : QUOTE QSTR QUOTE      { $$ = $2; }
    | QUOTE QUOTE           { EMPTY_TSTR($$); }
    ;

ext_opt
    : PATHEXT   { $$ = $1; }
    |           { EMPTY_TSTR($$); }
    ;

file_opt
    : ACCESSION { $$ = $1; }
    | PATHSTR   { $$ = $1; }
    |           { EMPTY_TSTR($$); }
    ;

url_list_comp
    : url_list
    {
        $$ = $1;
    }
    | url_list ACCESSION
    {
        $$ = $1;
        MERGE_TSTR( $$ . path, $2 );
        $$ . accession = $2;
    }
    | url_list ACCESSION url_list_tail
    {
        $$ = $1;
        MERGE_TSTR( $$ . path, $2 );
        MERGE_TSTR( $$ . path, $3 . path );        
        $$ . accession = $2;
        // TODO: catch filename/extension
    }
    ;

url_list
    : url_token
    {
        InitRequest( $$ );
        $$ . path = $1;
    }
    | url_list url_token
    {
        $$ = $1;
        MERGE_TSTR( $$ . path, $2 );
    }
    ;

url_list_tail
    : this is a collection of url tokens and accessions
    TODO: we are looking for the file-name and the extension.

url_token
    : SLASH         { $$ = $1; }
    | PATHSTR       { $$ = $1; }
    | PATHEXT       { $$ = $1; }
    | EQUAL         { $$ = $1; }
    | AMPERSAND     { $$ = $1; }
    | QMARK         { $$ = $1; }
    | PERCENT       { $$ = $1; }
    ;

object
    : QUOTE ACCESSION SLASH file_opt ext_opt QUOTE 
        { 
            InitRequest( $$ );
             
            $$.path . p = $2 . p;
            $$.path . n = $2 . n + 1 + $4 . n + $5 . n;

            $$.accession = $2;
            $$.filename  = $4;
            $$.extension = $5;
        }
    | QUOTE url_list QUOTE
        { 
            $$ = $2;
        }
    | QUOTE QUOTE 
        { 
             InitRequest( $$ );
        }
    ;

q_i64
    : QUOTE I64 QUOTE       { $$ = $2; }
    | QUOTE QUOTE           { EMPTY_TSTR($$); }
    ;

%%

void gcp_error( yyscan_t locp, NCBI::Logging::GCP_LogLines * lib, const char * msg )
{
    // intentionally left empty, we communicate errors rejected lines
}
