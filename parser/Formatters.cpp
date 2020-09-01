#include "Formatters.hpp"

#include "Queues.hpp"
#include <utf8proc.h>

#include <sstream>
#include <algorithm>

using namespace std;
using namespace NCBI::Logging;
using namespace ncbi;

FormatterInterface::~FormatterInterface()
{
}

string &
FormatterInterface::unEscape( const t_str & value, std::string & str ) const
{
    return str;
}

JsonLibFormatter::JsonLibFormatter()
: j ( JSON::makeObject() )
{
}

JsonLibFormatter::~JsonLibFormatter()
{
}

string
JsonLibFormatter::format()
{
    stringstream out;
    out << j->toJSON().toSTLString();
    j = JSON::makeObject();
    return out.str();
}

JSONValueRef ToJsonString( const t_str & in )
{
    if ( in . n > 0 )
    {
        return JSON::makeString( String( in . p, in . n) );
    }
    else
    {
        return JSON::makeString( String () );
    }
}

void
JsonLibFormatter::addNameValue( const std::string & name, const t_str & value )
{
    j -> addValue( String ( name.c_str(), name.size() ), ToJsonString ( value ) );
}

void
JsonLibFormatter::addNameValue( const std::string & name, int64_t value )
{
    j -> addValue( String ( name.c_str(), name.size() ), JSON::makeInteger( value ) );
}

void
JsonLibFormatter::addNameValue( const std::string & name, const std::string & value )
{
    j -> addValue( String ( name.c_str(), name.size() ), JSON::makeString( String( value.c_str(), value.size() ) ) );
}

/* ------------------------------------------------------------------------------------------ */

std::ostream& operator<< (std::ostream& os, const t_str & s)
{
    os . put ( '"' );

    const utf8proc_uint8_t * str = ( const utf8proc_uint8_t * ) s.p;
    utf8proc_int32_t ch;
    utf8proc_ssize_t len;
    utf8proc_ssize_t remain = s.n;
    while ( remain > 0 && ( len = utf8proc_iterate( str, remain, & ch ) ) )
    {
        if ( utf8proc_codepoint_valid ( ch ) )
        {
            switch ( ch )
            {
            case '\\':
            case '\"':
                os . put( '\\' );
                os . put( ch );
                break;
            case '\b':
                os . write( "\\b", 2 );
                break;
            case '\t':
                os . write( "\\t", 2 );
                break;
            case '\n':
                os . write( "\\n", 2 );
                break;
            case '\f':
                os . write( "\\f", 2 );
                break;
            case '\r':
                os . write( "\\r", 2 );
                break;
            default:
                if ( ch >= 0 && ch < 32 )
                {
                    stringstream tmp;
                    tmp << "\\u";
                    tmp << std::hex << std::setfill('0') << std::setw(4);
                    tmp << ch;
                    os . write( tmp.str().c_str(), tmp.str().size() );
                }
                else
                {
                    utf8proc_uint8_t out[4];
                    utf8proc_ssize_t size = utf8proc_encode_char( ch, out );
                    os . write( (const char*)out, size );
                }

                break;
            }
        }
        else
        {
            throw InvalidUTF8String ( XP ( XLOC ) << "badly formed UTF-8 character" );
        }

        str += len;
        remain -= len;
    }

    // ncbi::String k( s.p, s.n ); // throws if invalid UTF8; escapes control characters
    // ncbi::StringBuffer out;

    // ncbi::String::Iterator iter( k.makeIterator() );
    // while( iter.isValid() )
    // {
    //     auto ch = iter.get();
    //     switch ( ch )
    //     {
    //     case '\\':
    //     case '\"':
    //         out . append( '\\' );
    //         break;
    //     default:
    //         break;
    //     }
    //     out . append( ch );
    //     ++iter;
    // }
    // os . write( out.data(), out.size() );

    os . put ( '"' );
    return os;
}

JsonFastFormatter::~JsonFastFormatter()
{

}

string
JsonFastFormatter::format()
{
    stringstream s;
    s . put ( '{' );
    std::sort( kv.begin(), kv.end() );
    bool first = true;
    for( auto& item : kv )
    {
        if ( first )
            first = false;
        else
            s . put( ',' );
        s . write( item.c_str(), item.size() );
    }
    kv.clear();
    s . put ( '}' );
    return s.str();
}

void JsonFastFormatter::addNameValue( const std::string & name, const t_str & value )
{
    ss.str( "" );
    ss . put( '"' );
    ss . write( name.c_str(), name.size() );
    ss . put( '"' );
    ss . put( ':' );
    ss << value;    /* here exception can happen */
    kv.push_back( ss.str() );
}

void JsonFastFormatter::addNameValue( const std::string & name, int64_t value )
{
    ss.str( "" );
    ss . put( '"' );
    ss . write( name.c_str(), name.size() );
    ss . put( '"' );
    ss . put( ':' );
    ss << value;
    kv.push_back( ss.str() );
}

void JsonFastFormatter::addNameValue( const std::string & name, const std::string & value )
{
    t_str tmp { value.c_str(), value.size(), false };
    addNameValue( name, tmp );
}

