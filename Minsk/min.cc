
#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <fstream>
#include <streambuf>

#include "min.hh"

#if 0
typedef std::vector<std::string> str_list_t;

typedef std::list< str_list_t > str_list_stack_t;

enum {
	mis_normal = 0,
	mis_escape = 1,
	mis_string = 2,
	mis_string_esc = 3,
	mis_vstring    = 4,
	mis_variable   = 5,
};

enum {
	mi_ev_ok = 0,
	mi_ev_error,
	mi_ev_no_proc,
	mi_ev_return,	// special case: return from function
	mi_ev_do,	// special case: eval one of the args
};

enum {
	mi_rtc_call,	// rtc of a normal function call 
	mi_rtc_lex,	// rtc of a lexical (syntax) context
	mi_rtc_icall,	// rtc of an inline function call
};

struct mi_uproc {
	std::string name;
	std::string body;
	std::list< std::pair<std::string, std::string> > args;
};

struct mirtc {
	int level;
	mirtc *parent_rtc;
	int rtc_type;

	typedef std::map< std::string, mi_uproc > proc_table_t;
	proc_table_t proc_table;

	typedef std::map< std::string, std::string > var_table_t;
	var_table_t var_table;

	std::string side_effect;

	mirtc()
	: level(0)
	, parent_rtc(NULL)
	, rtc_type(mi_rtc_call)
	{};
};
#endif

static char decode_esc_char( char ch );

//int mi_eval( mirtc *rtc, const std::string& e );


//typedef int (*mi_command_t)( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res );


#if 1
#define PPR( rtc, fmt, ... )	\
	do { printf( "(rtc %d) " fmt, (rtc)->level , ##__VA_ARGS__  ); } while(0)
#else
#define PPR( ... )	do { } while(0)
#endif

#define PP( ... )	PPR( rtc, __VA_ARGS__ )

bool mi_var_get( mirtc *rtc, const std::string& var_name, std::string *pstr )
{
	while (rtc) {
		auto x = rtc->var_table.find( var_name );
		if (x != rtc->var_table.end()) {
			if (pstr) *pstr = x->second;
			return true;
		}
		rtc = rtc->parent_rtc;
	}
	if (pstr) *pstr = std::string();
	return false;
}

static std::string mi_val_join( const std::vector<std::string>& list, unsigned begin, unsigned end = -1u )
{
	if (end == -1u)
		end = list.size();

	std::string ret;
	bool first = true;

	for (unsigned i = begin; i<end; i++)
	{
		if (!first)
		{
			ret += " ";
			first = false;
		}
		ret += list[i];
	}

	return ret;
}


// assign (create if not available) variable: let varname value 
//   if variable doesn't exist, create it in the function call context
int micm_let( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0 )
{
	if (cur_cmd.size() >= 3)
	{
		//mirtc* srtc = rtc;
		const std::string& var_name = cur_cmd[1];

		bool sugar = (cur_cmd[2].compare("=") == 0);
		std::string val = mi_val_join( cur_cmd, sugar ? 3 : 2 );

		// try to update the existing variable
		for (mirtc *c = rtc; c != NULL; c = c->parent_rtc)
		{
			mirtc::var_table_t::iterator x = c->var_table.find(var_name);
			if (x != c->var_table.end())
			{
				x->second = val;
				rtc->side_effect = val;
				if (res) *res = val;
				return mi_ev_ok;
			}
		}

		// create new variable in function context
		for (mirtc *c = rtc; c != NULL; c = c->parent_rtc)
		{
			if (c->rtc_type == mi_rtc_call)
			{
				c->var_table[ var_name ] = val;
				rtc->side_effect = val;
				if (res) *res = val;
				return mi_ev_ok;
			}
		}
		assert( false );
		return mi_ev_error;	// no context?
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

// create variable: var varname value
//   variable is created in the current lex (that included icall) context
int micm_var( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0 )
{
	if (cur_cmd.size() >= 2)
	{
		std::string v;
		v = mi_val_join( cur_cmd, 2 );
		rtc->var_table[ cur_cmd[1] ] = v;
		rtc->side_effect = v;
		if (res) *res = v;
		PP( "DBG: create var: [%s], value [%s]\n", cur_cmd[1].c_str(), v.c_str() );
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

// proc hello name {print "hello, " $name "\n"}
//  function is created in the function call context (not lex/icall)
int micm_proc( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0)
{
	mi_uproc proc;
	if (cur_cmd.size() >= 3)
	{
		proc.name = cur_cmd[1];
		proc.body = cur_cmd.back();
		for (unsigned i = 2; i<cur_cmd.size()-1; i++)
		{
			proc.args.push_back( std::make_pair(cur_cmd[i], std::string()) );
		}

		for (mirtc *c = rtc; c != NULL; c = c->parent_rtc)
		{
			if (c->rtc_type == mi_rtc_call)
			{
				c->proc_table[ proc.name ] = proc;
				if (res) *res = cur_cmd[1];
				return mi_ev_ok;
			}
		}

	}
	return mi_ev_error;
}

int micm_if( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0 )
{
	bool yes = false;
	bool OK = false;
	int ret = mi_ev_ok;
	if (cur_cmd.size() >= 2)
	{
		if (cur_cmd[1].size()>0 && strtol( cur_cmd[1].c_str(), NULL, 0 ) != 0)
		{
			yes = true;
		}
	}

	if (cur_cmd.size() == 3)
	{
		// if bool expr-true
		if (yes)
		{
			ret = mi_eval( rtc, cur_cmd[2] );
			OK = true;
		}
	}
	else if (cur_cmd.size() == 4)
	{
		// if bool expr-true expr-false
		ret = mi_eval( rtc, yes ? cur_cmd[2] : cur_cmd[3] );
		OK = true;
	}
	else if (cur_cmd.size() == 5 && cur_cmd[3].compare("else") == 0)
	{
		// if bool expr-true else expr-false
		ret = mi_eval( rtc, yes ? cur_cmd[2] : cur_cmd[4] );
		OK = true;
	}
	else
	{
		return mi_ev_error;
	}

	if (OK)
	{
		if (res) *res = rtc->side_effect;
	}

	return ret;
}

//
// function library
//

int micm_add( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0 )
{
	if (cur_cmd.size() >= 2)
	{
		long ret = 0;
		char buf1[64];

		for (unsigned i = 1; i<cur_cmd.size(); i++)
		{
			ret += strtol( cur_cmd[i].c_str(), NULL, 0 );
		}

		snprintf( buf1, sizeof(buf1), "%ld", ret );

		//PP( "DBG: add ==> %s\n", buf1 );

		if (res) *res = std::string( buf1 );
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

int micm_mul( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0 )
{
	if (cur_cmd.size() >= 2)
	{
		long ret = 1;
		char buf1[64];

		for (unsigned i = 1; i<cur_cmd.size(); i++)
		{
			ret *= strtol( cur_cmd[i].c_str(), NULL, 0 );
		}

		snprintf( buf1, sizeof(buf1), "%ld", ret );

		//PP( "DBG: mul ==> %s\n", buf1 );

		if (res) *res = std::string( buf1 );
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

int micm_eq( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0 )
{
	if (cur_cmd.size() >= 3)
	{
		bool OK = true;

		for (unsigned i = 2; i<cur_cmd.size(); i++)
		{
			if (cur_cmd[1] != cur_cmd[i])
			{
				OK = false;
			}
		}

		if (res) *res = OK ? std::string( "1" ) : std::string();
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

int micm_return( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0 )
{
	if (res)
	{
		*res = mi_val_join( cur_cmd, 1 );
	}
	return mi_ev_return;
}

int micm_print( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0 )
{
	bool first = true;
	for (unsigned i = 1; i<cur_cmd.size(); i++)
	{
		if (!first)
		{
			printf( " " );
		}
		printf( "%s", cur_cmd[i].c_str() );
		first = false;
	}
	if (res) res->clear();
	return mi_ev_ok;
}

int micm_println( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long = 0 )
{
	int ret;

	ret = micm_print( rtc, cur_cmd, res );

	if (ret == mi_ev_ok)
	{
		printf( "\n" );
	}

	return mi_ev_ok;
}


struct micm_builtin_table_row {
	const char *name;
	mi_command_t entry;
};
static micm_builtin_table_row micm_builtin_table[] = {
	{ "proc", micm_proc },
	{ "var", micm_var },
	{ "let", micm_let },
	{ "add", micm_add },
	{ "mul", micm_mul },
	{ "eq", micm_eq },
	{ "if", micm_if },
	{ "print", micm_print },
	{ "println", micm_println },
	{ "return", micm_return },
};
struct micm_table_row {
	mi_command_t entry;
	long cookie;
	micm_table_row( mi_command_t cm = nullptr, long cookie_ = 0 )
	: entry(cm)
	, cookie(cookie_)
	{}
};
typedef std::map<std::string, micm_table_row> micm_table_t;
static micm_table_t *micm_builtin_table_ptr = nullptr;
static micm_table_t *micm_user_table_ptr = nullptr;

void mi_add_user_command( const char *name, mi_command_t fun_ptr, long cookie)
{
	if (!micm_user_table_ptr) {
		micm_user_table_ptr = new micm_table_t;
	}
	(*micm_user_table_ptr)[ std::string(name) ] = micm_table_row(fun_ptr, cookie);
}

int mi_call_uproc( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	int ret = mi_ev_ok;
	mirtc new_rtc;

	assert( rtc != NULL );

	new_rtc.parent_rtc = rtc;
	new_rtc.level = rtc ? rtc->level+1 : 1000;

	mi_uproc *puproc = NULL;
	for (mirtc *c = rtc; c != NULL; c = c->parent_rtc)
	{
		mirtc::proc_table_t::iterator x = c->proc_table.find( cur_cmd[0] );
		if (x != c->proc_table.end())
		{
			puproc = &(x->second);
			break;
		}
	}

	if (puproc)
	{
		PPR( &new_rtc, "XXX: puSh rtc %p <- %p\n", &new_rtc, rtc );

		unsigned i = 1;
		for (auto I : puproc->args )
		{
			if (i >= cur_cmd.size())
				break;

			PPR( &new_rtc, "call_proc [%s]: local var: [%s] = [%s]\n",
					cur_cmd[0].c_str(), I.first.c_str(), cur_cmd[i].c_str() );

			std::vector<std::string> var_cmd;
			var_cmd.push_back( std::string() );
			var_cmd.push_back( I.first );
			var_cmd.push_back( cur_cmd[i] );
			micm_var( &new_rtc, var_cmd, NULL );
			i++;
		}

		ret = mi_eval( &new_rtc, puproc->body );

		rtc->side_effect = new_rtc.side_effect;
		if (res) *res = new_rtc.side_effect;

		PPR( &new_rtc, "XXX: pOp  rtc %p -> %p\n", &new_rtc, rtc );
	}
	else
	{
		return mi_ev_no_proc;
	}

	return ret;
}

int mi_call( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
#if 1
	int i = 0;
	for (auto x : cur_cmd)
	{
		PP( "%s %d: \"%s\"\n", i == 0 ? "eval: " : "      " , i, x.c_str() );
		i++;
	}
#endif

	int ret = mi_call_uproc( rtc, cur_cmd, res );

	if (ret == mi_ev_no_proc)
	{
		if (!micm_builtin_table_ptr) {
			micm_builtin_table_ptr = new micm_table_t;
			for (auto x : micm_builtin_table) {
				(*micm_builtin_table_ptr)[ x.name ] = micm_table_row(x.entry, 0);
			}
		}
		micm_table_row *command_ptr = nullptr;
		auto P1 = micm_builtin_table_ptr->find( cur_cmd[0] );
		if (P1 != micm_builtin_table_ptr->end()) {
			command_ptr = &P1->second;
		}
		if (!command_ptr) {
			if (micm_user_table_ptr) {
				auto P2 = micm_user_table_ptr->find( cur_cmd[0] );
				if (P2 != micm_user_table_ptr->end())
					command_ptr = &P2->second;
			}
		}
		if ( command_ptr )
		{
			return command_ptr->entry( rtc, cur_cmd, res, command_ptr->cookie );
		}
		/*for (auto x : micm_builtin_table)
		{
			if ( cur_cmd[0].compare(x.name) == 0 )
			{
				return x.entry( rtc, cur_cmd, res );
			}
		}*/
	}
	else
	{
		return ret;
	}

	PPR( rtc, "ERR: unk proc: [%s] (rtc == %p)\n", cur_cmd[0].c_str(), rtc );

	//assert( false );

	return mi_ev_no_proc;
}


static char decode_esc_char( char ch )
{
	switch (ch)
	{
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'e': return 27;
	default: return ch;
	}
}

static std::string join_char_list( std::list<char>& chl )
{
	std::string ret;
	for (auto ch : chl)
		ret += ch;
	return ret;
}

int mi_eval( mirtc *parent_rtc, const std::string& e )
{
	size_t ii = 0;
	int ret = mi_ev_ok;

	assert( parent_rtc != NULL );

	mirtc &rtc = *parent_rtc;

	int state = mis_normal;
	std::list<int> state_stack;

	std::string cur_str;
	str_list_t cur_cmd;
	str_list_stack_t cmd_stack;

	std::list<char> brace_stack;
	std::list<char> sbrace_stack;

	while (ii <= e.size())
	{
		int ch;
		do {
			ch = ii < e.size() ? e[ii] : -1;
			ii++;
		} while (ch == '\r');

		//printf( "state: %d, char %c (%d)\n", state, isprint(ch) ? ch : '.', ch );

		if (state == mis_normal)
		{
			if (ch == -1 || ch == ';' || ch == '\n' || (not(brace_stack.empty()) and brace_stack.back() == ch))
			{
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				if (not cur_cmd.empty())
				{
					std::string res;
					int lret;

					lret = mi_call( &rtc, cur_cmd, &res );

					if (lret == mi_ev_return)
					{
						if (rtc.rtc_type == mi_rtc_lex)
						{
							ret = lret;
						}
						rtc.side_effect = res;
						PPR( &rtc, "DBG: return %s\n", rtc.side_effect.c_str() );
						break;
					}
					if (lret != mi_ev_ok)
					{
						ret = lret;
						break;
					}

					cur_cmd.clear();

					std::string brs = join_char_list( brace_stack );
					std::string sbrs = join_char_list( sbrace_stack );
					PPR( &rtc, "YYY: res [%s] (%lu) [%s] [%s]\n", res.c_str(),
						(long unsigned)cmd_stack.size(), brs.c_str(), sbrs.c_str());

					if (not(cmd_stack.empty()) and not(brace_stack.empty()) and brace_stack.back() == ch )
					{
						cur_cmd = cmd_stack.back();
						cmd_stack.pop_back();

						cur_cmd.push_back( res );

						brace_stack.pop_back();
					}
					else
					{
						//PPR( &rtc, "ZZZ: losing to side_effect [%s]\n", res.c_str() );
						// in case of call like `mi_eval("add 1 2")`, the side-effect is the result
						rtc.side_effect = res;
					}
				}
			}
			else if (isblank(ch))
			{
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
			}
			else if (ch == '(')
			{
				brace_stack.push_back( ')' );

				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}

				cmd_stack.push_back( cur_cmd );
				cur_cmd.clear();
			}
			else if (ch == '"')
			{
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				//sbrace_stack.push_back( '"' );
				state_stack.push_back( state );
				state = mis_string;
			}
			else if (ch == '{')
			{
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				sbrace_stack.push_back( '}' );
				state_stack.push_back( state );
				state = mis_vstring;
			}
			else if (ch == '$')
			{
				// variable subsitution
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				state_stack.push_back( state );
				state = mis_variable;
			}
			else if (ch == '\\')
			{
				state_stack.push_back( state );
				state = mis_escape;
			}
			else
			{
				cur_str.push_back( ch );
			}
		}
		else if (state == mis_escape)
		{
			//assert( false && "support for '\\' missing" );
			if (ch == '\n')
			{
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
			}
			else
			{
				cur_str.push_back( ch );
			}
			state = state_stack.back();
			state_stack.pop_back();
		}
		else if (state == mis_string)
		{
			if (ch == '"')
			{
				if (sbrace_stack.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				else
				{
					cur_str.push_back( ch );
				}

				state = state_stack.back();
				state_stack.pop_back();
			}
			else if (ch == '\\')
			{
				state = mis_string_esc;
			}
			else
			{
				cur_str.push_back( ch );
			}
		}
		else if (state == mis_string_esc)
		{
			cur_str.push_back( decode_esc_char(ch) );
			state = mis_string;
		}
		else if (state == mis_vstring)
		{
			if (ch == sbrace_stack.back())
			{
				state = state_stack.back();
				state_stack.pop_back();

				sbrace_stack.pop_back();

				if (sbrace_stack.empty())
				{
					assert( state != mis_vstring );
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				else
				{
					cur_str.push_back( ch );
				}
			}
			else if (ch == '"')
			{
				cur_str.push_back( ch );

				state_stack.push_back( state );
				state = mis_string;
			}
			else if (ch == '{')
			{
				cur_str.push_back( ch );

				sbrace_stack.push_back( '}' );
				state_stack.push_back( state );
			}
			else
			{
				cur_str.push_back( ch );
			}
		}
		else if (state == mis_variable)
		{
			if (isalnum(ch) || ch == '_')
			{
				cur_str.push_back(ch);
			}
			else
			{
				std::string val;

				(void)mi_var_get( &rtc, cur_str, &val);

				PPR( &rtc, "DBG: $ var name: [%s] == [%s]\n", cur_str.c_str(), val.c_str() );

				cur_cmd.push_back( val );
				cur_str.clear();
				
				state = state_stack.back();
				state_stack.pop_back();

				ii--;
			}
		}
		else
		{
			printf( "ERR: bad state == %d\n", state );
			assert( false && "bad state" );
		}
	}

	if (rtc.rtc_type == mi_rtc_call && ret == mi_ev_return)
	{
		ret = mi_ev_ok;
	}

	return ret;
}

void test1()
{
#if 0
	if (1)
	{
		const char *x1 = 
			"switch (a 1 2) {\n"
			"case \"a\" { print $a }\n"
			"case \"b\" { print $b }\n"
			"case \"c\" { print $c }\n"
			"}\n"
			;
		printf( "-------------------------------\n" );
		printf( "raw: [[[\n%s]]]\n", x1 );
		mirtc rtc;
		mi_eval( &rtc, std::string( x1 ) );
	}
	if (1)
	{
		printf( "-------------------------------\n" );
		mirtc rtc;
		mi_eval( &rtc, std::string("let a \"string test\"") );
	}
	if (1)
	{
		printf( "-------------------------------\n" );
		mirtc rtc;
		mi_eval( &rtc, std::string("let a (add (mul 2 2) (mul 3 (add 1 1)))") );
	}
	if (1)
	{
		printf( "-------------------------------\n" );
		mirtc rtc;
		mi_eval( &rtc, std::string("if $a {curly test {test} test} else { not wrong too }") );
	}
	if (1)
	{
		printf( "-------------------------------\n" );
		mirtc rtc;
		mi_eval( &rtc, std::string("proc fact n { let a (fact (add n )) }") );
	}
#endif

#if 0
	if (1)
	{
		const char *x11 = "proc a { return (b) }; proc b { return (add 1 1) }; println (a);";
		mirtc rtc;
		mi_eval( &rtc, std::string( x11 ) );
	}
#endif

#if 1
	if (1)
	{
		const char *x2 =
			"proc fact n {"					"\n"
			"	if (eq $n 0) {"				"\n"
			"		return 1"			"\n"
			"	} else {"				"\n"
			"		var m (add $n -1)"		"\n"
			"		return (mul $n (fact $m))"	"\n"
			"	}"					"\n"
			"}"						"\n"
			"println fact 6 = (fact 6)"			"\n"
			"println fact 6 = (mul 1 2 3 4 5 6)"		"\n"
			;
		printf( "raw: [[[\n%s]]]\n", x2 );
		mirtc rtc;
		mi_eval( &rtc, std::string( x2 ) );
	}
#endif
}

#if defined(SELF_TEST)
int main()
{
	test1();
	return 0;
}
#elif defined(MISH)
int main(int argc, char **argv)
{
	if (argc >= 3 && strcmp(argv[1], "-c") == 0)
	{
		mirtc rtc;
		rtc.level = 1;
		rtc.parent_rtc = NULL;
		for (int i = 2; i<argc; i++)
		{
			mi_eval( &rtc, std::string(argv[i]) );
		}
		return 0;
	}
	else if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
	{
		printf( "mish -c" );
		return 0;
	}
	else if (argc >= 2)
	{
		mirtc rtc;
		rtc.level = 1;
		rtc.parent_rtc = NULL;

		int ret = 0;

		for (int i = 1; i<argc; i++)
		{

			std::ifstream t;
			t.open(argv[i], std::ios::in);
			if (t.is_open())
			{
				std::string str
					(
						(std::istreambuf_iterator<char>(t)),
						std::istreambuf_iterator<char>()
					);

				int lret = mi_eval( &rtc, str );
				ret = (lret == mi_ev_ok) ? 0 : 1 ;
			}
			else
			{
				printf( "ERR: can't open file: %s\n", argv[i] );
				ret = 1;
			}
		}
		return ret;
	}
	return 1;
}
#endif

// vim:cindent:
