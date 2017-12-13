
#include "KonsLoader.hh"
#include "Kons.hh"

namespace {

static char SUBST_CHAR = '%';

bool gmake_subst(const std::string& pat, const std::string& repl, const std::string& instr, std::string *outstr)
{
	std::string::size_type pct_pos_pat, pct_pos_repl;

	outstr->clear();

	pct_pos_pat = pat.find( SUBST_CHAR);
	pct_pos_repl = repl.find( SUBST_CHAR );

	if (pct_pos_pat == std::string::npos && pct_pos_repl == std::string::npos) {
		// if both are missing %, but match, then match.
		if (pat == instr) {
			*outstr = instr;
			return true;
		}
		else {
			return false;
		}
	}
	else if (pct_pos_repl == std::string::npos || pct_pos_pat == std::string::npos)
	{
		// error: no % in pattern or replacement
		return false;
	}
	else
	{
		size_t pat_prefix_size = pct_pos_pat;
		size_t pat_suffix_size = pat.size() - (pct_pos_pat+1);
		//printf( "\t\tprefix: [%.*s]\n", (int)pat_prefix_size, pat.c_str() );
		//printf( "\t\tsuffix: [%.*s]\n", (int)pat_suffix_size, pat.c_str() + pct_pos_pat+1 );
		if (instr.size() >= pat_prefix_size + pat_suffix_size) {
			//printf( "\t\tlength check OK\n" );
			if (instr.compare( 0, pat_prefix_size, pat, 0, pat_prefix_size ) == 0) {
				//printf( "\t\tprefix OK\n" );
				if (instr.compare( instr.size() - pat_suffix_size, pat_suffix_size, pat, pct_pos_pat+1, pat_suffix_size ) == 0) {
					//printf( "\t\tsuffix OK\n" );
					*outstr = repl.substr( 0, pct_pos_repl ) +
						instr.substr( pat_prefix_size, instr.size() - (pat_suffix_size + pat_prefix_size)) +
						repl.substr( pct_pos_repl+1 );
					return true;
				}
			}
		}
		return false;
	}
	return false;
}
}

using namespace Ktr;


KonsLoader::
KonsLoader( const std::string& root_dir_ )
: root_dir(root_dir_)
, m(nullptr)
, rtc(new mirtc)
{
	mi_add_user_command( "k:subdir", S_CmdSubdir, (long)this);
	mi_add_user_command( "k:rule", S_CmdRule, (long)this);
	mi_add_user_command( "k:make", S_CmdMake, (long)this);
	mi_add_user_command( "k:do", S_CmdMake, (long)this);
	mi_add_user_command( "k:dump", S_CmdDump, (long)this);
	mi_add_user_command( "k:subst", S_CmdSubst, (long)this);
	mi_add_user_command( "k:var", S_CmdVar, (long)this);
}

KonsLoader::
~KonsLoader()
{
	delete rtc;
	if (m) delete m;
}

bool KonsLoader::
ReadFile( const std::string& file_name, std::string *content )
{
	FILE *ff;
	char buf[16<<10];

	ff = fopen( file_name.c_str(), "r" );

	if (!ff)
		return false;

	content->clear();

	while (not feof(ff)) {
		size_t bytes = fread( buf, 1, sizeof(buf), ff );
		if (!bytes)
			break;
		content->append( buf, bytes );
	}

	fclose( ff );

	return true;
}

bool KonsLoader::
Load()
{
	if (this->m) delete m;
	this->m = new Model( root_dir );
	this->curr_dir = m->dirs->FindDir( std::string() );

	while (1) {
		std::string fn;

		if (curr_dir->dir_name[0] != '/') {	// dir relative to root
			fn = root_dir;
			fn += "/";
		}
		fn += curr_dir->dir_name;
		fn += "/";
		fn += "kfile";

		fprintf( stderr, "... trying: %s\n", fn.c_str() );

		std::string fc;
		if (ReadFile( fn, &fc )) {
			if (not Eval( fc )) {
				fprintf( stderr, "failed to parse: %s\n", fn.c_str() );
				return false;
			}
		}
		else {
			fprintf( stderr, "can't read: %s\n", fn.c_str() );
			return false;
		}

		if (walk_list.empty())
			break;

		DirDescentEntry& next = walk_list.front();
		std::string sub_dir_name;
		if (next.sub_dir[0] == '/') {
			sub_dir_name = next.sub_dir;
		}
		else {
			fprintf( stderr, "%p %s %p %s\n", 
					curr_dir, curr_dir->dir_name.c_str(),
					next.parent_dir, next.parent_dir->dir_name.c_str()
					);
			sub_dir_name = next.parent_dir->dir_name + "/" + next.sub_dir;
		}

		Dir *new_dir = this->m->dirs->AddDir( sub_dir_name );
		if (!new_dir) {
			fprintf( stderr, "can't add dir to model: %s\n", sub_dir_name.c_str() );
			return false;
		}

		walk_list.pop_front();
		this->curr_dir = new_dir;
	}
	return true;
}

bool KonsLoader::
Eval( const std::string& expr )
{
	return (mi_eval( rtc, expr ) == mi_ev_ok);
}

int KonsLoader::
CmdSubdir( const std::vector<std::string>& args )
{
	bool first = true;
	for (auto x : args) {
		if (first) {
			first = false;
			continue;
		}

		const std::string& sub_dir = x;
		walk_list.push_back( DirDescentEntry( curr_dir, sub_dir ) );
	}
	return mi_ev_ok;
}

int KonsLoader::
S_CmdSubdir( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie )
{
	KonsLoader* kl = (KonsLoader*)cookie;
	return kl->CmdSubdir( args );
}

int KonsLoader::
CmdRule( const std::vector<std::string>& args )
{
	std::string rule_name;
	std::string command;
	int num = -1;
	bool skip = false;

	for (auto arg : args) {
		num++;
		if (skip) { skip = false; continue; }
		if (num == 0) continue;

		if (num == 1) {
			rule_name = args[num];
		}
		else {
			//const std::string& arg = args[num];
			if (arg.compare("c=") == 0 || arg.compare("cmd=") == 0) {
				if (num+1 < (int)args.size()) {
					command = args[num+1];
					skip = true;
				}
				else {
					fprintf( stderr, "rule: empty command\n" );
					return mi_ev_error;
				}
			}
			else if (arg.compare(0, 2, "c=")==0) {
				command = arg.substr( 2 );
			}
			else if (arg.compare(0, 4, "cmd=")==0) {
				command = arg.substr( 4 );
			}
			else {
				fprintf( stderr, "rule: unknown token: [%s]\n", arg.c_str() );
				return mi_ev_error;
			}
		}
	}
	if (not rule_name.empty() && not command.empty()) {
		Rule* r = m->rules->AddRule( curr_dir, rule_name );
		if (r) {
			fprintf( stderr, "rule: *** got rule %s %s\n", rule_name.c_str(), command.c_str() );
			r->command = command;
		}
		else {
			return mi_ev_error;
		}
	}
	return mi_ev_ok;
}

int KonsLoader::
S_CmdRule( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie )
{
	KonsLoader* kl = (KonsLoader*)cookie;
	return kl->CmdRule( args );
}


int KonsLoader::
CmdMake( const std::vector<std::string>& args )
{
	std::string rule_name;
	std::vector<std::string> inputs;
	std::vector<std::string> outputs;
	std::vector<std::string> deps;

	std::vector<std::string> temp;

	int num = -1;
	bool skip = false;
	bool has_positional_args = true;

	for (auto arg : args) {
		num++;
		if (skip) { skip = false; continue; }
		if (num == 0) continue;

		if (has_positional_args) {
			if (arg.size() >= 2 && arg[1] == '=')
				has_positional_args = false;
		}


		if (has_positional_args) {
			//fprintf( stderr, "x task: pos arg [%s] \n", arg.c_str() );
			if (num == 1) {
				rule_name = arg;
			}
			else if (num == 2) {
				inputs.push_back( arg );
			}
			else if (num == 3) {
				outputs.push_back( arg );
			}
			else {
				deps.push_back( arg );
			}
			//fprintf( stderr, "x task: adding %d OUTs\n", temp.size() );
		}
		else {
			//fprintf( stderr, "x task: arg [%s] \n", arg.c_str() );
			if (Utils::BeginsWith( arg, "r=" )) {
				if (arg == "r=") {
					rule_name = args[num+1];
					skip = true;
				}
				else {
					rule_name = arg.substr( 2 );
				}
			}
			else if (Utils::BeginsWith( arg, "i=" )) {
				temp.clear();
				if (arg == "i=") {
					Utils::Split( args[num+1], ' ', temp, true );
					skip = true;
				}
				else {
					Utils::Split( arg.substr( 2 ), ' ', temp, true );
				}
				inputs.insert( inputs.end(), temp.begin(), temp.end() );
			}
			else if (Utils::BeginsWith( arg, "o=" )) {
				temp.clear();
				if (arg == "o=") {
					Utils::Split( args[num+1], ' ', temp, true );
					skip = true;
				}
				else {
					Utils::Split( arg.substr( 2 ), ' ', temp, true );
				}
				outputs.insert( outputs.end(), temp.begin(), temp.end() );
				//fprintf( stderr, "x task: adding %d OUTs\n", temp.size() );
			}
			else if (Utils::BeginsWith( arg, "d=" )) {
				temp.clear();
				if (arg == "d=") {
					Utils::Split( args[num+1], ' ', temp, true );
					skip = true;
				}
				else {
					Utils::Split( arg.substr( 2 ), ' ', temp, true );
				}
				deps.insert( deps.end(), temp.begin(), temp.end() );
			}
		}

	}

	if (rule_name.empty() || inputs.empty()) {
		fprintf( stderr, "task: empty\n" );
		return mi_ev_error;
	}

	Task* t = m->tasks->AddTask( curr_dir, rule_name );
	if (t) {
		for ( auto I : inputs ) {
			if (not m->taskObjs->AddTaskObject( curr_dir, t, I, TaskObject::INPUT )) {
				fprintf( stderr, "task: can't add object %s\n", I.c_str() );
			}
		}

		for ( auto I : outputs ) {
			if (not m->taskObjs->AddTaskObject( curr_dir, t, I, TaskObject::OUTPUT )) {
				fprintf( stderr, "task: can't add object %s\n", I.c_str() );
			}
		}

		for ( auto I : deps ) {
			if (not m->taskObjs->AddTaskObject( curr_dir, t, I, TaskObject::DEP )) {
				fprintf( stderr, "task: can't add object %s\n", I.c_str() );
			}
		}
	}

	return mi_ev_ok;
}

int KonsLoader::
S_CmdMake( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie )
{
	KonsLoader* kl = (KonsLoader*)cookie;
	return kl->CmdMake( args );
}


int KonsLoader::
CmdDump( const std::vector<std::string>& args )
{
	if (args.size()>=2) {
		EntityType ent;
		const std::string& str_ent = args[1];
		if (str_ent == "all")  {
			m->Dump( std::cerr, KDIR );
			m->Dump( std::cerr, KENV );
			m->Dump( std::cerr, KRULE );
			m->Dump( std::cerr, KOBJECT );
			m->Dump( std::cerr, KTASK );
			m->Dump( std::cerr, KTASK_OBJ );
			return mi_ev_ok;
		}
		else if (Utils::BeginsWith( str_ent, "rule"))  {
			ent = KRULE;
		}
		else if (Utils::BeginsWith( str_ent, "env"))  {
			ent = KENV;
		}
		else if (Utils::BeginsWith( str_ent, "tasko"))  {
			ent = KTASK_OBJ;
		}
		else if (Utils::BeginsWith( str_ent, "obj"))  {
			ent = KOBJECT;
		}
		else if (Utils::BeginsWith( str_ent, "task"))  {
			ent = KTASK;
		}
		else if (Utils::BeginsWith( str_ent, "dir"))  {
			ent = KDIR;
		}
		else {
			return mi_ev_error;
		}
		m->Dump( std::cerr, ent );
	}
	return mi_ev_ok;
}

int KonsLoader::
S_CmdDump( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie )
{
	KonsLoader* kl = (KonsLoader*)cookie;
	return kl->CmdDump( args );
}

int
KonsLoader::
CmdVar( const std::vector<std::string>& args )
{
	int ret = mi_ev_error;
	if (args.size() >= 2) {
		std::string var_name = args[1];
		std::string var_value;
		bool first = true;
		size_t eqpos = args[1].find('=');

		if (eqpos != std::string::npos) {
			var_name = args[1].substr( 0, eqpos );
			var_value = args[1].substr( eqpos+1 );
			first = false;
		}

		if (args.size() > 2)
		{
			size_t x = 2;
			if (args[2] == "=") {
				x = 3;
			}
			for (size_t i=x; i<args.size(); i++) {
				if (not first) {
					var_value += " ";
				}
				var_value += args[i];
				first = false;
			}
		}

		if (m->envs->AddVar( curr_dir->env_id, var_name, var_value )) {
			ret = mi_ev_ok;
		}
	}
	return ret;
}

int 
KonsLoader::
S_CmdVar( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie )
{
	KonsLoader* kl = (KonsLoader*)cookie;
	return kl->CmdVar( args );
}

int
KonsLoader::
CmdSubst( const std::vector<std::string>& args, std::string *res )
{
	std::string out;
	if (args.size()>=4) {
		if (gmake_subst( args[1], args[2], args[3], &out )) {
			if (res) *res = out;
		}
		else {
			if (res) *res = args[3];
		}
		return mi_ev_ok;
	}
	else {
		return mi_ev_error;
	}
}

int
KonsLoader::
S_CmdSubst( mirtc *, const std::vector<std::string>& args, std::string *res, long cookie )
{
	KonsLoader* kl = (KonsLoader*)cookie;
	return kl->CmdSubst( args, res );
}

#if !defined(NO_FUN)
int main()
{
	KonsLoader* kl = new KonsLoader( "." );
	kl->Load();
	Kons *k = new Kons( kl->m );
	k->AddTarget( "test1" );
	k->Test();
	k->bs->DumpBuildState( std::cerr );
	return 0;
}
#endif
