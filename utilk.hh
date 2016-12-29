#ifndef K_UTIL_____
#define K_UTIL_____

#include <string>
#include <vector>
#include <set>

#include <stdio.h>

namespace Ktr
{
namespace Utils
{

enum stat_file_result {
	SF_NOENT = 0,
	SF_FILE,
	SF_DIR,
	SF_LINK,
};

typedef std::set<std::string> StringSetType;
typedef std::vector<std::string> StringVecType;

bool StatFile( const std::string &fn, stat_file_result &r );
bool IsDir( const std::string &fn );
bool DeleteFile( const std::string &afn );

std::string AbsFileName( const std::string &s );
std::string FindFileDir( const std::string &start, const std::string &fname );

bool BeginsWith( const std::string &str, const std::string &pref );
bool EndsWith( const std::string &str, const char *suff );
bool EndsWith( const std::string &str, const std::string &suff );
bool StrHasChar( const std::string &s, char ch );

std::string F( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
std::string Fv( const char *fmt, va_list ap );

void CleanStr( std::string &str );

StringVecType &Split(const std::string &s, char delim, StringVecType &elems, bool skip_empty = false);
void Split(const std::string &s, const char *delims, std::vector<char *> &elems, bool skip_empty);

std::string Join( char ch, const StringVecType &v );

std::string basename( const std::string &s );
std::string dirname( const std::string &s );
std::string ChopDirFront( const std::string &str, const std::string &pref );
std::string NormalizePath( const std::string &str, bool *err );

void chomp( char *p, ssize_t &len );
void chomp( std::string &s );

void StrvecDump( FILE *f, const char *prefix, const StringVecType &v );
void StrvecDumpSl( FILE *f, const char *delim, const StringVecType &v );

void CheckMake( int argc, char **argv );

bool GetMtime( const std::string &fn, struct timespec &ts );

bool IsFileYounger( struct timespec &ts1, struct timespec &ts2 );
bool IsFileOlder( struct timespec &ts1, struct timespec &ts2 );
bool IsYoungerMtime( const std::string &fn, struct timespec &ts2 );
bool IsOlderMtime( const std::string &fn, struct timespec &ts2 );

};
};

#endif // K_UTIL_____
