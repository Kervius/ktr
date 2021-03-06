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

bool begins_with( const std::string &str, const std::string &pref );
bool ends_with( const std::string &str, const char *suff );
bool ends_with( const std::string &str, const std::string &suff );
bool str_has_char( const std::string &s, char ch );

std::string F( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
std::string Fv( const char *fmt, va_list ap );

void clean_str( std::string &str );

StringVecType &split(const std::string &s, char delim, StringVecType &elems, bool skip_empty = false);
void split(const std::string &s, const char *delims, std::vector<char *> &elems, bool skip_empty);

std::string join( char ch, const StringVecType &v );

std::string basename( const std::string &s );
std::string dirname( const std::string &s );
std::string chop_dir_front( const std::string &str, const std::string &pref );
std::string normalize_path( const std::string &str, bool *err );

void chomp( char *p, ssize_t &len );
void chomp( std::string &s );

void strvec_dump( FILE *f, const char *prefix, const StringVecType &v );
void strvec_dump_sl( FILE *f, const char *delim, const StringVecType &v );

void check_make( int argc, char **argv );

bool is_file_younger( struct timespec &ts1, struct timespec &ts2 );
bool is_file_older( struct timespec &ts1, struct timespec &ts2 );
bool is_younger_mtime( const std::string &fn, struct timespec &ts2 );
bool is_older_mtime( const std::string &fn, struct timespec &ts2 );

#endif // K_UTIL_____
