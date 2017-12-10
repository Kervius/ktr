
#include <stdio.h>

#include "KModel.hh"

#include "Minsk/min.hh"

namespace Ktr {

struct KonsLoader {

	std::string root_dir;
	Model* m;
	mirtc *rtc;
	Dir *curr_dir;

	struct DirDescentEntry {
		Dir* parent_dir;
		std::string sub_dir;
		DirDescentEntry( Dir* par, const std::string& dn )
		: parent_dir(par)
		, sub_dir(dn)
		{}
	};
	std::list< DirDescentEntry > walk_list;

	KonsLoader( const std::string& root_dir_ );
	~KonsLoader();

	bool ReadFile( const std::string& file_name, std::string *content );

	bool Load();

	bool Eval( const std::string& expr );

	int CmdSubdir( const std::vector<std::string>& args );
	static int S_CmdSubdir( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie );

	int CmdRule( const std::vector<std::string>& args );
	static int S_CmdRule( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie );

	int CmdMake( const std::vector<std::string>& args );
	static int S_CmdMake( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie );

	int CmdDump( const std::vector<std::string>& args );
	static int S_CmdDump( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie );

	int CmdSubst( const std::vector<std::string>& args, std::string *res );
	static int S_CmdSubst( mirtc *rtc, const std::vector<std::string>& args, std::string *res, long cookie );
};

}

