
#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <fstream>
#include <streambuf>


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

int mi_eval( mirtc *rtc, const std::string& e );

typedef int (*mi_command_t)( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res, long cookie );
void mi_add_user_command( const char *name, mi_command_t fun_ptr, long cookie);


