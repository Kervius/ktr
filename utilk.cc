
#include <sstream>

#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
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

bool begins_with( const std::string &str, const std::string &pref )
{
	return str.size() >= pref.size() && str.compare( 0, pref.size(), pref ) == 0;
}

bool ends_with( const std::string &str, const char *suff )
{
	size_t sl = strlen(suff);
	if (str.size() < sl)
		return false;
	return str.compare( str.size()-sl, sl, suff ) == 0;
}

bool ends_with( const std::string &str, const std::string &suff )
{
	if (str.size() < suff.size())
		return false;
	return str.compare( str.size()-suff.size(), suff.size(), suff ) == 0;
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
	if ((f == '"' && l == '"') || (f == '(' && l == ')')) {
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

void split(const std::string &s, const char *delims, std::vector<char *> &elems, bool skip_empty)
{
	char *p, *t;
	static const char spaces[] = " \t\n\r";

	if (!delims || !delims[0])
		delims = spaces;

	t = const_cast<char *>(s.c_str());

	while (1) {
		p = strpbrk( t, delims );
		if (p) {
			size_t l = p - t;
			if (l == 0 && skip_empty) {
				t = p+1;
				continue;
			}
			//printf( "l = %d; %.*s\n", (int)l, (int)l, t );
			elems.push_back( strndup( t, l ) );
			t = p+1;
		}
		else {
			elems.push_back( strdup( t ) );
			break;
		}
	}
	//for (auto x : elems) printf( "split2: [%s]\n", x );
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

	//printf( "dirname(%s)\n", s.c_str() );

	while (s[eo] == '/')  // skip trailing '/'
		eo--;

	while (eo > 0 && s[eo] != '/')  // find real last '/'
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
void chomp( std::string &s )
{
	while (s.size()) {
		char ch = s[s.size()-1];
		if (ch == '\n' || ch == '\r')
			s.resize( s.size()-1 );
		else
			break;
	}
}

std::string chop_dir_front( const std::string &str, const std::string &pref )
{
	if (begins_with(str, pref)) {
		if (str.length() > pref.size() && str[pref.size()] == '/')
			return str.substr( pref.length()+1 );
		else
			return str.substr( pref.length() );
	}
	else {
		return str;
	}
}

std::string normalize_path( const std::string &str, bool *err )
{
	StringVecType pcs;
	StringVecType ret;
	std::string r;
	bool is_abs = false;

	if (str.empty())
		return std::string();

	if (err) *err = false;

	is_abs = (str[0] == '/');

	// remove "//"
	// remove "/./"
	// resolve "../aaa"

	split( str, '/', pcs, true );
	for (size_t i = 0; i<pcs.size(); i++) {
		if (pcs[i].compare( "." ) == 0) {
			// skip
		}
		else if (pcs[i].compare( ".." ) == 0) {
			if (i < pcs.size()-1) {
				// skip
				i++;
			}
			else {
				if (err) *err = true;
				ret.push_back( pcs[i] );
			}
		}
		else {
			ret.push_back( pcs[i] );
		}
	}

	if (is_abs) r += '/';
	for ( auto s : ret ) {
		r += s;
		r += '/';
	}
	if (r.size() > 1)
		r.resize( r.size()-1 ); // strip last '/';
	return r;
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
	if (0) {
		if (system( "make -q" ) == 0x100) {
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
}

bool get_mtime( const std::string &fn, struct timespec &ts )
{
	struct stat st;
	if (stat( fn.c_str(), &st ) == 0) {
		ts = st.st_mtim;
		//fprintf( stderr, "\t" "%s" "\t" "%ld.%09ld\n", fn.c_str(), ts.tv_sec, ts.tv_nsec );
		return true;
	}
	else {
		ts.tv_sec = 0;
		ts.tv_nsec = 0;
		return false;
	}
}

bool is_file_younger( struct timespec &ts1, struct timespec &ts2 )
{
	// younger == greater
	return (ts1.tv_sec > ts2.tv_sec) || (ts1.tv_sec == ts2.tv_sec && ts1.tv_nsec > ts2.tv_nsec);
}

bool is_file_older( struct timespec &ts1, struct timespec &ts2 )
{
	// older == less
	return is_file_younger( ts2, ts1 );
}

bool is_younger_mtime( const std::string &fn, struct timespec &ts2 )
{
	struct timespec ts1;
	if (get_mtime( fn, ts1 )) {
		if (is_file_younger( ts1, ts2 ))
			ts2 = ts1;
		return true;
	}
	return false;
}

bool is_older_mtime( const std::string &fn, struct timespec &ts2 )
{
	struct timespec ts1;
	if (get_mtime( fn, ts1 )) {
		if (is_file_older( ts1, ts2 ))
			ts2 = ts1;
		return true;
	}
	return false;
}
