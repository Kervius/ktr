	
#include <map>
#include <string>
#include <vector>
#include <list>
#include <sstream>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <regex.h>

/*
 * definitions
 */

namespace K {
struct Env;
struct UserString;
}

void strvec_dump( FILE *f, const char *prefix, const std::vector<std::string> &v );
void strvec_dump_sl( FILE *f, const char *delim, const std::vector<std::string> &v );
void strvec_dump( FILE *f, const char *prefix, const std::vector<K::UserString> &v );
void strvec_dump_sl( FILE *f, const char *delim, const std::vector<K::UserString> &v );
std::string env_expand( K::Env *env, const std::string &str );

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
	std::string expand( const std::string &s )
	{
		return env_expand( this, s );
	}
};

struct UserString {
	mutable bool is_exp;
	Env *env;
	std::string value;
	mutable std::string expanded;
	UserString( Env *env_ = NULL, const std::string &str = std::string() )
	{
		env = env_;
		is_exp = false;
		value = str;
	}
	void set( Env *env_, const std::string &s )
	{
		env = env_;
		value = s;
	}
	void expand_this() const
	{
		if (is_exp) {
		} else {
			expanded = env->expand( value );
			is_exp = true;
		}
	}
	const std::string &str() const
	{
		expand_this();
		return expanded;
	}
	const char *c_str() const
	{
		return str().c_str();
	}
	bool operator == (const std::string &s) const
	{
		return str() == s;
	}
};

struct RuleDef {
	enum ParamType{
		RD_P_ANY = -1,
		RD_P_NONE = 0,
	};
	int ptInput;
	int ptOutput;
	UserString command;
	UserString name;

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

struct RuleInstance {
	std::vector<UserString> input;
	std::vector<UserString> output;
	std::vector<UserString> dependencies;
	UserString rule_name;
	RuleDef *rule;

	RuleInstance()
		: rule(NULL)
	{}

	void dump( FILE *f ) const
	{
		fprintf( f, "do %s i=(", rule_name.c_str() );
		strvec_dump_sl( f, " ", input );
		fprintf( f, ") o=(" );
		strvec_dump_sl( f, " ", output );
		fprintf( f, ") d=(" );
		strvec_dump_sl( f, " ", dependencies );
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
	std::vector<RuleInstance *> ri;
	std::vector<UserString> defaults;
	
	std::vector<UserString> subdirs;
	std::vector<KFile *> subparts;

	KFile()
		: parent(NULL)
	{}

	Env *e() { return &env; }

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

	RuleInstance *find_ri_output( const std::string &name )
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
};

struct Target {
	KFile *kf;
	RuleInstance *ri;
	Target()
	{
		kf = NULL;
		ri = NULL;
	}
	operator bool ()
	{
		return kf && ri;
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

std::string env_expand( K::Env *env, const std::string &str )
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

std::string subm_to_str( const char *str, const regmatch_t &m )
{
	if (m.rm_so != -1 && m.rm_eo != -1) {
		std::string s( str + m.rm_so, m.rm_eo - m.rm_so );
		return s;
	}
	else {
		return std::string();
	}
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

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems, bool skip_empty = false) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		if (skip_empty && item.length() == 0)
			continue;
		elems.push_back(item);
	}
	return elems;
}

std::string basename( const std::string &s )
{
	size_t eo, so;
	eo = s.length()-1;
	while (s[eo] == '/') {
		eo--;
	}
	so = eo-1;
	while (so > 0 && s[so] != '/') {
		so--;
	}
	return s.substr( so+1, eo-so+1 );
}

std::string dirname( const std::string &s )
{
	size_t eo;
	eo = s.length()-1;
	while (s[eo] == '/') {
		eo--;
	}
	while (eo > 0 && s[eo] != '/') {
		eo--;
	}
	return s.substr( 0, eo );
}

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
		kf->subdirs.push_back( K::UserString( kf->e(), subdir ) );
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
		kf->defaults.push_back( K::UserString( kf->e(), defl ) );
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
		rd->name.set( kf->e(), name );
		clean_str( ccc );
		rd->command.set( kf->e(), ccc );
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

		K::RuleInstance *ri = new K::RuleInstance;
		ri->rule_name.set( kf->e(), name );

		std::vector<std::string> temp;

		temp.clear();
		split( ddd, ' ', temp );
		for (i=0;i<temp.size();i++)
			ri->dependencies.push_back( K::UserString( kf->e(), temp[i] ) );

		temp.clear();
		split( iii, ' ', temp );
		for (i=0;i<temp.size();i++)
			ri->input.push_back( K::UserString( kf->e(), temp[i] ) );

		temp.clear();
		split( ooo, ' ', temp );
		for (i=0;i<temp.size();i++)
			ri->output.push_back( K::UserString( kf->e(), temp[i] ) );

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

int kfile_load( const std::string &fn, K::KFile *kf )
{
	FILE *f;
	char *line_buf = 0;
	size_t buf_len = 0;
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
	return 0;
}

void strvec_dump( FILE *f, const char *prefix, const std::vector<std::string> &v )
{
	size_t i;
	for (i=0; i<v.size(); i++) {
		fprintf( f, "%s%s\n", prefix, v[i].c_str() );
	}
}

void strvec_dump_sl( FILE *f, const char *delim, const std::vector<std::string> &v )
{
	const char *sep;
	size_t i;
	sep = "";
	for (i=0; i<v.size(); i++) {
		fprintf( f, "%s%s", sep, v[i].c_str() );
		sep = delim;
	}
}

void strvec_dump( FILE *f, const char *prefix, const std::vector<K::UserString> &v )
{
	size_t i;
	for (i=0; i<v.size(); i++) {
		fprintf( f, "%s%s\n", prefix, v[i].value.c_str() );
	}
}

void strvec_dump_sl( FILE *f, const char *delim, const std::vector<K::UserString> &v )
{
	const char *sep;
	size_t i;
	sep = "";
	for (i=0; i<v.size(); i++) {
		fprintf( f, "%s%s", sep, v[i].value.c_str() );
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
	for (i=0; i<kf->rd.size(); i++) {
		kf->rd[i]->dump( f );
	}
	for (i=0; i<kf->ri.size(); i++) {
		kf->ri[i]->dump( f );
	}
}

K::KFile *k_load_sub( const std::string &dir, K::KFile *parent )
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


	if (kf->parent == NULL) {
		kf->env.set( "root", kf->absdirname );
	}

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
		std::string n = dir + "/" + kf->subdirs[i].str();
		//printf( "recurse %s\n", n.c_str() );
		part = k_load_sub( n, kf );
		kf->subparts.push_back( part );
	}

	return kf;
}

K::K *k_load( const std::string &root_dir )
{
	K::K *k = new K::K;
	k->root_dir = root_dir;
	k->root_kfile = k_load_sub( k->root_dir, 0 );
	return k;
}

K::Target k_find_target_rel( K::KFile *kf, const std::string &target )
{
	K::Target ret;
	//K::KFile *kf;
	size_t i;
	std::vector<std::string> splinters;

	split( target, '/', splinters, true );

	//kf = k->root_kfile;
	for (i = 0; i < splinters.size()-1; i++) {
		kf = kf->findsubfile( splinters[i] );
		if (kf == NULL)
			return ret;
	}
	if ((ret.ri = kf->find_ri_output( splinters[splinters.size()-1] ))) {
		ret.kf = kf;
		return ret;
	}

	return ret;
}

K::Target k_find_target_abs( K::KFile *kf, const std::string &target )
{
	K::Target ret;
	if ((ret.ri = kf->find_ri_output( target ))) {
		ret.kf = kf;
		return ret;
	}
	else {
		for (size_t i = 0; i < kf->subparts.size(); i++) {
			ret = k_find_target_abs( kf->subparts[i], target );
			if (ret)
				return ret;
		}
	}
	return ret;
}


K::Target kfile_find_target( K::KFile *kf, const std::string &target )
{
	K::Target ret;
	ret = k_find_target_rel( kf, target );
	if (!ret)
		ret = k_find_target_abs( kf, target );
	return ret;
}

K::Target k_find_target( K::K *k, const std::string &target )
{
	return kfile_find_target( k->root_kfile, target );
}

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

void check_make( char *prg )
{
	if (system( "make -q" )) {
		if (system( "make" )==0) {
			execl( prg, prg, NULL );
		}
		else {
			exit(1);
		}
	}
}

struct inp_typ {
	K::KFile *kf;
	std::string target;
	inp_typ( K::KFile *kf_, const std::string &tg )
		: kf(kf_), target(tg)
	{};
};

bool str_has_char( const std::string &s, char ch )
{
	char b[2];
	std::string::size_type p;
	b[0] = ch; b[1] = 0;
	p = s.find_first_of( b );
	return (p != std::string::npos);
}

/* =========================================================== */

/* =========================================================== */

/* =========================================================== */

int main( int argc, char **argv )
{
	char buf[4096];
	std::string root_dir;
	getcwd( buf, sizeof(buf) );

	check_make(argv[0]);

	root_dir = std::string(buf);
	root_dir += "/";
	root_dir += "ktor/test";

	K::K *k = k_load( root_dir );

	std::list<inp_typ> inp;
	std::vector<inp_typ> tl;
	std::vector<std::string> not_found;

	for (size_t i=0; i<k->root_kfile->defaults.size(); i++) {
		inp.push_back( inp_typ( k->root_kfile, k->root_kfile->defaults[i].str() ) );
	}

	while (inp.size()) {
		inp_typ t = inp.front();
		inp.pop_front();

		tl.push_back( t );
		printf( "looking for target: %s\n", t.target.c_str() );
		K::Target tgt = kfile_find_target( t.kf, t.target );
		if (tgt) {
			printf( "found under [%s]\n", tgt.kf->dirname.c_str() );
			printf( "rule is [%s]\n", tgt.ri->rule_name.c_str() );
			if (!tgt.ri->rule)
				tgt.ri->rule = tgt.kf->find_rule_def( tgt.ri->rule_name.str() );
			printf( "command is [%s]\n", tgt.ri->rule ? tgt.ri->rule->command.c_str() : "UNKNOWN" );
			printf( "list of inputs:\n" );
			for (size_t i=0; i<tgt.ri->input.size(); i++) {
				std::string tn;
				if (str_has_char( tgt.ri->input[i].str(), '/' )) {
					// abs target
					tn = tgt.ri->input[i].str();
					inp.push_back( inp_typ( k->root_kfile, tn ) );
					printf( "\tabs: %s\n", tn.c_str() );
				}
				else {
					// rel target
					tn = tgt.kf->dirname + "/" + tgt.ri->input[i].str();
					inp.push_back( inp_typ( tgt.kf, tgt.ri->input[i].str() ) );
					printf( "\trel: %s\n", tn.c_str() );
				}
			}
		}
		else {
			printf( "target [%s] wasn't found (looked in: [%s])\n", t.target.c_str(), t.kf->dirname.c_str() );
			not_found.push_back( t.kf->dirname + "/" + t.target );
		}
	}
	strvec_dump( stdout, "not_found:   ", not_found );
	return 0;
}

