#ifndef KTR_GLOBAL_DATA_HH
#define KTR_GLOBAL_DATA_HH

#include "KTypes.hh"

namespace Ktr {
/// @brief the root of the data model, containing the global settings
struct GlobalData {
	/// root directory
	std::string root_dir;
	/// id of the root kdir entity
	DirIdType root_dir_id;

	GlobalData( const std::string& root )
	: root_dir(root)
	, root_dir_id(InvalidDir)
	{
	}
};
}

#endif // KTR_GLOBAL_DATA_HH
