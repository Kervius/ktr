
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

#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>
#include <unistd.h>

#include "k.hh"
#include "utilk.hh"

// declarations {{{1

static std::string kfile_target_fname( K::KFile *kf, const std::string &name );
static std::string kfile_target_afname( K::KFile *kf, const std::string &name );


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

void k_build_tree( K::K *k, K::InvocMap &im );

K::K *k_load( const std::string &root_dir )
{
	K::K *k = new K::K;
	k->root_dir = root_dir;
	k->root_kfile = kfile_load_sub( k->root_dir, 0 );
	if (!k->root_kfile) {
		delete k;
		return NULL;
	}
	k_build_tree( k, k->im );
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

std::string kfile_target_name_( const std::string &root, const std::string &name )
{
	std::string fn;
	if (name[0] == '/')
		fn = name;
	else if (not str_has_char( name, '/' ))
		fn = root + "/" + name;
	else
		fn = name;
	return fn;
}


std::string kfile_target_fname( K::KFile *kf, const std::string &name )
{
	return kfile_target_name_( kf->dirname, name );
}

std::string kfile_target_afname( K::KFile *kf, const std::string &name )
{
	return kfile_target_name_( kf->absdirname, name );
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
			std::string fn = kfile_target_fname( kf, n );
			im.om[ fn ] = tmp;
		}
	}
	// RIs of the subparts
	for (K::KFile *&skf : kf->subparts)
		kfile_fill_target_map( skf, im );
}

void kinvoctree_fill_prereq_vec( K::InvocMap &im, K::InvocTree *it, StringVecType &inputs )
{
	for (std::string &ii : inputs) {
		std::string ifn = kfile_target_fname( it->kf, ii );
		auto ptr = im.om.find(ifn);
		// have we found an output which is our input?
		if (ptr != im.om.end()) // we depend on it.
			it->prereq.insert( ptr->second );
		else	// it must be a source file.
			im.src.insert( ifn );
	}
}

void kinvoctree_fill_prereq( K::InvocMap &im )
{
	for (K::InvocTree *&it : im.tt) {
		kinvoctree_fill_prereq_vec( im, it, it->ri->input );
		kinvoctree_fill_prereq_vec( im, it, it->ri->deps );
	}
}

void kinvoctree_fill_contrib( K::InvocMap &im )
{
	// a requires b -> b contibutes to a
	// when b is build, it can notify a
	for (K::InvocTree *const&a : im.tt)
		for (K::InvocTree *const&b : a->prereq)
			b->contrib.insert( a );
}

void k_build_tree( K::K *k, K::InvocMap &im )
{
	kfile_fill_target_map( k->root_kfile, im );
	kinvoctree_fill_prereq( im );
	kinvoctree_fill_contrib( im );
}


bool k_find_target( K::K *k, const std::string &target, K::KFile **pkf, K::InvocTree **ptt )
{
	const bool V = false;
	*pkf = NULL;
	*ptt = NULL;

	if (V) printf( "XXX1 %s\n", target.c_str() );

	// probably it is already in the map?
	auto p = k->im.om.find( target );
	if (p != k->im.om.end()) {
		*ptt = p->second;
		if (V) printf( "YYY %s :: found in om\n", target.c_str() );
		return true;
	}

	std::string t;
	if (target[0] == '/') {
		if (target.compare( 0, k->root_dir.length(), k->root_dir) == 0) {
			if (target.length() > k->root_dir.length() && target[k->root_dir.length()] == '/')
				t = target.substr( k->root_dir.length()+1 );
			else
				t = target.substr( k->root_dir.length() );
		}
	}
	else {
		t = target;
	}

	if (t.empty()) {
		if (V) printf( "YYY %s :: is a root\n", target.c_str() );
		*pkf = k->root_kfile;
		return true;
	}

	if (V) printf( "XXX2 %s\n", t.c_str() );

	// probably it is a relative path?
	p = k->im.om.find( t );
	if (p != k->im.om.end()) {
		if (V) printf( "YYY %s :: found2 in om\n", t.c_str() );
		*ptt = p->second;
		return true;
	}

	// find kfile
	StringVecType dirs;
	K::KFile *kf = k->root_kfile;
	split( t, '/', dirs );
	for (const std::string &s : dirs) {
		size_t i;
		for (i = 0; i<kf->subdirs.size(); i++) {
			if (kf->subdirs[i] == s)
				break;
		}
		if (i>=kf->subdirs.size()) {
			if (V) printf( "XXX3 %s : %s\n", t.c_str(), s.c_str() );
			return false;
		}
		kf = kf->subparts[i];
	}
	if (kf) {
		if (V) printf( "YYY %s :: found kfile\n", t.c_str() );
		*pkf = kf;
		return true;
	}
	return false;
}

bool k_fill_job_queue( K::InvocTree *it, K::JobQueue &jq )
{
	if (jq.visited.count( it )) // has node been already visited?
		return true;

	jq.visited.insert( it );

	if (not it->prereq.empty()) {
		for (K::InvocTree * const&it2 : it->prereq) {
			bool ret = k_fill_job_queue( it2, jq );
			if (!ret) return false;
		}
	}
	jq.queue.push_back( it );
	return true;
}

bool k_fill_target_job_queue( K::K *k, const std::string &target, K::JobQueue &jq )
{
	K::KFile *kf;
	K::InvocTree *tt;
	bool b = k_find_target( k, target, &kf, &tt );

	if (b && kf) {
		for (const std::string &dt : kf->defaults) {
			std::string tmp = kfile_target_fname( kf, dt );
			b = k_fill_target_job_queue( k, tmp, jq );
			if (!b) {
				printf( "XYZ bailing on %s\n", tmp.c_str() );
				return false;
			}
		}
		return true;
	}
	else if (b && tt) {
		return k_fill_job_queue( tt, jq );
	}
	else {
		fprintf( stderr, "target %s is not found\n", target.c_str() );
		return false;
	}
}

std::string kinvoctree_get_command( K::InvocTree *it )
{
	std::string cmd;

	cmd += "cd ";
	cmd += it->kf->absdirname;
	cmd += " && ( ";
	cmd += it->ri->command;
	cmd += " )";

	return cmd;
}

bool kinvoctree_invoke( K::InvocTree *it )
{
	std::string cmd;
	int ret;

	cmd = kinvoctree_get_command( it );

	fprintf( stderr, "CMD: %s\n", cmd.c_str() );

	ret = system( cmd.c_str() );

	if (ret == -1)
		fprintf( stderr, "failed to start the job.\n" );
	else if (ret)
		fprintf( stderr, "job has failed, rc=%04x\n", ret );

	return ret == 0;
}

bool k_clean( K::K *k )
{
	K::JobQueue jq;
	int rc;
	bool ret = true;
	K::InvocMap &im = k->im;

	for ( auto x : im.om )
		if (not k_fill_target_job_queue( k, x.first, jq ))
			return false;

	// simply delete all products, in the reverse order
	for ( auto I=jq.queue.rbegin(), E=jq.queue.rend(); I!=E; ++I ) {
		for ( const std::string &x : (*I)->ri->output ) {
			std::string afn = kfile_target_afname( (*I)->kf, x );
			fprintf( stderr, "CLEAN: %s\n", afn.c_str() );
			if (not isdir(afn)) {
				rc = unlink( afn.c_str() );
				if (rc == 0 || (rc == -1 && errno == ENOENT)) {
					continue;
				}
				else {
					fprintf( stderr, "cleaning file %s: error: %d (%s)", afn.c_str(), errno, strerror(errno) );
					ret = false;
				}
			}
			else {
				rc = rmdir( afn.c_str() );
				if (rc == 0 || (rc == -1 && errno == ENOENT)) {
					continue;
				}
				else {
					fprintf( stderr, "cleaning dir %s: error: %d (%s)", afn.c_str(), errno, strerror(errno) );
					ret = false;
				}
			}
		}
	}
	return ret;
}

bool k_build( K::K *k, const std::string &target )
{
	bool ret = k_fill_target_job_queue( k, target, k->jq );
	if (ret) {
		for (K::InvocTree *&it : k->jq.queue)
			if (not kinvoctree_invoke( it ))
				return false;
	}
	else {
		fprintf( stderr, "target %s: failed to create command queue\n", target.c_str() );
		return false;
	}
	return true;
}

void k_print( K::K *k, const std::string &target )
{
	bool ret = k_fill_target_job_queue( k, target, k->jq );
	if (ret) {
		for (K::InvocTree *&it : k->jq.queue) {
			std::string cmd = kinvoctree_get_command( it );
			printf( "%s\n", cmd.c_str() );
		}
	}
}


// the main {{{1

/* =========================================================== */

/* =========================================================== */

/* =========================================================== */

int main( int argc, char **argv )
{
	char buf[4096];
	K::KOpt opts;
	int rc;
	std::map<std::string, K::K *> km;

	check_make(argc, argv);
	rc = kmain(argc, argv, opts);
	if (rc) {
		if (opts.command == K::KOpt::CMD_USAGE)
			fprintf( stderr, "usage\n" );
		return rc;
	}

//	if (opts.command != K::KOpt::CMD_BUILD) {
//		fprintf( stderr, "hoopla\n" );
//		return 1;
//	}

	switch (opts.command) {
	case K::KOpt::CMD_TEST:
	case K::KOpt::CMD_BUILD:
	case K::KOpt::CMD_PRINT:
	case K::KOpt::CMD_CLEAN:
	case K::KOpt::CMD_DUMP:
		if (opts.targets.empty()) {
			std::string root_dir;
			getcwd( buf, sizeof(buf) );
			root_dir = std::string(buf);
			root_dir += "/";
			root_dir += "test";
			opts.targets.push_back( root_dir );
		}
		for (std::string &t : opts.targets) {
			K::K *k;
			std::string afn = abs_file_name( t );
			std::string dd = find_file_dir( afn, opts.kfile );
			bool b;
			if (!km.count(dd)) {
				k = k_load( dd );
				km[dd] = k;
			}
			else {
				k = km[dd];
			}
			switch (opts.command) {
			default:
			case K::KOpt::CMD_TEST:
				fprintf( stderr, "testing %s\n", afn.c_str() );
				break;
			case K::KOpt::CMD_BUILD:
				fprintf( stderr, "building %s\n", afn.c_str() );
				b = k_build( k, afn );
				if (not b) {
					fprintf( stderr, "target %s: build failed.\n", t.c_str() );
					rc = 1;
				}
				break;
			case K::KOpt::CMD_PRINT:
				fprintf( stderr, "printing %s\n", afn.c_str() );
				k_print( k, afn );
				break;
			case K::KOpt::CMD_CLEAN:
				fprintf( stderr, "cleaning %s\n", dd.c_str() );
				b = k_clean( k );
				if (not b) {
					fprintf( stderr, "target %s: cleaning failed.\n", t.c_str() );
					rc = 1;
				}
				break;
			case K::KOpt::CMD_DUMP:
				k_dump( k );
				break;
			}
		}
		return rc;
		break;
	case K::KOpt::CMD_VERSION:
		fprintf( stdout, "ktor v0.0.1\n" );
		break;
	case K::KOpt::CMD_USAGE:
		fprintf( stdout, "usage\n" );
		return 0;
	default:
		fprintf( stderr, "usage\n" );
		return 1;
	}

#if 0
	if (0) {
		std::string root_dir;
		getcwd( buf, sizeof(buf) );
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

			if (1) {
				printf( "cleaning:\n" );
				if (not k_clean( k ))
					fprintf( stderr, "cleaning failed.\n" );
			}

			if (1) {
				printf( "build defaults:\n" );
				for (const std::string &d : k->root_kfile->defaults) {
					printf( "%s\n", d.c_str() );

					K::JobQueue jq;
					bool ret = k_fill_target_job_queue( d, im, jq );
					if (ret) {
						//for (K::InvocTree *&it : jq.queue)
						//	printf( "\t%u : %s : %s\n", (unsigned)it->prereq.size(), it->kf->absdirname.c_str(), it->ri->command.c_str() );
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
		}


		if (0) {
			StringVecType a;
			k_find_sources( k, a );
			for (std::string &s : a) {
				printf( "%s/%s\n", k->root_dir.c_str(), s.c_str() );
			}
		}
	}
#endif
	return 0;
}

// vim:foldmethod=marker:foldopen+=jump:
