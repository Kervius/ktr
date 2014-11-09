
#include <sstream>

#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utilk.hh"


bool stat_file( const std::string &fn, stat_file_result &r )
{
	struct stat st;
	int rc;
	r = SF_NOENT;
	rc = stat( fn.c_str(), &st );
	if (rc == 0) {
		if (S_ISREG(st.st_mode))
			r = SF_FILE;
		else if (S_ISDIR(st.st_mode))
			r = SF_DIR;
		else if (S_ISLNK(st.st_mode))
			r = SF_LINK;
		else
			return false;
		return true;
	}
	else if (rc == -1 && (errno == ENOENT || errno == EACCES || errno == ENOTDIR)) {
		r = SF_NOENT;
	}
	else {
		return false;
	}
	return true;
}

bool isdir( const std::string &fn )
{
	stat_file_result st;
	return (stat_file(fn, st) && st == SF_DIR);
}


bool delete_file( const std::string &afn )
{
	bool ret = true;
	int rc;

	if (not isdir(afn)) {
		rc = unlink( afn.c_str() );
		if (rc == 0 || (rc == -1 && errno == ENOENT)) {
		}
		else {
			fprintf( stderr, "delete file %s: error: %d (%s)", afn.c_str(), errno, strerror(errno) );
			ret = false;
		}
	}
	else {
		rc = rmdir( afn.c_str() );
		if (rc == 0 || (rc == -1 && errno == ENOENT)) {
		}
		else {
			fprintf( stderr, "delete dir %s: error: %d (%s)", afn.c_str(), errno, strerror(errno) );
			ret = false;
		}
	}
	return ret;
}

std::string abs_file_name( const std::string &s )
{
	char buf[4096];

	if (s.length()) {
		std::string tmp;
		if (s[0] == '/') {
			tmp = s;
		}
		else {
			if (not getcwd( buf, sizeof(buf) ))
				return std::string();
			tmp += buf;
			tmp += "/";
			tmp += s;
		}
		if (1) {
			// trim "/." on the end of directory
			while (tmp.length() > 2 && tmp[tmp.length()-1] == '.' && tmp[tmp.length()-2] == '/') {
				tmp.resize( tmp.length()-2 );
			}
			// remove "//"
			do {
				size_t i = tmp.find( "//" );
				if (i == std::string::npos)
					break;
				tmp = tmp.substr( 0, i ) + tmp.substr( i+1 );
			} while (1);
			// remove "/../"
			const std::string dd = "/../";
			do {
				size_t i = tmp.find( dd );
				if (i == std::string::npos)
					break;
				size_t x = tmp.rfind( '/', i-1 );
				if (x == std::string::npos)
					return std::string(); // error
				// remove from x to i+dd.length();
				tmp = tmp.substr( 0, x+1 ) + tmp.substr( i+dd.length() );
			} while (1);
			// remove '/..' on the end of directory
			if (tmp.length() > 4 && tmp[tmp.length()-1] == '.' && tmp[tmp.length()-2] == '.' && tmp[tmp.length()-3] == '/') {
				size_t x = tmp.rfind( '/', tmp.length()-4 );
				if (x == std::string::npos)
					return std::string();
				tmp.resize( x );
			}
			return tmp;
		}
		else {
			// that also resolves the sym-links, which is undesirable.
			char *pp = realpath( tmp.c_str(), NULL );
			if (!pp)
				return std::string();
			tmp = pp;
			free(pp);
			return tmp;
		}
	}
	else {
		return s;
	}
}

std::string find_file_dir( const std::string &start, const std::string &fname )
{
	std::string dir = start;
	stat_file_result st;
	while (dir.length() && dir.compare("/") != 0) {
		std::string fn = dir + "/" + fname;
		if (stat_file(fn, st)) {
			if (st == SF_FILE || st == SF_LINK)
				return dir;
			dir = dirname( dir );
		}
		else {
			return std::string();
		}
	}
	return dir;
}

bool str_has_char( const std::string &s, char ch )
{
	char b[2];
	std::string::size_type p;
	b[0] = ch; b[1] = 0;
	p = s.find_first_of( b );
	return (p != std::string::npos);
}


std::string Fv( const char *fmt, va_list ap )
{
	char buf[4<<10];
	int len;
	len = vsnprintf( buf, sizeof(buf), fmt, ap );
	return std::string( buf, (size_t)len );
}

std::string F( const char *fmt, ... )
{
	va_list ap;
	std::string x;

	va_start( ap, fmt );
	x = Fv( fmt, ap );
	va_end( ap );

	return x;
}

void clean_str( std::string &str )
{
	char f = str[0];
	char l = str[str.length()-1];
	if ((f == '"' && f == '"') || (f == '(' && l == ')')) {
		std::string su = str.substr( 1, str.length()-2 );
		str = su;
	}
}

StringVecType &split(const std::string &s, char delim, StringVecType &elems, bool skip_empty)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		if (skip_empty && item.length() == 0)
			continue;
		elems.push_back(item);
	}
	return elems;
}

std::string join( char ch, const StringVecType &v )
{
	std::string ret;
	size_t i;

	if (not v.empty()) {
		ret += v[0];
		for (i = 1; i < v.size(); i++) {
			ret += ch;
			ret += v[i];
		}
	}

	return ret;
}

std::string basename( const std::string &s )
{
	size_t eo, so;
	eo = s.length()-1;

	while (s[eo] == '/')
		eo--;

	so = eo-1;
	while (so > 0 && s[so] != '/')
		so--;

	return s.substr( so+1, eo-so+1 );
}

std::string dirname( const std::string &s )
{
	size_t eo;

	if (s.length() == 0)
		return s;

	if (s.compare(".") == 0)
		return s;

	eo = s.length()-1;

	printf( "dirname(%s)\n", s.c_str() );

	while (s[eo] == '/')
		eo--;

	while (eo > 0 && s[eo] != '/')
		eo--;

	return s.substr( 0, eo );
}

void chomp( char *p, ssize_t &len )
{
	while (len > 0) {
		if (p[len - 1] == '\n' || p[len - 1] == '\r') {
			p[len - 1] = 0;
			len--;
		}
		else {
			break;
		}
	}
}

void strvec_dump( FILE *f, const char *prefix, const StringVecType &v )
{
	size_t i;
	for (i=0; i<v.size(); i++)
		fprintf( f, "%s%s\n", prefix, v[i].c_str() );
}

void strvec_dump_sl( FILE *f, const char *delim, const StringVecType &v )
{
	const char *sep;
	size_t i;
	sep = "";
	for (i=0; i<v.size(); i++) {
		fprintf( f, "%s%s", sep, v[i].c_str() );
		sep = delim;
	}
}

void check_make( int argc, char **argv )
{
	if (system( "make -q" )) {
		if (system( "make" )==0) {
			char **cp = (char **)calloc( argc+1, sizeof(void *) );
			memcpy( cp, argv, sizeof(void *)*argc );
			execv( argv[0], argv );
			printf( "exec failed\n" );
			exit(33);
		}
		else {
			exit(1);
		}
	}
}
