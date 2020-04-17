#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "g_parser.h"

typedef struct g_parser
{
    time_t cached_t;
    int cached_year;
    int cached_month;
} g_parser;

g_parser * make_g_parser( void )
{
    g_parser * obj = malloc( sizeof *obj );
    if ( obj != NULL )
    {
        obj -> cached_t = 0;
        obj -> cached_year = 0;
        obj -> cached_month = 0;
    }
    return obj;
}

void destroy_g_parser( g_parser * self )
{
    if ( NULL == self ) return;
    free( ( void * ) self );
}

static void parse_word( const t_str *src, size_t *src_idx, t_parsed * res, char term )
{
    size_t n = 0;
    res -> kind = epk_str;
    t_str * v = &( res -> str );
    v -> p = &( src -> p[ *src_idx ] );
    v -> n = 0;
    while ( *src_idx < src -> n )
    {
        if ( src -> p[ *src_idx ] == term ) break;
        n++;
        (*src_idx)++;
    }
    v -> n = n;
}

static void parse_quoted( g_parser * self, const t_str *src, size_t *src_idx, t_parsed * res, char term )
{
    if ( src -> p[ *src_idx ] == '"' )
    {
        t_parsed dst[ 2 ];
        t_str qsrc = { &( src -> p[ *src_idx ] ), src -> n - *src_idx };
        size_t num_values;
        /* recursivly calling g_parse! - it is save because we do not pass '%t' in format */
        size_t errors = g_parse( self, &qsrc, "\"%s\"", dst, 2, &num_values );
        if ( errors == 0 && num_values == 1 )
        {
            res -> kind = epk_str;
            res -> str = dst[ 0 ] . str;
            *src_idx += ( dst [ 0 ] . str . n + 2 );
        }
        else
        {
            res -> kind = epk_err;
        }
    }
    else
        parse_word( src, src_idx, res, term );
}

static bool parse_uint( const t_str *src, size_t *src_idx, t_parsed * res, char term )
{
    parse_word( src, src_idx, res, term );
    if ( res -> str . n < 1 ) return false;
    bool ok;
    uint64_t v = str_2_u64( &( res -> str ), &ok );
    if ( ok )
    {
        res -> kind = epk_uint;
        res -> u = v;
    }
    else
        res -> kind = epk_err;
    return ok;
}

static bool parse_int( const t_str *src, size_t *src_idx, t_parsed * res, char term )
{
    parse_word( src, src_idx, res, term );
    if ( res -> str . n < 1 ) return false;
    bool ok;
    int64_t v = str_2_i64( &( res -> str ), &ok );
    if ( ok )
    {
        res -> kind = epk_int;
        res -> i = v;
    }
    else
        res -> kind = epk_err;
    return ok;
}

static bool parse_float( const t_str *src, size_t *src_idx, t_parsed * res, char term )
{
    parse_word( src, src_idx, res, term );
    if ( res -> str . n < 1 ) return false;
    res -> kind = epk_float;
    res -> f = strtod( res -> str . p, NULL );
    return true;
}

typedef struct
{
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} t_time_ints;

static time_t get_unix_time( const t_time_ints * time_ints )
{
    struct tm t;
    memset( &t, 0, sizeof( t ) );
    t . tm_sec  = time_ints -> sec; /* 0-59 */
    t . tm_min  = time_ints -> min; /* 0-59 */
    t . tm_hour = time_ints -> hour;  /* 0-23 */
    t . tm_mday = time_ints -> day;   /* 1-31 */
    t . tm_mon  = time_ints -> month - 1; /* 0-11 */
    t . tm_year = time_ints -> year - 1900; /* years since 1900 */
    return mktime( &t );
}

static time_t calc_unix_time( g_parser * self, const t_time_ints * time_ints )
{
    time_t day_ofs  = ( time_ints -> day - 1 ) * ( 24 * 60 * 60 );
    time_t hour_ofs = time_ints -> hour * 60 * 60;
    time_t min_ofs = time_ints -> min * 60;
    return self -> cached_t + day_ofs + hour_ofs + min_ofs + time_ints -> sec;
}

static time_t to_unix_time( g_parser * self, const t_time_ints * time_ints )
{

    if ( NULL == self )
        return get_unix_time( time_ints );

    if ( self -> cached_year != time_ints -> year ||
         self -> cached_month != time_ints -> month )
    {
        t_time_ints t_ym;
        memset( &t_ym, 0, sizeof( t_ym ) );            
        t_ym . year  = time_ints -> year;
        t_ym . month = time_ints -> month;
        t_ym . day   = 1;
        self -> cached_t = get_unix_time( &t_ym );
        self -> cached_year = time_ints -> year;
        self -> cached_month = time_ints -> month;
    }
    return calc_unix_time( self, time_ints );
}

static bool parse_time( g_parser * self, const t_str *src, size_t *src_idx, t_parsed * res, char term )
{
    t_parsed dst[ 8 ];
    size_t num_values, errors;
    t_time_ints time_ints;
    
    parse_word( src, src_idx, res, term );
    if ( res -> str . n < 1 ) return false;
    /* recursivly calling g_parse! - it is save because we do not pass '%t' in format */
    errors = g_parse( self, &( res -> str ), "%u/%s/%u:%u:%u:%u %i", dst, 8, &num_values );
    if ( errors > 0 ) return false;
    if ( num_values < 6 ) return false;
    if ( dst[ 0 ] . kind != epk_uint ) return false;
    time_ints . day = dst[ 0 ] . u;
    if ( time_ints . day < 1 || time_ints . day > 31 ) return false;
    if ( dst[ 1 ] . kind != epk_str ) return false;
    time_ints . month = month_str_2_n( &( dst[ 1 ] . str ) );
    if ( dst[ 2 ] . kind != epk_uint ) return false;
    time_ints . year = dst[ 2 ] . u;
    if ( time_ints . year < 1900 || time_ints . year > 2099 ) return false;
    if ( dst[ 3 ] . kind != epk_uint ) return false;
    time_ints . hour = dst[ 3 ] . u;
    if ( time_ints . hour > 23 ) return false;
    if ( dst[ 4 ] . kind != epk_uint ) return false;
    time_ints . min = dst[ 4 ] . u;
    if ( time_ints . min > 59 ) return false;    
    if ( dst[ 5 ] . kind != epk_uint ) return false;
    time_ints . sec = dst[ 5 ] . u;
    if ( time_ints . sec > 59 ) return false;    

    res -> kind = epk_time;
    res -> u = get_unix_time( &time_ints );
    return true;
}

static void get_leaf( const t_str *src, t_str *dst )
{
    size_t i = 0;
    dst -> p = src -> p;
    dst -> n = 0;
    while( i < src -> n )
    {
        char c = src -> p[ i ];
        if ( c == '?' )
        {
            break;
        }
        else if ( c == '/' )
        {
            dst -> n = 0;
        }
        else
        {
            if ( dst -> n == 0 ) dst -> p = &( src -> p[ i ] );
            dst -> n ++;
        }
        i++;
    }
}

static bool parse_request( g_parser * self, const t_str *src, size_t *src_idx, t_parsed * res, char term )
{
    t_parsed dst[ 4 ];
    size_t num_values, errors;

    parse_word( src, src_idx, res, term );
    if ( res -> str . n < 1 ) return false;
    /* recursivly calling g_parse! - it is save because we do not pass '%r' in format */
    errors = g_parse( self, &( res -> str ), "%s %s %s", dst, 4, &num_values );
    if ( errors > 0 ) return false;
    if ( num_values != 3 ) return false;
    res -> req . method = dst[ 0 ] . str;
    res -> req . path = dst[ 1 ] . str;
    res -> req . vers = dst[ 2 ] . str;
    get_leaf( &( res -> req . path ), &( res -> req . leaf ) );
    res -> kind = epk_req;
    return true;
}

size_t g_parse( g_parser * self, const t_str * src, const char * fmt, t_parsed * dst, size_t dst_size, size_t * num_values )
{
    size_t errors = 0;
    t_str fmtstr;
    size_t src_idx = 0;
    size_t fmt_idx = 0;
    size_t dst_idx = 0;

    if ( NULL == src ) errors++;
    if ( NULL == fmt ) errors++;
    if ( NULL == dst ) errors++;
    if ( 0 == dst_size ) errors++;
    if ( errors > 0 ) return errors;
    if ( 0 == fmt[ 0 ] ) errors++;
    if ( !fill_str( &fmtstr, fmt, 1000 ) ) errors++;
    if ( errors > 0 ) return errors;
    
    if ( NULL != num_values ) *num_values = 0;
    while( src_idx < src -> n && fmt_idx < fmtstr . n )
    {
        char fmt_char = fmtstr . p[ fmt_idx++ ];
        if ( fmt_char == '%' )
        {
            /* special format */
            if ( ( fmt_idx < fmtstr . n ) && ( dst_idx < dst_size ) )
            {
                t_parsed * res = &( dst[ dst_idx ++ ] );
                char term = ' ';
                fmt_char = fmtstr . p[ fmt_idx++ ];
                if ( fmt_idx < fmtstr . n ) term = fmtstr . p[ fmt_idx ];
                switch( fmt_char )
                {
                    case 's' : parse_word( src, &src_idx, res, term ); break;
                    case 'q' : parse_quoted( self, src, &src_idx, res, term ); break;
                    case 'u' : if ( !parse_uint( src, &src_idx, res, term ) ) errors++; break;
                    case 'i' : if ( !parse_int( src, &src_idx, res, term ) ) errors++; break;
                    case 'f' : if ( !parse_float( src, &src_idx, res, term ) ) errors++; break;
                    case 't' : if ( !parse_time( self, src, &src_idx, res, term ) ) errors++; break;
                    case 'r' : if ( !parse_request( self, src, &src_idx, res, term ) ) errors++; break;
                    case '%' : if ( src -> p[ src_idx++ ] != fmt_char ) errors++; break;
                }
                if ( NULL != num_values ) (*num_values)++;
            }
            else
            {
                errors++;
                return errors;
            }
        }
        else
        {
            /* direct match against src */
            if ( src -> p[ src_idx++ ] != fmt_char ) errors++;
        }
    }
    return errors;
}

void print_g_parsed( const t_parsed * parsed, size_t num_parsed )
{
    if ( NULL == parsed ) return;
    if ( 0 == num_parsed ) return;
    for ( size_t i = 0; i < num_parsed; ++i )
    {
        const t_parsed * p = &( parsed[ i ] );
        switch( p -> kind )
        {
            case epk_err   : printf( "[%d] ERROR\n", i ); break;
            case epk_str   : printf( "[%d] <str> : '%.*s'\n", i, p -> str . n, p -> str . p ); break;
            case epk_uint  : printf( "[%d] <uint> : %lu\n", i, p -> u ); break;
            case epk_int   : printf( "[%d] <int> : %ld\n", i, p -> i ); break;
            case epk_float : printf( "[%d] <float> : %f\n", i, p -> f ); break;
            case epk_time  : printf( "[%d] <time> : %lu\n", i, p -> u ); break;
            case epk_req   : printf( "[%d] <req> : str='%.*s' method:'%.*s' path:'%.*s' vers:'%.*s leaf:'%.*s'\n", i,
                                    p -> str . n, p -> str . p,
                                    p -> req . method . n, p -> req . method . p,
                                    p -> req . path . n, p -> req . path . p,
                                    p -> req . vers . n, p -> req . vers . p,
                                    p -> req . leaf . n, p -> req . leaf . p
                                   );
                                    break;
        }
    }
}

/*
 example line:
 158.111.236.250 - - [01/Jan/2020:02:50:24 -0500] "sra-download.ncbi.nlm.nih.gov" "GET /traces/sra34/SRR/003923/SRR4017927 HTTP/1.1" 206 32768 0.000 "-" "linux64 sra-toolkit fastq-dump.2.9.1" "-" port=443 rl=293
 */
bool g_parse_log_data( struct g_parser * self, const char * buff, size_t buff_len, t_log_data * data )
{
    t_str src = { buff, buff_len };
    t_parsed dst[ 12 ];
    size_t num_errors, num_values;
    const char * fmt = "%s - - [%t] %q \"%r\" %u %u %f \"-\" \"%s\" \"-\" port=%u rl=%u\r";
    if ( NULL == buff ) return false;
    if ( NULL == data ) return false;
    if ( 0 == buff_len ) return false;
    if ( 0 == buff[ 0 ] ) return false;

    num_errors = g_parse( self, &src, fmt, dst, sizeof dst / sizeof dst[ 0 ], &num_values );
    if ( num_errors > 0 ) return false;
    //if ( num_values != 10 ) return false;

    /* 0: ip-addr */
    if ( dst[ 0 ] . kind != epk_str ) return false;
    data -> ip = dst[ 0 ] . str;

    /* 1: date, converted to unix-t */
    if ( dst[ 1 ] . kind != epk_time ) return false;
    data -> unix_date = dst[ 1 ] . u;

    /* 2: domain */
    if ( dst[ 2 ] . kind != epk_str ) return false;
    data -> dom = dst[ 2 ] . str;

    /* 3: request */
    if ( dst[ 3 ] . kind != epk_req ) return false;
    data -> req = dst[ 3 ] . str;
    data -> method = dst[ 3 ] . req . method;
    data -> path = dst[ 3 ] . req . path;
    data -> vers = dst[ 3 ] . req . vers;
    data -> acc = dst[ 3 ] . req . leaf;

    /* 4: result-code */
    if ( dst[ 4 ] . kind != epk_uint ) return false;
    data -> res = dst[ 4 ] . u;

    /* 5: bytes returned */
    if ( dst[ 5 ] . kind != epk_uint ) return false;
    data -> num_bytes = dst[ 5 ] . u;

    /* 6: factor */
    if ( dst[ 6 ] . kind != epk_float ) return false;
    data -> factor = dst[ 6 ] . f;

    /* 7: user-agent */
    if ( dst[ 7 ] . kind != epk_str ) return false;
    data -> agnt = dst[ 7 ] . str;

    /* 8: port */
    if ( dst[ 8 ] . kind != epk_uint ) return false;
    data -> port = dst[ 8 ] . u;

    /* 9: request-length */
    if ( dst[ 9 ] . kind != epk_uint ) return false;
    data -> req_len = dst[ 9 ] . u;

    return true;
}

void print_data_tsv( t_log_data * data )
{
    const char * fmt = "%.*s\t%u\t%.*s\t%.*s\t%u\t%u\t%.*s\t%u\t%u\n";
    if ( NULL == data ) return;
    printf( fmt,
            data -> ip . n, data -> ip . p,
            data -> unix_date,
            data -> dom . n, data -> dom . p,
            data -> req . n, data -> req . p,
            data -> res,
            data -> num_bytes,
            data -> agnt . n, data -> agnt . p,
            data -> port,
            data -> req_len );
}

void print_data_dbg( t_log_data * data )
{
    if ( NULL == data ) return;
    printf( "ip:\t'%.*s'\n", data -> ip . n, data -> ip . p );
    printf( "t:\t%u\n", data -> unix_date );
    printf( "dom:\t'%.*s'\n", data -> dom . n, data -> dom . p );
    printf( "req:\t'%.*s'\n", data -> req . n, data -> req . p );
    printf( "method:\t'%.*s'\n", data -> method . n, data -> method . p );
    printf( "path:\t'%.*s'\n", data -> path . n, data -> path . p );
    printf( "acc:\t'%.*s'\n", data -> acc . n, data -> acc . p );
    printf( "vers:\t'%.*s'\n", data -> vers . n, data -> vers . p );
    printf( "res:\t%u\n", data -> res );
    printf( "bytes:\t%u\n", data -> num_bytes );
    printf( "factor:\t%f\n", data -> factor );
    printf( "agnt:\t'%.*s'\n", data -> agnt . n, data -> agnt . p );
    printf( "port:\t%u\n", data -> port );
    printf( "req-len:\t%u\n\n", data -> req_len );
}
