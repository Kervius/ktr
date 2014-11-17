#ifndef K__________
#define K__________

#include <string>
#include <vector>
#include <set>
#include <map>

namespace K {
	struct KOpt {
		enum Cmd {
			CMD_NONE,
			CMD_TEST,
			CMD_BUILD,
			CMD_CLEAN,
			CMD_PRINT,
			CMD_DUMP,
			CMD_VERSION,
			CMD_USAGE,
		};
		enum VerboseLevel {
			VL_DEAD = 0,
			VL_SILENT = 1,
			VL_INFO = 2,
			VL_TRACE = 3,
			VL_DEBUG = 4,
		};
		Cmd command;
		bool check;
		bool force;
		int jobs;
		int verbose_level;
		std::vector<std::string> targets;
		std::string kfile;
		std::string kpart;

		KOpt()
		:	
			command(CMD_NONE),
			check(true),
			force(false),
			jobs(1),
			verbose_level(VL_INFO)
		{
			kfile = "kfile";
			kpart = "kpart";
		}
	};
};

namespace K {
struct Env;
struct RuleInvoc;
struct KFile;
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

struct InvocTree {
	size_t id;
	RuleInvoc *ri = NULL;
	KFile *kf = NULL;
	std::set<InvocTree *> prereq;
	std::set<InvocTree *> contrib;
	size_t prereq_num = 0;
	size_t pending_num = 0;

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
	std::map<size_t, std::set<K::InvocTree *>> jm;
};


struct K {
	std::string root_dir;
	KFile *root_kfile;
	InvocMap im;
	JobQueue jq;
};

}

int kmain(int argc, char **argv, K::KOpt &opts);


/*
int kfile_line_var( const char *inp, K::KFile *kf );
int kfile_line_subdir( const char *inp, K::KFile *kf );
int kfile_line_default( const char *inp, K::KFile *kf );
int kfile_line_rule_def( const char *inp, K::KFile *kf );
int kfile_line_do( const char *inp, K::KFile *kf );
int kfile_line_empty( const char *inp );
int kfile_load( const std::string &fn, K::KFile *kf ); */
void kfile_dump( K::KFile *kf, FILE *f = NULL );
K::KFile *kfile_load_sub( const std::string &dir, K::KFile *parent );

#endif /* K__________ */
