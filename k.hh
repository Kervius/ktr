#ifndef K__________
#define K__________

#include <string>
#include <vector>

namespace K {
	struct KOpt {
		enum Cmd {
			CMD_NONE,
			CMD_BUILD,
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


#endif /* K__________ */
