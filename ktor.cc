
// includes {{{1

#include <map>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <sstream>
#include <utility>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <regex.h>

// declarations {{{1

/*
 * definitions
 */

namespace K {
struct Env;
struct RuleInvoc;
}

typedef std::set<std::string> StringSetType;
typedef std::vector<std::string> StringVecType;
typedef std::vector<K::RuleInvoc *> RuleInvocVecType;

void strvec_dump( FILE *f, const char *prefix, const StringVecType &v );
void strvec_dump_sl( FILE *f, const char *delim, const StringVecType &v );
std::string env_expand( const K::Env *env, const std::string &str );
void env_expand( const K::Env *env, StringVecType &v );



namespace K
{

struct Env {
	Env *parent;
	typedef std::map<std::string, std::string> items_type;
	items_type items;

	Env( Env *elder = NULL )
		: parent(elder)
	{
	}

	void set( const std::string &n, const std::string &v ) { items[n] = v; };
	const std::string& get( const std::string &n ) const { return items.at(n); };
	std::string getd( const std::string &n, const std::string &def = std::string()) const
	{
		items_type::const_iterator p = items.find( n );
		if (p != items.end())
			return p->second;
		else if (parent)
			return parent->getd( n, def );
		else
			return def;
	}
	void dump( FILE *f ) const
	{
		items_type::const_iterator II, EE;
		II = items.begin();
		EE = items.end();
		for ( ;II != EE; ++II) {
			fprintf( f, "%s=%s\n", II->first.c_str(), II->second.c_str() );
		}
	}
	std::string exp( const std::string &s ) const
	{
		return env_expand( this, s );
	}
};

struct RuleDef {
	enum ParamType{
		RD_P_ANY = -1,
		RD_P_NONE = 0,
	};
	int ptInput;
	int ptOutput;
	std::string command;
	std::string name;

	void dump( FILE *f ) const
	{
		char iii[16], ooo[16];
		if (ptInput == RD_P_ANY)
			snprintf( iii, sizeof(iii), "%s", "any" );
		else
			snprintf( iii, sizeof(iii), "%d", ptInput );

		if (ptOutput == RD_P_ANY)
			snprintf( ooo, sizeof(ooo), "%s", "any" );
		else
			snprintf( ooo, sizeof(ooo), "%d", ptOutput );

		fprintf( f, "rule %s i=(%s) o=(%s) c=(%s)\n", name.c_str(), iii, ooo, command.c_str() );
	}
};

struct RuleInvoc {
	StringVecType input;
	StringVecType output;
	StringVecType deps;
	std::string rule_name;
	std::string command;
	RuleDef *rule;

	RuleInvoc()
		: rule(NULL)
	{}

	void dump( FILE *f ) const
	{
		fprintf( f, "do %s i=(", rule_name.c_str() );
		strvec_dump_sl( f, " ", input );
		fprintf( f, ") o=(" );
		strvec_dump_sl( f, " ", output );
		fprintf( f, ") d=(" );
		strvec_dump_sl( f, " ", deps );
		fprintf( f, ")\n" );
	}
};

struct KFile {
	Env env;
	KFile *parent;

	std::string basename;
	std::string dirname;
	std::string absdirname;


	std::vector<RuleDef *> rd;
	RuleInvocVecType ri;
	StringVecType defaults;
	
	StringVecType subdirs;
	std::vector<KFile *> subparts;

	KFile()
		: parent(NULL)
	{}


	void set_parent( KFile *p )
	{
		this->parent = p;
		this->env.parent = p ? &p->env : NULL;
	}

	KFile *findsubfile( const std::string &name )
	{
		for (size_t i=0; i<subdirs.size(); i++)
			if (subdirs[i] == name)
				return subparts[i];
		return NULL;
	}

	RuleInvoc *find_ri_output( const std::string &name )
	{
		for (size_t i=0; i<ri.size(); i++)
			for (size_t j=0; j<ri[i]->output.size(); j++)
				if (ri[i]->output[j] == name)
					return ri[i];
		return NULL;
	}

	bool has_target( const std::string &name )
	{
		return find_ri_output( name ) != NULL;
	}

	RuleDef *find_rule_def( const std::string &name )
	{
		for (size_t i=0; i<rd.size(); i++)
			if (rd[i]->name == name)
				return rd[i];
		if (parent)
			return parent->find_rule_def( name );
		return NULL;
	}

	const Env *e() const { return &env; }
	Env *e() { return &env; }
	std::string expand_var( const std::string &str ) const
	{
		return env_expand( &this->env, str );
	}
};

struct K {
	std::string root_dir;
	KFile *root_kfile;
};

}


/* =========================================================== */

/* =========================================================== */

/* =========================================================== */

// functions {{{1

// utility {{{2

std::string env_expand( const K::Env *env, const std::string &str )
{
	size_t i;
	std::string ret;
	i = 0;
	while (i<str.length()) {
		if (str[i] != '%') {
			ret.push_back( str[i] );
			i++;
		}
		else {
			size_t s, e;
			if (str[i+1] == '{') {
				s = i+2;
				e = s;
				while (str[e] != '}')
					e++;
				std::string var_name = str.substr( s, e-s );
				std::string val = env->getd( var_name );
				std::string expanded = env_expand( env, val );
				ret += expanded;
				i = e+1;
			}
			else {
				exit(1);
			}
		}
	}
	return ret;
}

void env_expand( const K::Env *env, StringVecType &v )
{
	size_t j;
	for (j=0; j<v.size(); j++)
		v[j] = env->exp( v[j] );
}

std::string subm_to_str( const char *str, const regmatch_t &m )
{
	if (m.rm_so != -1 && m.rm_eo != -1)
		return std::string( str + m.rm_so, m.rm_eo - m.rm_so );
	else
		return std::string();
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

StringVecType &split(const std::string &s, char delim, StringVecType &elems, bool skip_empty = false) {
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
	eo = s.length()-1;
	while (s[eo] == '/')
		eo--;

	while (eo > 0 && s[eo] != '/')
		eo--;

	return s.substr( 0, eo );
}

static void chomp( char *p, ssize_t &len )
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

std::string F( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
std::string F( const char *fmt, ... )
{
	va_list ap;
	std::string x;

	va_start( ap, fmt );
	x = Fv( fmt, ap );
	va_end( ap );

	return x;
}

// kfile loading {{{2

int kfile_line_var( const char *inp, K::KFile *kf )
{
	static bool second_time;
	static regex_t reg;
	regmatch_t subm[5];
	int rc;

	if (!second_time) {
		second_time = true;
		rc = regcomp( &reg, "^var[[:space:]]+([^=]+)=(.*)$", REG_EXTENDED );
		if (rc)
			return -1;
	}

	rc = regexec( &reg, inp, 5, subm, 0 );
	if (rc == 0) {
		std::string name = subm_to_str( inp, subm[1] );
		std::string value = subm_to_str( inp, subm[2] );
//		printf( ">>> var %s=%s\n", name.c_str(), value.c_str() );
		kf->e()->set( name, value );
		return 0;
	}
	else if (rc == REG_NOMATCH) {
		return 1;
	}
	else {
		return -1;
	}

	return 0;
}

int kfile_line_subdir( const char *inp, K::KFile *kf )
{
	static bool second_time;
	static regex_t reg;
	regmatch_t subm[5];
	int rc;

	if (!second_time) {
		second_time = true;
		rc = regcomp( &reg, "^subdir[[:space:]]+(.+)[[:space:]]*$", REG_EXTENDED );
		if (rc)
			return -1;
	}

	rc = regexec( &reg, inp, 5, subm, 0 );
	if (rc == 0) {
		std::string subdir = subm_to_str( inp, subm[1] );
//		printf( ">>> subdir %s\n", subdir.c_str() );
		kf->subdirs.push_back( subdir );
		return 0;
	}
	else if (rc == REG_NOMATCH) {
		return 1;
	}
	else {
		return -1;
	}

	return 0;
}

int kfile_line_default( const char *inp, K::KFile *kf )
{
	static bool second_time;
	static regex_t reg;
	regmatch_t subm[5];
	int rc;

	if (!second_time) {
		second_time = true;
		rc = regcomp( &reg, "^default[[:space:]]+(.+)[[:space:]]*$", REG_EXTENDED );
		if (rc)
			return -1;
	}

	rc = regexec( &reg, inp, 5, subm, 0 );
	if (rc == 0) {
		std::string defl = subm_to_str( inp, subm[1] );
//		printf( ">>> default %s\n", defl.c_str() );
		kf->defaults.push_back( defl );
		return 0;
	}
	else if (rc == REG_NOMATCH) {
		return 1;
	}
	else {
		return -1;
	}

	return 0;
}

int kfile_line_rule_def( const char *inp, K::KFile *kf )
{
	static bool second_time;
	static regex_t reg;
	regmatch_t subm[10];
	int rc;

	if (!second_time) {
		second_time = true;
		rc = regcomp( &reg, 
				"^rule[[:space:]]+"
				"([^[:space:]]+)[[:space:]]+"
				"i=(\\(.*\\)|\\\".*\\\"|[^[:space:]]+)[[:space:]]+"
				"o=(\\(.*\\)|\\\".*\\\"|[^[:space:]]+)[[:space:]]+"
				"c=(\\(.*\\)|\\\".*\\\"|[^[:space:]]+)[[:space:]]*"
				"$"
				,
				REG_EXTENDED );
		if (rc) {
			printf( "oops\n" );
			return -1;
		}
	}

	rc = regexec( &reg, inp, 10, subm, 0 );
	//printf( "rc == %d; line=[%s]\n", rc, inp);
	if (rc == 0) {
		std::string name = subm_to_str( inp, subm[1] );
		std::string iii = subm_to_str( inp, subm[2] );
		std::string ooo = subm_to_str( inp, subm[3] );
		std::string ccc = subm_to_str( inp, subm[4] );

//		printf( ">>> rule %s i=(%s) o=(%s) c=(%s)\n", name.c_str(), iii.c_str(), ooo.c_str(), ccc.c_str() );

		K::RuleDef *rd = new K::RuleDef;
		rd->name = name;
		clean_str( ccc );
		rd->command = ccc;
		if (iii.compare("any") == 0)
			rd->ptInput = K::RuleDef::RD_P_ANY;
		else
			rd->ptInput = atoi( iii.c_str() );

		if (ooo.compare("any") == 0)
			rd->ptOutput = K::RuleDef::RD_P_ANY;
		else
			rd->ptOutput = atoi( ooo.c_str() );

		kf->rd.push_back( rd );

		return 0;
	}
	else if (rc == REG_NOMATCH) {
		return 1;
	}
	else {
		return -1;
	}

	return 0;
}

int kfile_line_do( const char *inp, K::KFile *kf )
{
	static bool second_time;
	static regex_t reg;
	regmatch_t subm[10];
	int rc;
	size_t i;

	if (!second_time) {
		second_time = true;
		rc = regcomp( &reg, 
				"^do[[:space:]]+"
				"([^[:space:]]+)[[:space:]]+"
				"i=(\\(.*\\)|\\\".*\\\"|[^[:space:]]+)[[:space:]]+"
				"o=(\\(.*\\)|\\\".*\\\"|[^[:space:]]+)"
				"([[:space:]]+d=(\\(.*\\)|\\\".*\\\"|[^[:space:]]+)[[:space:]]*)?"
				"$"
				,
				REG_EXTENDED );
		if (rc) {
			return -1;
		}
	}

	rc = regexec( &reg, inp, 10, subm, 0 );
	if (rc == 0) {
		std::string name = subm_to_str( inp, subm[1] );
		std::string iii = subm_to_str( inp, subm[2] ); clean_str( iii );
		std::string ooo = subm_to_str( inp, subm[3] ); clean_str( ooo );
		std::string ddd = subm_to_str( inp, subm[5] ); clean_str( ddd );

//		printf( ">>> do %s i=(%s) o=(%s) d=(%s)\n", name.c_str(), iii.c_str(), ooo.c_str(), ddd.c_str() );

		K::RuleInvoc *ri = new K::RuleInvoc;
		ri->rule_name = name;

		StringVecType temp;

		temp.clear();
		split( ddd, ' ', temp );
		for (i=0;i<temp.size();i++)
			ri->deps.push_back( temp[i] );

		temp.clear();
		split( iii, ' ', temp );
		for (i=0;i<temp.size();i++)
			ri->input.push_back( temp[i] );

		temp.clear();
		split( ooo, ' ', temp );
		for (i=0;i<temp.size();i++)
			ri->output.push_back( temp[i] );

		kf->ri.push_back( ri );

		return 0;
	}
	else if (rc == REG_NOMATCH) {
		return 1;
	}
	else {
		return -1;
	}

	return 0;
}


int kfile_line_empty( const char *inp )
{
	while ( *inp ) {
		if (isspace(*inp)) {
			inp++;
			continue;
		}
		return false;
	}
	return true;
}

int kfile_load( const std::string &fn, K::KFile *kf )
{
	FILE *f;
	char *line_buf = 0;
	size_t buf_len = 0;
	size_t i;
	ssize_t line_len;
	int rc;

	f = fopen( fn.c_str(), "r" );
	if (!f)
		return -1;

	while ( (line_len = getline( &line_buf, &buf_len, f )) != -1 ) {
		//printf( "%s", line_buf );
		chomp( line_buf, line_len );
		if (kfile_line_empty( line_buf ))
			continue;

		if ((rc = kfile_line_var( line_buf, kf )) == 0) continue;
		if (rc < 0) break;

		if ((rc = kfile_line_subdir( line_buf, kf )) == 0) continue;
		if (rc < 0) break;

		if ((rc = kfile_line_default( line_buf, kf )) == 0) continue;
		if (rc < 0) break;

		if ((rc = kfile_line_rule_def( line_buf, kf )) == 0) continue;
		if (rc < 0) break;

		if ((rc = kfile_line_do( line_buf, kf )) == 0) continue;
		if (rc < 0) break;

		printf( "unrec l: [%s]\n", line_buf );
		return -1;
	}
	if (rc < 0)
		return -1;

	// expand variables

	for (i=0; i<kf->defaults.size(); i++) {
		kf->defaults[i] = kf->expand_var( kf->defaults[i] );
	}
	for (i=0; i<kf->rd.size(); i++) { // RuleDef
		kf->rd[i]->name = kf->expand_var( kf->rd[i]->name );
	}
	for (i=0; i<kf->ri.size(); i++) { // RuleInvoc KFile
		size_t j;
		K::RuleInvoc *ri = kf->ri[i];
		ri->rule_name = kf->expand_var( ri->rule_name );
		ri->rule = kf->find_rule_def( ri->rule_name );
		if (ri->rule == NULL) {
			printf( "rule def [%s] not found\n", ri->rule_name.c_str() );
			return -1;
		}

		env_expand( kf->e(), ri->input );
		env_expand( kf->e(), ri->output );
		env_expand( kf->e(), ri->deps );

		K::Env *tmp = new K::Env( kf->e() );

		tmp->set( "input", join( ' ', ri->input ) );
		for (j=0; j<ri->input.size(); j++)
			tmp->set( F("input%lu",(long unsigned)(j+1)), ri->input[j] );

		tmp->set( "output", join( ' ', ri->output ) );
		for (j=0; j<ri->output.size(); j++)
			tmp->set( F("output%lu",(long unsigned)(j+1)), ri->output[j] );

		ri->command = tmp->exp( ri->rule->command );
		//printf( "CE: [%s] -> [%s]\n", ri->rule->command.c_str(), ri->command.c_str() );
	}

	return 0;
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

void kfile_dump( K::KFile *kf, FILE *f = NULL )
{
	size_t i;
	if (!f)
		f = stdout;
	kf->e()->dump( f );
	strvec_dump( f, "default ", kf->defaults );
	strvec_dump( f, "subdir ", kf->subdirs );
	for (i=0; i<kf->rd.size(); i++)
		kf->rd[i]->dump( f );
	for (i=0; i<kf->ri.size(); i++)
		kf->ri[i]->dump( f );
}

// k loading {{{2

K::KFile *kfile_load_sub( const std::string &dir, K::KFile *parent )
{
	size_t i;
	int rc;
	std::string ffn;
	K::KFile *kf;
	
	kf = new K::KFile;
	kf->set_parent( parent );

	kf->basename = basename( dir );
	kf->dirname = (kf->parent != NULL)
		? (kf->parent->dirname[0] == '.') 
			? kf->basename
			: kf->parent->dirname + "/" + kf->basename
		: std::string(".");
	kf->absdirname = dir;

	if (kf->parent == NULL)
		kf->env.set( "root", kf->absdirname );

	ffn = dir;
	ffn += "/";
	ffn += (kf->parent == NULL) ? "kfile" : "kpart";

	//printf( "loading %s\n", ffn.c_str() );

	rc = kfile_load( ffn, kf );
	if (rc < 0) {
		printf( "failed to load file %s\n", ffn.c_str() );
		exit(1);
	}

	for (i = 0; i < kf->subdirs.size(); i++) {
		K::KFile *part;
		std::string n = dir + "/" + kf->subdirs[i];
		//printf( "recurse %s\n", n.c_str() );
		part = kfile_load_sub( n, kf );
		kf->subparts.push_back( part );
	}

	return kf;
}

K::K *k_load( const std::string &root_dir )
{
	K::K *k = new K::K;
	k->root_dir = root_dir;
	k->root_kfile = kfile_load_sub( k->root_dir, 0 );
	return k;
}

// target utility {{{2

void k_dump_r( K::KFile *kf )
{
	for (size_t i=0; i<kf->subdirs.size(); i++) {
		printf( "\t%s\n", kf->subdirs[i].c_str() );
		k_dump_r( kf->subparts[i] );
	}
}

void k_dump( K::K *k )
{
	printf( "root is %s\n", k->root_dir.c_str() );
	printf( "root kf dir %s\n", k->root_kfile->dirname.c_str() );
	k_dump_r( k->root_kfile );
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

enum {
	CA_INPUT,
	CA_OUTPUT,
	CA_DEPS
};

void kfile_collect_all_x( K::KFile *kf, int x, StringSetType &set )
{
	size_t i, j;
	for (i=0; i<kf->ri.size(); i++) {
		K::RuleInvoc *ri = kf->ri[i];
		StringVecType *v_;

		if (x == CA_INPUT)
			v_ = &ri->input;
		else if (x == CA_OUTPUT)
			v_ = &ri->output;
		else
			v_ = &ri->deps;

		StringVecType &v = *v_;

		for (j=0; j<v.size(); j++) {
			if (!str_has_char( v[j], '/' ))
				set.insert( kf->dirname + "/" + v[j] );
			else if ( v[j][0] == '/')
				set.insert( v[j] );
			else
				set.insert( v[j] );
		}
	}
	for (i=0; i<kf->subparts.size(); i++)
		kfile_collect_all_x( kf->subparts[i], x, set );
}
void k_collect_all_x( K::K *k, int x, StringSetType &set )
{
	set.clear();
	kfile_collect_all_x( k->root_kfile, x, set );
}


void k_find_sources( K::K *k, StringVecType &sources )
{
	StringSetType ins, outs;

	k_collect_all_x( k, CA_INPUT, ins );
	k_collect_all_x( k, CA_OUTPUT, outs );

	sources.clear();

	auto II = ins.begin();
	auto EE = ins.end();
	for ( ; II != EE; ++II)
		if (outs.count(*II) == 0)
			sources.push_back( *II );
}

namespace K
{
struct InvocTree {
	RuleInvoc *ri;
	KFile *kf;
	std::vector<InvocTree *> prereq;
	InvocTree( RuleInvoc *ri_=NULL, KFile *kf_=NULL ) : ri(ri_), kf(kf_) {};
};
typedef std::map< std::string, InvocTree * > OutputMapType;

struct InvocMap {
	std::vector<InvocTree *> tt;
	OutputMapType om;
	std::set<std::string> src;
};

struct JobQueue {
	std::set<InvocTree *> visited;
	std::vector<InvocTree *> queue;
};

}


void kfile_fill_target_map( K::KFile *kf, K::InvocMap &im )
{
	// RIs of the file.
	for (K::RuleInvoc *&ri : kf->ri) {
		if (ri->output.empty())
			continue;
		K::InvocTree *tmp = new K::InvocTree;
		tmp->ri = ri;
		tmp->kf = kf;
		im.tt.push_back( tmp );
		for (std::string &n : ri->output) {
			std::string fn;
			if (not str_has_char( n, '/' )) // local target
				fn = kf->dirname + "/" + n;
			else // relative or absolute target
				fn = n;
			im.om[ fn ] = tmp;
		}
	}
	// RIs of the subparts
	for (K::KFile *&skf : kf->subparts)
		kfile_fill_target_map( skf, im );
}

void kinvoctree_fill_prereq_vec( K::InvocMap &im, K::InvocTree *it, StringVecType &inputs )
{
	const std::string &dn = it->kf->dirname;
	for (std::string &ii : inputs) {
		std::string ifn;
		if (not str_has_char( ii, '/' ))
			ifn = dn + "/" + ii;
		else
			ifn = ii;
		auto ptr = im.om.find(ifn);
		// have we found an output which is our input?
		if (ptr != im.om.end()) // we depend on it.
			it->prereq.push_back( ptr->second );
		else	// it must be a source file.
			im.src.insert( ifn );
	}
}

void kinvoctree_fill_prereq( K::InvocMap &im )
{
	for (K::InvocTree *&it : im.tt ) {
		kinvoctree_fill_prereq_vec( im, it, it->ri->input );
		kinvoctree_fill_prereq_vec( im, it, it->ri->deps );
	}
}

void k_build_tree( K::K *k, K::InvocMap &im )
{
	kfile_fill_target_map( k->root_kfile, im );
	kinvoctree_fill_prereq( im );
}

bool k_fill_job_queue( K::InvocTree *it, K::JobQueue &jq )
{
	if (jq.visited.count( it )) // has node been already visited?
		return true;

	jq.visited.insert( it );

	if (not it->prereq.empty()) {
		for (K::InvocTree *&it2 : it->prereq) {
			bool ret = k_fill_job_queue( it2, jq );
			if (!ret) return false;
		}
	}
	jq.queue.push_back( it );
	return true;
}
bool k_fill_job_queue( const std::string &target, K::InvocMap &im, K::JobQueue &jq )
{
	auto p = im.om.find( target );
	if (p != im.om.end()) {
		return k_fill_job_queue( p->second, jq );
	}
	return false;
}

bool kinvoctree_invoke( K::InvocTree *it )
{
	std::string cmd;
	int ret;

	cmd += "cd ";
	cmd += it->kf->absdirname;
	cmd += " && ( ";
	cmd += it->ri->command;
	cmd += " )";

	fprintf( stderr, "CMD: %s\n", cmd.c_str() );

	ret = system( cmd.c_str() );

	if (ret == -1)
		fprintf( stderr, "failed to start the job.\n" );
	else if (ret)
		fprintf( stderr, "job has failed, rc=%04x\n", ret );

	return ret == 0;
}

bool k_clean( K::InvocMap &im )
{
	(void)&im;
	// TODO
	// build job queue for all targets
	// execute commands in the reverse order
	return false;
}

// the main {{{1

/* =========================================================== */

/* =========================================================== */

/* =========================================================== */

int main( int argc, char **argv )
{
	char buf[4096];
	std::string root_dir;
	getcwd( buf, sizeof(buf) );

	check_make(argc, argv);

	root_dir = std::string(buf);
	root_dir += "/";
	root_dir += "test";

	K::K *k = k_load( root_dir );

	if (1) {
		K::InvocMap im;
		k_build_tree( k, im );

		printf( "sources:\n" );
		for (const std::string &s : im.src)
			printf("\t%s\n", s.c_str());
		printf( "\n" );

		printf( "targets:\n" );
		for (auto &om : im.om) {
			const std::string &tn = om.first;
			K::InvocTree *it = om.second;
			printf("\t%s <- %p\n",tn.c_str(),it);
		}
		printf( "\n" );

		printf( "look-up defaults:\n" );
		for (const std::string &d : k->root_kfile->defaults) {
			printf( "%s\n", d.c_str() );

			K::JobQueue jq;
			bool ret = k_fill_job_queue( d, im, jq );
			if (ret) {
				for (K::InvocTree *&it : jq.queue)
					printf( "\t%u : %s : %s\n", (unsigned)it->prereq.size(), it->kf->absdirname.c_str(), it->ri->command.c_str() );
				for (K::InvocTree *&it : jq.queue)
					if (not kinvoctree_invoke( it ))
						exit(1);
			}
			else {
				printf( "\t\tERROR\n" );
			}
			
		}
		printf( "\n" );
	}


	if (0) {
		StringVecType a;
		k_find_sources( k, a );
		for (std::string &s : a) {
			printf( "%s/%s\n", k->root_dir.c_str(), s.c_str() );
		}
	}

	return 0;
}

// vim:foldmethod=marker:foldopen+=jump:
