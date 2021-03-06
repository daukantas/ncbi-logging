#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "log_lines.hpp"
#include "helper.hpp"
#include "test_helper.hpp"

using namespace std;
using namespace NCBI::Logging;

struct SLogGCPEvent
{
    int64_t     time;
    SRequest    request;
    string      ip;
    int64_t     ip_type;
    string      ip_region;
    string      uri;
    int64_t     status;
    int64_t     request_bytes;
    int64_t     result_bytes;
    int64_t     time_taken;
    string      host;
    string      referer;
    SAgent      agent;
    string      request_id;
    string      operation;
    string      bucket;
    string      unparsed;

    SLogGCPEvent( const LogGCPEvent &ev )
    {
        time        = ev . time;
        request     = ev . request;
        ip          = ToString( ev . ip );
        ip_type     = ev . ip_type;
        ip_region   = ToString( ev . ip_region );
        uri         = ToString( ev . uri );
        status      = ev . status;
        request_bytes = ev . request_bytes;
        result_bytes = ev . result_bytes;
        time_taken  = ev. time_taken;
        host        = ToString( ev . host );
        referer     = ToString( ev . referer );
        agent       = ev . agent;
        request_id  = ToString( ev . request_id );
        operation   = ToString( ev . operation );
        bucket      = ToString( ev . bucket );
        unparsed    = ToString( ev . unparsed );
    }
};

struct TestLogLines : public GCP_LogLines
{
    virtual void unrecognized( const t_str & text )
    {
        m_unrecognized . push_back( ToString( text ) );
    }

    virtual void acceptLine( const LogGCPEvent & event )
    {
        m_accepted . push_back ( SLogGCPEvent( event ) );
    }

    virtual void rejectLine( const LogGCPEvent & event )
    {
        m_rejected . push_back ( SLogGCPEvent( event ) );
    }

    virtual void headerLine()
    {
    }

    vector< string > m_unrecognized;
    vector < SLogGCPEvent > m_accepted;
    vector < SLogGCPEvent > m_rejected;    

    virtual ~TestLogLines()
    {
    }

};

TEST ( TestParse, InitDestroy )
{
    TestLogLines lines;
    GCP_Parser p( lines, cin );
}

TEST ( TestParse, LineRejecting )
{
    const char * input = "\"1\"\n\"2\"\n\"3\"\n";
    std::istringstream inputstream( input );
    TestLogLines lines;
    GCP_Parser p( lines, inputstream );
    p.parse();
    ASSERT_EQ( 3, lines.m_rejected.size() );
}

TEST ( TestParse, LineFilter )
{
    const char * input = "\"1\"\n\"2\"\n\"3\"\n";
    std::istringstream inputstream( input );
    TestLogLines lines;
    GCP_Parser p( lines, inputstream );
    p . setLineFilter( 2 );
    p . parse();
    ASSERT_EQ( 1, lines . m_rejected.size() );
    ASSERT_EQ( "\"2\"", lines . m_rejected . front() . unparsed );
}

class TestParseFixture : public ::testing::Test
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}

    SLogGCPEvent parse_gcp( const char * input, bool p_debug = false )
    {
        std::istringstream inputstream( input );
        {
            GCP_Parser p( m_lines, inputstream );
            p.setDebug( p_debug );
            p.parse();
            if ( m_lines.m_accepted.empty() ) throw logic_error( "last_m_accepted is null" );
            return m_lines . m_accepted.back();
        }
    }

    TestLogLines m_lines;
};

TEST_F ( TestParseFixture, Empty )
{
    std::istringstream input ( "" );
    GCP_Parser( m_lines, input ).parse();
}

// GCP
TEST_F ( TestParseFixture, GCP )
{
    const char * InputLine =
    "\"1589759989434690\","
    "\"35.245.177.170\","
    "\"1\","
    "\"\","
    "\"GET\","
    "\"/storage/v1/b/sra-pub-src-9/o/SRR1371108%2FCGAG_2.1.fastq.gz?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\","
    "\"404\","
    "\"0\","
    "\"291\","
    "\"27000\","
    "\"www.googleapis.com\","
    "\"\","
    "\"apitools gsutil/4.37 Python/2.7.13 (linux2) \\\"google\\\"-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\","
    "\"AAANsUmaKBTw9gqOSHDOdr10MW802XI5jlNu87rTHuxhlRijModRQnNlwOd-Nxr0EHWq4iVXXEEn9LW4cHb7D6VK5gs\","
    "\"storage.objects.get\","
    "\"sra-pub-src-9\","
    "\"SRR1371108/CGAG_2.1.fastq.gz\""
    "\n";

    SLogGCPEvent e = parse_gcp( InputLine );

    ASSERT_EQ( 1589759989434690, e.time );
    ASSERT_EQ( "35.245.177.170", e.ip );
    ASSERT_EQ( 1, e.ip_type );
    ASSERT_EQ( "", e.ip_region );
    ASSERT_EQ( "GET", e.request.method );
    ASSERT_EQ( "/storage/v1/b/sra-pub-src-9/o/SRR1371108%2FCGAG_2.1.fastq.gz?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl", e . uri );
    ASSERT_EQ( 404, e . status );
    ASSERT_EQ( 0, e . request_bytes );
    ASSERT_EQ( 291, e . result_bytes );
    ASSERT_EQ( 27000, e . time_taken );
    ASSERT_EQ( "www.googleapis.com", e . host );
    ASSERT_EQ( "", e . referer );
    ASSERT_EQ( "apitools gsutil/4.37 Python/2.7.13 (linux2) \\\"google\\\"-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)", e . agent . original);
    ASSERT_EQ( "AAANsUmaKBTw9gqOSHDOdr10MW802XI5jlNu87rTHuxhlRijModRQnNlwOd-Nxr0EHWq4iVXXEEn9LW4cHb7D6VK5gs", e . request_id );
    ASSERT_EQ( "storage.objects.get", e . operation );
    ASSERT_EQ( "sra-pub-src-9", e . bucket );
    ASSERT_EQ( "SRR1371108/CGAG_2.1.fastq.gz", e . request . path );
    ASSERT_EQ( "SRR1371108", e . request . accession );
    ASSERT_EQ( "CGAG_2", e . request . filename );
    ASSERT_EQ( ".1.fastq.gz", e . request . extension );
}

TEST_F ( TestParseFixture, GCP_EmptyAgent )
{
    const char * InputLine =
    "\"1591118933830501\",\"35.202.252.53\",\"1\",\"\",\"GET\",\"/sra-pub-run-8/SRR10303547/SRR10303547.1?GoogleAccessId=data-access-service%40nih-sra-datastore.iam.gserviceaccount.com&Expires=1591478828&userProject=nih-sra-datastore&Signature=ZNzj62MP4PWwSVvtkmsB97Lu33wQq4cFGLyWRJTcb%2F8h1BvVXi3lokOoT16ihScR%0At2EHti%2FgQ80VVMv9BGpAY%2FQ9HTqXeq57N53tMcjXQQMKFVttyXgIW89OLWO0UC0h%0AZdFq5AcKZywgnZql8z3RoaQi%2FPKdrdMO803tW%2Bxe%2Boy8sCd%2FyCXcG9jBrkGbdqPc%0A3xnyuycW1Va4LHIh4muGGdFSIqBk7oaLjkjLV54L8e4InzFMD3Kx0Q5raIlNadxx%0AIX%2B2hoPJuCdSh6IxEikrvUri%2Fd6i9Nqo%2BkZ%2BPSGtvlah9I9AafXrs3EAlwZkvc%2Bp%0AnjssKH8zalZQ5SmPpfHImQ%3D%3D%0A\",\"206\",\"0\",\"262144\",\"31000\",\"storage.googleapis.com\",\"\",\"\",\"AAANsUnhN-04LMubyLe-H4MQzIYbbqFwxT85S0jQpptnyQoxcHZP2JsKPbvPI9OK7TJkHIZRBcc4vvt6atty7aj6UoY\",\"GET_Object\",\"sra-pub-run-8\",\"SRR10303547/SRR10303547.1\""
    "\n";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "", e.agent.original );
    ASSERT_EQ( "", e.agent.vdb_os );
    ASSERT_EQ( "", e.agent.vdb_tool );
    ASSERT_EQ( "", e.agent.vdb_release );
    ASSERT_EQ( "", e.agent.vdb_phid_compute_env );
    ASSERT_EQ( "", e.agent.vdb_phid_guid );
    ASSERT_EQ( "", e.agent.vdb_phid_session_id );
    ASSERT_EQ( "", e.agent.vdb_libc );
}

TEST_F ( TestParseFixture, GCP_EmptyIP_EmptyURI )
{
    const char * InputLine =
    "\"1591062496651000\",\"\",\"\",\"\",\"PUT\",\"\",\"200\",\"\",\"\",\"\",\"\",\"\",\"GCS Lifecycle Management\",\"02478f5fe3deaa079d1a4f6033043a80\",\"PUT_Object\",\"sra-pub-src-3\",\"SRR1269314/s_3_11000.p1_export.txt.gzb_fixed.bam\""
    "\n";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "", e . ip );
    ASSERT_EQ( "", e . uri );
}

TEST_F ( TestParseFixture, GCP_EmptyIP_ReqId_Numeric )
{
    const char * InputLine =
    "\"1554306916471623\",\"35.245.77.223\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"0\",\"42000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"19919634438459959682894277668675\",\"storage.objects.get\",\"sra-pub-run-1\",\"SRR002994/SRR002994.2\""
    "\n";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "19919634438459959682894277668675", e.request_id );
}

TEST_F ( TestParseFixture, GCP_ErrorRecovery )
{
    const char * InputLine =
    "\"1554306916471623\",\"nonsense\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"0\",\"42000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"19919634438459959682894277668675\",\"storage.objects.get\",\"sra-pub-run-1\",\"SRR002994/SRR002994.2\""
    "\n"
    
    "\"1554306916471623\",\"35.245.77.223\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"0\",\"42000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"19919634438459959682894277668675\",\"storage.objects.get\",\"sra-pub-run-1\",\"SRR002994/SRR002994.2\""
    "\n"    ;

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ ( 1, m_lines.m_rejected.size() ); // line 1
    ASSERT_EQ ( 1, m_lines.m_accepted.size() ); // line 2
    ASSERT_EQ ( 0, m_lines.m_unrecognized.size() );    
}

TEST_F ( TestParseFixture, GCP_rejected )
{
    const char * InputLine =
    "\"1588261829246636\",\"35.245.218.83\",\"1\",\"\",\"POST\",\"/resumable/upload/storage/v1/b/sra-pub-src-14/o?fields=generation%2CcustomerEncryption%2Cmd5Hash%2Ccrc32c%2Cetag%2Csize&alt=json&userProject=nih-sra-datastore&uploadType=resumable\",\"200\",\"1467742168\",\"158\",\"11330000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"AAANsUnxuPe3SnDN8Y2xbJ2y94VV3u924Bfq6MLxdYC5L6aemGMz3KGEFHWBlJnz96leDkMCkJZFJO-40Rw7wdV__fs\",\"storage.objects.insert\",\"sra-pub-src-14\",\"SRR1929577/{control_24h_biorep1}.fastq.gz\"";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ ( 1, m_lines.m_accepted.size() );
}

TEST_F ( TestParseFixture, GCP_UnparsedInput_WhenRejected )
{
    const char * InputLine =
    "\"1554306916471623\",\"nonsense\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"0\",\"42000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"19919634438459959682894277668675\",\"storage.objects.get\",\"sra-pub-run-1\",\"SRR002994/SRR002994.2\"";

    std::istringstream inputstream( InputLine );
    {
        GCP_Parser p( m_lines, inputstream );
        p.parse();
        ASSERT_EQ ( 1, m_lines.m_rejected.size() ); 
        ASSERT_EQ ( string (InputLine), m_lines.m_rejected.front().unparsed);
        ASSERT_EQ ( 0, m_lines.m_accepted.size() ); 
        ASSERT_EQ ( 0, m_lines.m_unrecognized.size() );    
    }
}

TEST_F ( TestParseFixture, GCP_Object_URL_Acc_File_Ext )
{
    const char * InputLine =
    "\"1554306916471623\",\"35.245.218.83\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"0\",\"42000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"19919634438459959682894277668675\",\"storage.objects.get\",\"sra-pub-run-1\","
    "\"SRR002994/qwe.2\"";

    std::istringstream inputstream( InputLine );
    {
        GCP_Parser p( m_lines, inputstream );
        SLogGCPEvent e = parse_gcp( InputLine );
        ASSERT_EQ( "SRR002994/qwe.2", e . request . path );
        ASSERT_EQ( "SRR002994", e . request . accession );
        ASSERT_EQ( "qwe", e . request . filename );
        ASSERT_EQ( ".2", e . request . extension );
    }
}

TEST_F ( TestParseFixture, GCP_Object_URL_Acc_Acc_Ext )
{
    const char * InputLine =
    "\"1554306916471623\",\"35.245.218.83\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"0\",\"42000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"19919634438459959682894277668675\",\"storage.objects.get\",\"sra-pub-run-1\","
    "\"SRR002994/SRR002994.2\"";

    std::istringstream inputstream( InputLine );
    {
        GCP_Parser p( m_lines, inputstream );
        SLogGCPEvent e = parse_gcp( InputLine );
        ASSERT_EQ( "SRR002994/SRR002994.2", e . request . path );
        ASSERT_EQ( "SRR002994", e . request . accession );
        ASSERT_EQ( "SRR002994", e . request . filename );
        ASSERT_EQ( ".2", e . request . extension );
    }
}

TEST_F ( TestParseFixture, GCP_Object_URL_Acc__Ext )
{
    const char * InputLine =
    "\"1554306916471623\",\"35.245.218.83\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"0\",\"42000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"19919634438459959682894277668675\",\"storage.objects.get\",\"sra-pub-run-1\","
    "\"SRR002994/.2\"";

    std::istringstream inputstream( InputLine );
    {
        GCP_Parser p( m_lines, inputstream );
        SLogGCPEvent e = parse_gcp( InputLine );
        ASSERT_EQ( "SRR002994/.2", e . request . path );
        ASSERT_EQ( "SRR002994", e . request . accession );
        ASSERT_EQ( "", e . request . filename );
        ASSERT_EQ( ".2", e . request . extension ); // an explicitly empty filename, so do not use the accession to populate it
    }
}

TEST_F ( TestParseFixture, GCP_Object_URL_Acc_File_ )
{
    const char * InputLine =
    "\"1554306916471623\",\"35.245.218.83\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"0\",\"42000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"19919634438459959682894277668675\",\"storage.objects.get\",\"sra-pub-run-1\","
    "\"SRR002994/2\"";

    std::istringstream inputstream( InputLine );
    {
        GCP_Parser p( m_lines, inputstream );
        SLogGCPEvent e = parse_gcp( InputLine );
        ASSERT_EQ( "SRR002994/2", e . request . path );
        ASSERT_EQ( "SRR002994", e . request . accession );
        ASSERT_EQ( "2", e . request . filename );
        ASSERT_EQ( "", e . request . extension );
    }
}

TEST_F ( TestParseFixture, GCP_Object_URL_NoAccession )
{
    const char * InputLine =
    "\"1554306916471623\",\"35.245.218.83\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"0\",\"42000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"19919634438459959682894277668675\",\"storage.objects.get\",\"sra-pub-run-1\","
    "\"qwe.2222222\"";

    std::istringstream inputstream( InputLine );
    SLogGCPEvent e = parse_gcp( InputLine );
    // no accession in the object field ("qwe.2"), so we parse the original request
    ASSERT_EQ( "/storage/v1/b/sra-pub-run-1/o/SRR002994%2FSRR002994.2?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl", e . request . path ); 
    ASSERT_EQ( "SRR002994", e . request . accession );
    ASSERT_EQ( "SRR002994", e . request . filename );
    ASSERT_EQ( ".2", e . request . extension ); // not .2222222 from the object field
}

TEST_F ( TestParseFixture, GCP_object_empty_and_accession_in_path_parameters )
{
    const char * InputLine =
    "\"1589021828012489\",\"35.186.177.30\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-src-8/o?projection=noAcl&versions=False&fields=prefixes%2CnextPageToken%2Citems%2Fname&userProject=nih-sra-datastore&prefix=SRR1755353%2FMetazome_Annelida_timecourse_sample_0097.fastq.gz&anothrPrefix=SRR77777777&maxResults=1000&delimiter=%2F&alt=json\",\"200\",\"0\",\"3\",\"27000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"AAANsUm2OYFw_ABbdF3IlhsUe5t4NuypE_5950pi4Ox34TnnKVjsaHA7cvogPp3T5-ePgGqCBLiMsai2BLhDcRhZ95E\",\"\",\"sra-pub-src-8\",\"\"";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "/storage/v1/b/sra-pub-src-8/o?projection=noAcl&versions=False&fields=prefixes%2CnextPageToken%2Citems%2Fname&userProject=nih-sra-datastore&prefix=SRR1755353%2FMetazome_Annelida_timecourse_sample_0097.fastq.gz&anothrPrefix=SRR77777777&maxResults=1000&delimiter=%2F&alt=json", e . request . path );
    ASSERT_EQ( "SRR1755353", e . request . accession );
    ASSERT_EQ( "Metazome_Annelida_timecourse_sample_0097", e . request . filename );
    ASSERT_EQ( ".fastq.gz", e . request . extension );
}

TEST_F ( TestParseFixture, GCP_object_empty_only_accession_in_request )
{
    const char * InputLine =
    "\"1589021828012489\",\"35.186.177.30\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-src-8/o?projection=noAcl&versions=False&fields=prefixes%2CnextPageToken%2Citems%2Fname&userProject=nih-sra-datastore&prefix=SRR1755353\",\"200\",\"0\",\"3\",\"27000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"AAANsUm2OYFw_ABbdF3IlhsUe5t4NuypE_5950pi4Ox34TnnKVjsaHA7cvogPp3T5-ePgGqCBLiMsai2BLhDcRhZ95E\",\"\",\"sra-pub-src-8\",\"\"";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "/storage/v1/b/sra-pub-src-8/o?projection=noAcl&versions=False&fields=prefixes%2CnextPageToken%2Citems%2Fname&userProject=nih-sra-datastore&prefix=SRR1755353", e . request . path );
    ASSERT_EQ( "SRR1755353", e . request . accession );
    ASSERT_EQ( "SRR1755353", e . request . filename );
    ASSERT_EQ( "", e . request . extension );
}

TEST_F ( TestParseFixture, GCP_object_filename_has_equal_sign )
{
    const char * InputLine =
    "\"1589943868942178\",\"35.199.59.195\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-src-10/o/SRR11453435%2Frun1912_lane2_read1_indexN705-S506%3D122016_Cell94-F12.fastq.gz.1?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"387\",\"29000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"AAANsUkOJYv3sKNYJZeo1FanJIcIecdxEsYa4TBKKLK88UHXdQa6nlqSjqzqMZJ7nJlD7QBNKdJwEHvmXpyVam9Kwak\",\"storage.objects.get\",\"sra-pub-src-10\",\"SRR11453435/run1912_lane2_read1_indexN705-S506=122016_Cell94-F12.fastq.gz.1\"";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "SRR11453435", e . request . accession );
    ASSERT_EQ( "run1912_lane2_read1_indexN705-S506=122016_Cell94-F12", e . request . filename );
    ASSERT_EQ( ".fastq.gz.1", e . request . extension );
}

TEST_F ( TestParseFixture, GCP_object_ext_has_equal_sign )
{
    const char * InputLine =
    "\"1589943868942178\",\"35.199.59.195\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-src-10/o/SRR11453435%2Frun1912_lane2_read1_indexN705-S506%3D122016_Cell94-F12.fastq.gz.1?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"387\",\"29000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"AAANsUkOJYv3sKNYJZeo1FanJIcIecdxEsYa4TBKKLK88UHXdQa6nlqSjqzqMZJ7nJlD7QBNKdJwEHvmXpyVam9Kwak\",\"storage.objects.get\",\"sra-pub-src-10\",\"SRR11453435/run1912_lane2_read1_indexN705-S506_122016_Cell94-F12.fastq.gz=1\"";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "SRR11453435", e . request . accession );
    ASSERT_EQ( "run1912_lane2_read1_indexN705-S506_122016_Cell94-F12", e . request . filename );
    ASSERT_EQ( ".fastq.gz=1", e . request . extension );
}

TEST_F ( TestParseFixture, GCP_object_filename_has_spaces_and_ampersands )
{
    const char * InputLine =
    "\"1589988801294070\",\"34.86.94.122\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-src-12/o/SRR11509941%2FLZ017%26LZ018%26LZ019_2%20sample%20taq.fastq.gz.1?fields=name&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"343\",\"24000\",\"www.googleapis.com\",\"\",\"apitools gsutil/4.37 Python/2.7.13 (linux2) google-cloud-sdk/237.0.0 analytics/disabled,gzip(gfe)\",\"AAANsUloSsCf-idOT-SgYBGO8s8SdXw4zhnQtvdmCtC-ctONXnJ59iRUMEE5ToS7MhsA3Rh38QNfoGSckZswCZLSmrM\",\"storage.objects.get\",\"sra-pub-src-12\",\"SRR11509941/LZ017&LZ018&LZ019_2 sample taq.fast&q.gz.1\"";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "SRR11509941", e . request . accession );
    ASSERT_EQ( "LZ017&LZ018&LZ019_2 sample taq", e . request . filename );
    ASSERT_EQ( ".fast&q.gz.1", e . request . extension );
}

TEST_F ( TestParseFixture, GCP_double_accession )
{
    const char * InputLine =
    "\"1590718096562025\",\"130.14.28.7\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-sars-cov2/o/sra-src%2FSRR004257%2FSRR004257?fields=updated%2Cname%2CtimeCreated%2Csize&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"297\",\"40000\",\"storage.googleapis.com\",\"\",\"apitools gsutil/4.46 Python/2.7.5 (linux2) google-cloud-sdk/274.0.1 analytics/disabled,gzip(gfe)\",\"AAANsUl0Ofxs9M0aVz_qBWEFs-oNbk42zNRcrU6KXN5hGz3odbZ9v3_Hr_XAMIkNgsd-iYKmTR3RnQqr37E8jeEFJsE\",\"storage.objects.get\",\"sra-pub-sars-cov2\",\"sra-src/SRR004257/SRR004257\"";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "SRR004257", e . request . accession );
    ASSERT_EQ( "SRR004257", e . request . filename );
    ASSERT_EQ( "", e . request . extension );
}

TEST_F ( TestParseFixture, GCP_bad_object )
{
    const char * InputLine =
    "\"1590718096562025\",\"130.14.28.7\",\"1\",\"\",\"GET\",\"\",\"404\",\"0\",\"297\",\"40000\",\"storage.googleapis.com\",\"\",\"\",\"\",\"storage.objects.get\",\"sra-pub-sars-cov2\","
    "\"SRR004257&\"";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "", e . request . path );
    ASSERT_EQ( "", e . request . accession );
    ASSERT_EQ( "", e . request . filename );
    ASSERT_EQ( "", e . request . extension );
}

TEST_F ( TestParseFixture, GCP_user_agent )
{   
    const char * InputLine =
    "\"1590718096562025\",\"130.14.28.7\",\"1\",\"\",\"GET\",\"/storage/v1/b/sra-pub-sars-cov2/o/sra-src%2FSRR004257%2FSRR004257?fields=updated%2Cname%2CtimeCreated%2Csize&alt=json&userProject=nih-sra-datastore&projection=noAcl\",\"404\",\"0\",\"297\",\"40000\",\"storage.googleapis.com\",\"\",\"linux64 sra-toolkit test-sra.2.8.2 (phid=noc7737000,libc=2.17)\",\"AAANsUl0Ofxs9M0aVz_qBWEFs-oNbk42zNRcrU6KXN5hGz3odbZ9v3_Hr_XAMIkNgsd-iYKmTR3RnQqr37E8jeEFJsE\",\"storage.objects.get\",\"sra-pub-sars-cov2\",\"sra-src/SRR004257/SRR004257\"";

    SLogGCPEvent e = parse_gcp( InputLine );
    ASSERT_EQ( "linux64 sra-toolkit test-sra.2.8.2 (phid=noc7737000,libc=2.17)", e.agent.original );
    ASSERT_EQ( "linux64", e.agent.vdb_os );
    ASSERT_EQ( "test-sra", e.agent.vdb_tool );
    ASSERT_EQ( "2.8.2", e.agent.vdb_release );
    ASSERT_EQ( "noc", e.agent.vdb_phid_compute_env );
    ASSERT_EQ( "7737", e.agent.vdb_phid_guid );
    ASSERT_EQ( "000", e.agent.vdb_phid_session_id );
    ASSERT_EQ( "2.17", e.agent.vdb_libc );
}

extern "C"
{
    int main ( int argc, const char * argv [], const char * envp []  )
    {
        testing :: InitGoogleTest ( & argc, ( char ** ) argv );
        return RUN_ALL_TESTS ();
    }
}
