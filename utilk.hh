#ifndef K_UTIL_____
#define K_UTIL_____

#include <string>
#include <vector>
#include <set>

#include <stdio.h>

enum stat_file_result {
	SF_NOENT = 0,
	SF_FILE,
	SF_DIR,
	SF_LINK,
};

typedef std::set<std::string> StringSetType;
typedef std::vector<std::string> StringVecType;

bool stat_file( const std::string &fn, stat_file_result &r );
bool isdir( const std::string &fn );
bool delete_file( const std::string &afn );
std::string abs_file_name( const std::string &s );
std::string find_file_dir( const std::string &start, const std::string &fname );
bool str_has_char( const std::string &s, char ch );
std::string F( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
std::string Fv( const char *fmt, va_list ap );
void clean_str( std::string &str );
StringVecType &split(const std::string &s, char delim, StringVecType &elems, bool skip_empty = false);
std::string join( char ch, const StringVecType &v );
std::string basename( const std::string &s );
std::string dirname( const std::string &s );
void chomp( char *p, ssize_t &len );
void strvec_dump( FILE *f, const char *prefix, const StringVecType &v );
void strvec_dump_sl( FILE *f, const char *delim, const StringVecType &v );
void check_make( int argc, char **argv );

#endif // K_UTIL_____
