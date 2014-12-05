
#include <stdio.h>

#include "cksum.hh"
#include "utilk.hh"

/*
<cksum>:<type>:<entry-ts>:<fsize>:<file-mtime>:<filename>
*/

static inline uint8_t 
hex2byte( const char *buf )
{
	const uint8_t *p = (const uint8_t *)buf;

	return ((isdigit(p[0]) ? (p[0]-'0') : ((p[0]|0x20)-'a'+10)) << 4) |
		((isdigit(p[1]) ? (p[1]-'0') : ((p[1]|0x20)-'a'+10)));
}

static inline void
byte2hex( char *buf, char ch )
{
	static const char hh[] = "0123456789abcdef";

	buf[0] = hh[(ch>>4) & 0xf];
	buf[1] = hh[ch & 0xf];
}

CkSumEntry *CkSumDB::
find( const std::string &fn )
{
	auto p = db.find( fn );
	if (p != db.end())
		return p->second;
	return NULL;
}

void CkSumDB::
update( const std::string &fn, size_t fsize, char type, const std::string &cksum )
{
	CkSumEntry *e;
	auto p = db.find( fn );
	if (p != db.end()) {
		e = p->second;
	}
	else {
		e = new CkSumEntry();
		e->fname = fn;
		db[fn] = e;
	}
	e->fsize = fsize;
	e->type = type;
	e->cksum.assign( cksum.begin(), cksum.end() );
	e->entry_date = time(0);
	modified = true;
}

void CkSumDB::
clear()
{
	for ( auto I : db ) {
		CkSumEntry *p = I.second;
		delete p;
	}
	db.clear();
	modified = false;
}

bool CkSumDB::
load()
{
	char buf[8<<10];
	FILE *dbf;
	std::string fn;
	CkSumEntry *e;
	char *line = NULL;
	size_t llen = 0;
	ssize_t lread;

	//if (modified) throw something?;

	fn = dir + "/" + db_fname;
	dbf = fopen( fn.c_str(), "r" );
	if (not dbf) {
		fprintf( stderr, "error opening file: %s\n", fn.c_str() );
		return false;
	}

	clear();

	while ((lread = getline(&line, &llen, dbf))) {
		e = new CkSumEntry();
		chomp( line, lread );
		if (not deserialize( e, buf, lread ))
			return false;
		db[e->fname] = e;
	}

	fclose( dbf );

	return true;
}

bool CkSumDB::
flush( bool sync )
{
	char buf[8<<10];
	FILE *dbf;
	std::string fn;

	if (not modified)
		return true;

	fn = dir + "/" + db_fname;
	dbf = fopen( fn.c_str(), "w" );
	if (not dbf) {
		fprintf( stderr, "error opening file: %s\n", fn.c_str() );
		return false;
	}

	for ( auto I : db ) {
		int len = serialize( I.second, buf, sizeof(buf) );
		fwrite( buf, 1, len, dbf );
		if (ferror(dbf))
			return false;
		fwrite( "\n", 1, 1, dbf );
		if (ferror(dbf))
			return false;
	}

	modified = false;

	return true;
}

int CkSumDB::
serialize( CkSumEntry *e, char *buf, int buf_len )
{
	int rc;
	int olen = buf_len;

	for ( auto ch : e->cksum ) {
		byte2hex( buf, ch );
		buf+=2;
		buf_len-=2;
	}

	rc = snprintf( buf, buf_len, ":%c:%lx:%lx:%lx.%lx:",
			(char)e->type,
			(long unsigned)e->entry_date,
			(long unsigned)e->fsize,
			(long unsigned)e->fmtime.tv_sec,
			(long unsigned)e->fmtime.tv_nsec
			);
	buf += rc;
	buf_len -= rc;

	for ( auto ch : e->fname ) {
		if (ch >= 0x20 && ch <= 0x7f && ch != '%') {
			*(buf++) = ch;
			buf_len--;
		}
		else {
			buf[0] = '%';
			byte2hex( buf+1, ch );
			buf+=2;
			buf_len-=3;
		}
	}
	*(buf++) = 0;
	buf_len--;

	return olen - buf_len + 1;
}

bool CkSumDB::
deserialize( CkSumEntry *e, char *buf, int buf_len )
{
	char s_type;
	long unsigned s_entry_date;
	long unsigned s_fsize;
	long unsigned s_fmtime_sec;
	long unsigned s_fmtime_nsec;
	int rc;
	int scanned;

	int l = buf_len;
	char *p = buf;
	while (*p != 0 && *p != ':') {
		uint8_t ch = hex2byte( p );
		e->cksum.push_back( ch );
		p+=2;
		l-=2;
	}

	rc = sscanf( p, ":%c:%lx:%lx:%lx.%lx:%n", 
			&s_type,
			&s_entry_date,
			&s_fsize,
			&s_fmtime_sec, &s_fmtime_nsec,
			&scanned
		   );

	if ( rc < 5 )
		return false;

	p+=scanned;
	l-=scanned;

	e->type = s_type;
	e->entry_date = s_entry_date;
	e->fsize = s_fsize;
	e->fmtime.tv_sec = s_fmtime_sec;
	e->fmtime.tv_nsec = s_fmtime_nsec;

	while (*p) {
		if (*p != '%') {
			e->fname.push_back( *p );
			p++;
			l--;
		}
		else {
			uint8_t ch = hex2byte( p+1 );
			e->fname.push_back( (char)ch );
			p+=3;
			l-=3;
		}
	}

	return true;
}

