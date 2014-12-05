
// includes {{{1

#include <map>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <sstream>
#include <utility>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>

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

// k loading {{{2

void k_build_tree( K::K *k, K::InvocMap &im );

K::K *k_load( const std::string &root_dir )
{
	std::list< K::KFile * > kfl;
	K::K *k = new K::K;
	k->root_dir = root_dir;
	k->root_kfile = kfile_load_sub( k->root_dir, 0 );
	if (!k->root_kfile) {
		delete k;
		return NULL;
	}
	k_build_tree( k, k->im );

	kfl.push_back( k->root_kfile );
	k->kfm[ std::string() ] = k->root_kfile;
	while ( not kfl.empty() ) {
		K::KFile *kf = kfl.front();
		kfl.pop_front();
		k->kfm[ kf->dirname ] = kf;
		for (auto p : kf->subparts)
			kfl.push_back( p );
	}

	return k;
}

// target utility {{{2

void k_dump_r( K::KFile *kf )
{
	if (kf) {
		kfile_dump( kf );
		for (size_t i=0; i<kf->subdirs.size(); i++) {
			printf( "\t%s\n", kf->subdirs[i].c_str() );
			k_dump_r( kf->subparts[i] );
		}
	}
}

void k_dump( K::K *k )
{
	printf( "root is %s\n", k->root_dir.c_str() );
	printf( "root kf dir %s\n", k->root_kfile->dirname.c_str() );
	k_dump_r( k->root_kfile );
}

static std::string kfile_target_name_( const std::string &root, const std::string &name )
{
	std::string fn;
	if (name[0] == '/')
		fn = name;
	else if (not str_has_char( name, '/' ))
		fn = (root == ".")
			? name
			: root + "/" + name;
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

/*
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
			set.insert( kfile_target_fname( kf, v[j]) );
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
*/

void kfile_fill_target_map( K::KFile *kf, K::InvocMap &im )
{
	size_t id = 1;
	// RIs of the file.
	for (K::RuleInvoc *&ri : kf->ri) {
		if (ri->output.empty())
			continue;
		K::InvocTree *tmp = new K::InvocTree;
		tmp->id = id++;
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
		if (ptr != im.om.end()) { // we depend on it.
			if (it->prereq.insert( ptr->second ).second)
				it->prereq_num++;
		}
		else {	// it must be a source file.
			im.src.insert( ifn );
		}
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

K::KFile *k_find_kfile( K::KFile *kf, const std::string &n )
{
	if (not kf)
		return kf;
	
	StringVecType dirs;
	split( n, '/', dirs, true );

	for (const std::string &s : dirs) {
		kf = kf->findsubfile( s );
		if (not kf)
			break;
	}
	return kf;
}

K::KFile *k_find_kfile( K::K *k, const std::string &n )
{
	if (n.empty()) return k->root_kfile;

	auto p = k->kfm.find( n );
	if (p != k->kfm.end())
		return p->second;
	else
		return k_find_kfile( k->root_kfile, n );
}

bool k_find_single_target( K::K *k, const std::string &target, K::KFile **pkf, K::InvocTree **ptt )
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
	t = chop_dir_front( target, k->root_dir );

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
	K::KFile *tmp = k_find_kfile( k, t );
	if (!tmp) {
		if (V) printf( "XXX3 %s\n", t.c_str() );
		return false;
	}
	if (tmp) {
		if (V) printf( "YYY %s :: found kfile\n", t.c_str() );
		*pkf = tmp;
		return true;
	}

	return false;
}

bool k_fill_job_queue_sub( K::InvocTree *it, K::JobQueue &jq )
{
	if (jq.visited.count( it )) // has node been already visited?
		return true;

	jq.visited.insert( it );

	if (not it->prereq.empty()) {
		for (K::InvocTree * const&it2 : it->prereq) {
			bool ret = k_fill_job_queue_sub( it2, jq );
			if (!ret) return false;
		}
	}
	jq.queue.push_back( it );
	return true;
}

bool k_expand_user_target_literal_string( K::K *k, const std::string &tg_, std::vector< std::string > &strv )
{
	std::string tg;
	K::KFile *kf = NULL;

	bool tg_all_local = false;
	bool tg_all_rec = false;
	bool tg_default = false;
	bool tg_precise = false;

	tg = tg_;

	if (ends_with( tg, "%%" ) || ends_with( tg, "**" )) {
		tg_all_rec = true;
		tg.resize( tg.size()-2 );
	}
	else if (ends_with( tg, "%" ) || ends_with( tg, "*" )) {
		tg_all_local = true;
		tg.resize( tg.size()-1 );
	}
	else if (ends_with( tg, "/-" )) {
		tg.resize( tg.size()-2 );

		kf = k_find_kfile( k, tg );

		if (kf)
			tg_default = true;
		else
			tg_all_local = true;
	}
	else {
		kf = k_find_kfile( k, tg );
		if (kf)
			tg_default = true;
		else
			tg_precise = true;
	}

	if (tg_default) {
		printf( "taking defaults from '%s'\n", tg.c_str() );
		for (const std::string &s : kf->defaults) {
			std::string tn = kfile_target_fname( kf, s );
			auto p = k->im.om.find( tn );
			if (p!=k->im.om.end()) {
				strv.push_back( tn );
			}
			else {
				fprintf( stderr, "bad default: %s\n", tn.c_str() );
			}
		}
	}
	else if (tg_precise) {
		printf( "taking precise match '%s'\n", tg.c_str() );
		auto p = k->im.om.find( tg );
		if (p != k->im.om.end())
			strv.push_back( tg );
	}
	else {
		printf( "taking prefix match '%s'\n", tg.c_str() );
		auto EE = k->im.om.end();
		auto I = k->im.om.lower_bound( tg );
		
		for ( ; I!=EE; ++I ) {
			const std::string &s = I->first;
			if (!begins_with( s, tg ))
				break;
			if (tg_all_rec) {
				strv.push_back( s );
			}
			else if (tg_all_local) {
				size_t x = s.find_first_of( '/', tg.size() );
				if (x == std::string::npos) // no '/' found
					strv.push_back( s );
			}
		}
	}

	return not(strv.empty());
}

bool k_expand_user_target_string( K::K *k, const std::string &tg_, StringVecType &strv )
{
	//fprintf( stderr, "expanding target: [%s]\n\troot: [%s]\n", tg_.c_str(), k->root_dir.c_str() );
	k_expand_user_target_literal_string( k, tg_, strv );
	if (begins_with( tg_, k->root_dir )) {
		std::string s = chop_dir_front( tg_, k->root_dir );
		k_expand_user_target_literal_string( k, s, strv );
	}
	return not(strv.empty());
}

bool k_expand_user_target( K::K *k, const std::string &tg_, std::vector< K::InvocTree * > &vri )
{
	StringVecType strv;
	std::set< K::InvocTree * > stt;

	if (k_expand_user_target_string( k, tg_, strv )) {
		for ( const std::string &s : strv ) {
			auto p = k->im.om.find( s );
			if (p != k->im.om.end())
				stt.insert( p->second );
		}
		vri.insert( vri.end(), stt.begin(), stt.end() );
	}

	return not(vri.empty());
}


bool k_fill_target_job_queue_sub( K::K *k, const std::string &target, K::JobQueue &jq )
{
	K::KFile *kf;
	K::InvocTree *tt;
	std::vector< K::InvocTree * > vri;
	bool ret = true;
	
	if (1) {
		vri.clear();
		if (k_expand_user_target( k, target, vri )) {
			for (auto p : vri) {
				if (not k_fill_job_queue_sub( p, jq )) {
					fprintf( stderr, "target %s has problem\n", target.c_str() );
					ret = false;
				}
			}
		}
		else {
			fprintf( stderr, "x/target %s is not found\n", target.c_str() );
			ret = false;
		}
		return ret;
	}
	else {
		bool b = k_find_single_target( k, target, &kf, &tt );

		if (b && kf) {
			for (const std::string &dt : kf->defaults) {
				std::string tmp = kfile_target_fname( kf, dt );
				b = k_fill_target_job_queue_sub( k, tmp, jq );
				if (!b) {
					printf( "XYZ bailing on %s\n", tmp.c_str() );
					return false;
				}
			}
			return true;
		}
		else if (b && tt) {
			return k_fill_job_queue_sub( tt, jq );
		}
		else {
			fprintf( stderr, "target %s is not found\n", target.c_str() );
			return false;
		}
	}
}

bool k_fill_target_job_queue( K::K *k, const std::string &target, K::JobQueue &jq )
{
	bool b;
	b = k_fill_target_job_queue_sub( k, target, jq );
	if (!b) return b;

	for (K::InvocTree * const&x : jq.visited) {
		x->pending_num = x->prereq_num;
		jq.jm[ x->pending_num ].insert( x );
	}

	return true;
}

std::string kinvoctree_get_command( K::InvocTree *it, bool add_cd = true )
{
	std::string cmd;

	if (add_cd) {
		cmd += "cd ";
		cmd += it->kf->absdirname;
		cmd += " && ( ";
	}

	cmd += it->ri->command;

	if (add_cd) {
		cmd += " )";
	}

	return cmd;
}

bool kinvoctree_need_build_mtime( K::InvocTree *it, bool *err )
{
	struct timespec ts;
	struct timespec ts_input;
	struct timespec ts_output;

	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	if (err) *err = false;

	for ( auto s : it->ri->input ) {
		std::string fn = kfile_target_afname( it->kf, s );
		bool ok = is_younger_mtime( fn, ts );
		if (not ok) {
			if (err) *err = true;
			fprintf( stderr, "error stating file: [%s]\n", fn.c_str() );
			return true;
		}
		//fprintf( stderr, "  i : %ld.%09ld : %s\n", ts.tv_sec, ts.tv_nsec, fn.c_str() );
	}
	for ( auto s : it->ri->deps ) {
		std::string fn = kfile_target_afname( it->kf, s );
		bool ok = is_younger_mtime( fn, ts );
		if (not ok) {
			if (err) *err = true;
			fprintf( stderr, "error stating file: [%s]\n", fn.c_str() );
			return true;
		}
		//fprintf( stderr, "  d : %ld.%09ld : %s\n", ts.tv_sec, ts.tv_nsec, fn.c_str() );
	}

	ts_input = ts;

	ts.tv_sec = LONG_MAX;
	ts.tv_nsec = 1*1000*1000*1000 - 1;

	for ( auto s : it->ri->output ) {
		std::string fn = kfile_target_afname( it->kf, s );
		bool ok = is_older_mtime( fn, ts );
		if (not ok) {
			return true; // output is missing, rebuild
		}
		//fprintf( stderr, "  o : %ld.%09ld : %s\n", ts.tv_sec, ts.tv_nsec, fn.c_str() );
	}

	ts_output = ts;

	if (0) fprintf( stderr, "  i=%ld.%09ld o=%ld.%09ld\n",
			ts_input.tv_sec, ts_input.tv_nsec,
			ts_output.tv_sec, ts_output.tv_nsec
			);

	return is_file_younger( ts_input, ts_output );
}

bool kinvoctree_need_build( K::InvocTree *it, bool *err )
{
	return kinvoctree_need_build_mtime( it, err );
}

pid_t kinvoctree_invoke_bg( K::InvocTree *it )
{
	pid_t pid = -1;
	int rc;
	const char *cmd[4] = {};
	const char *black_list = ";&\\\"\'{}()<>";
	bool no_shell = false;

	pid = fork();
	if (pid == -1) {
		return pid;
	}

	if (pid == 0) {
		// child
		rc = chdir( it->kf->absdirname.c_str() );
		if (rc != 0) {
			fprintf( stderr, "failed to chdir(\"%s\"): %d (%s)\n", it->kf->absdirname.c_str(), errno, strerror(errno) );
			exit(100);
		}

		if (strpbrk( it->ri->command.c_str(), black_list)) {
			// has black listed characters - run via shell
			cmd[0] = "/bin/sh";
			cmd[1] = "-c";
			cmd[2] = it->ri->command.c_str();
			execv( cmd[0], (char **)cmd );
		}
		else {
			// no black-listed characters - run directly.
			no_shell = true;
			std::vector<char *> pp;
			split( it->ri->command, "", pp, true );
			pp.push_back( NULL );
			execvp( pp[0], &pp[0] );
		}

		fprintf( stderr, "failed to exec(\"%s%s\"): %d (%s)\n",
				no_shell ? "" : "/bin/sh -c ",
				it->ri->command.c_str(),
				errno, strerror(errno) );
		exit(101);
	}
	// parent
	return pid;
}

bool kinvoctree_invoke( K::InvocTree *it )
{
	pid_t pid = kinvoctree_invoke_bg( it );
	if (pid == -1) {
		fprintf( stderr, "failed to start the job.\n" );
		return false;
	}
	else {
		pid_t wpid;
		int status = 0;
		do
			wpid = waitpid( pid, &status, 0 );
		while (wpid == -1 && errno == EINTR);
		return (wpid == pid && status == 0);
	}
}

K::InvocTree *kjobqueue_pop( K::JobQueue &jq )
{
	std::set< K::InvocTree * > &jm0 = jq.jm[0];
	if (not jm0.empty()) {
		K::InvocTree *it;
		it = *(jm0.begin());
		jm0.erase( jm0.begin() );
		return it;
	}
	else {
		return NULL;
	}
}

void kjobqueue_finish( K::JobQueue &jq, K::InvocTree *it )
{
	for (auto it2 : it->contrib) {
		size_t pn = it2->pending_num;
		//assert( pn > 0 );
		if (pn > 0) {
			assert( jq.jm[pn].count( it2 ) > 0 );
			jq.jm[pn].erase( it2 );
			if (jq.jm[pn].empty())
				jq.jm.erase( pn );
			it2->pending_num = --pn;
			jq.jm[pn].insert( it2 );
		}
	}
}

// test to see whether anything needs building.
// true == need rebuild
bool k_test( K::K *k, const std::string &target )
{
	bool ret;
	K::JobQueue &jq = k->jq;

	fprintf( stderr, "testing target: %s\n", target.c_str() );
	
	ret = k_fill_target_job_queue( k, target, jq );

	if (!ret) {
		fprintf( stderr, "target %s: failed to create command queue\n", target.c_str() );
		return true;
	}

	for (K::InvocTree *&it : jq.queue) {
		if (not kinvoctree_need_build( it, 0 ))
			continue;
		return true;
	}
	return false;
}

bool k_build( K::K *k, const std::string &target, int max_jobs = 1)
{
	bool ret;
	K::JobQueue &jq = k->jq;

	//fprintf( stderr, "building target: %s\n", target.c_str() );
	
	ret = k_fill_target_job_queue( k, target, jq );

	if (!ret) {
		fprintf( stderr, "target %s: failed to create command queue\n", target.c_str() );
		return ret;
	}

	if (max_jobs <= 1) {
		for (K::InvocTree *&it : jq.queue) {
			if (not kinvoctree_need_build( it, 0 ))
				continue;
			fprintf( stderr, "[%s/] %s\n", it->kf->dirname.c_str(), it->ri->command.c_str() );
			if (not kinvoctree_invoke( it ))
				return false;
		}
		return true;
	}
	else {
		int slots = max_jobs;
		std::list< std::pair< pid_t, K::InvocTree *> > pit;
		for (;;) {
			while (slots > 0) {
				K::InvocTree *it = kjobqueue_pop( jq );
				if (not it)
					break;

				if (not kinvoctree_need_build( it, 0 )) {
					kjobqueue_finish( jq, it );
					continue;
				}

				pid_t pid = kinvoctree_invoke_bg( it );
				if (pid>=0) {
					fprintf( stderr, "started task, pid %ld (%s)\n", 
							(long)pid, it->ri->command.c_str() );
					pit.push_back( std::make_pair( pid, it ) );
					slots--;
				}
				else {
					fprintf( stderr, "failed to start job\n" );
					exit(1);
				}
			}

			fprintf( stderr, "running %d jobs\n", max_jobs - slots );

			if (not pit.empty()) {
				int status;
				pid_t pid;

				do
					pid = waitpid( -1, &status, 0 );
				while (pid == -1 && errno == EINTR);

				if (pid == -1) {
					fprintf( stderr, "wait() failed\n" );
					exit(1);
				}

				slots++;

				K::InvocTree *it = NULL;
				auto I = pit.begin();
				auto E = pit.end();
				for ( ; I!=E; ++I) {
					if ((*I).first == pid) {
						it = (*I).second;
						pit.erase( I );
						break;
					}
				}
				if (it) {
					if (status) {
						fprintf( stderr, "task %ld failed: %d,%d,%d\n",
								(long)pid, status&0x7f, !!(status&0x80), status>>8 );
						exit(1);
					}

					fprintf( stderr, "task %ld finished, contributes to %lu\n",
							(long)pid, (long unsigned)it->contrib.size() );
					kjobqueue_finish( jq, it );
				}
				else {
					fprintf( stderr, "unknown pid %ld\n", (long)pid );
					exit(1);
				}
			}
			else {
				if (jq.jm.empty() || (jq.jm.size() == 1 && jq.jm.count(0) == 1 && jq.jm[0].empty())) {
					fprintf( stderr, "all done\n" );
					return true;
				}
				else {
					fprintf( stderr, "no jobs were started; no jobs to wait for; bailing out\n" );
					exit(1);
				}
			}
		}
	}

	return true;
}

bool k_clean( K::K *k )
{
	K::JobQueue jq;
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
			ret = delete_file( afn.c_str() );
		}
	}
	return ret;
}

bool k_query( K::K *k, const std::string &target )
{
	StringVecType strv;
	std::set< std::string > strs;
	size_t count = 0;

	if (k_expand_user_target_string( k, target, strv )) {
		printf( "[%s]\n", target.c_str() );
		for (const std::string &str : strv) {
			if (strs.count(str))
				continue;
			printf( "\t%s\n", str.c_str() );
			strs.insert( str );
			count++;
		}
	}
	else {
		printf( "[%s] - no matches!\n", target.c_str() );
	}
	return (count > 0);
}

void k_print( K::K *k, const std::string &target )
{
	bool ret = k_fill_target_job_queue( k, target, k->jq );
	if (ret) {
		printf( "commands\n" );
		for (K::InvocTree *&it : k->jq.queue) {
			std::string cmd = kinvoctree_get_command( it );
			printf( "\t%s\n", cmd.c_str() );
		}
		printf( "\n" );
		printf( "om\n" );
		for (auto x : k->im.om) {
			printf( "\t%s\n", x.first.c_str() );
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

	switch (opts.command) {
	case K::KOpt::CMD_TEST:
	case K::KOpt::CMD_BUILD:
	case K::KOpt::CMD_PRINT:
	case K::KOpt::CMD_CLEAN:
	case K::KOpt::CMD_DUMP:
	case K::KOpt::CMD_QUERY:
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
				b = k_test( k, afn );
				if (b) {
					fprintf( stderr, "target %s: need rebuild.\n", afn.c_str() );
					rc = 1;
				}
				break;
			case K::KOpt::CMD_BUILD:
				fprintf( stderr, "building %s\n", afn.c_str() );
				b = k_build( k, afn, opts.jobs );
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
			case K::KOpt::CMD_QUERY:
				b = k_query( k, afn );
				if (not b) {
					rc = 1;
				}
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

	return 0;
}

// vim:foldmethod=marker:foldopen+=jump:
