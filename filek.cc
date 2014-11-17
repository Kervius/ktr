
#include <string>

#include <sys/types.h>
#include <regex.h>

#include "k.hh"
#include "utilk.hh"

static std::string subm_to_str( const char *str, const regmatch_t &m )
{
	if (m.rm_so != -1 && m.rm_eo != -1)
		return std::string( str + m.rm_so, m.rm_eo - m.rm_so );
	else
		return std::string();
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

void kfile_dump( K::KFile *kf, FILE *f )
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

