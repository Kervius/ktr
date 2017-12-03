#ifndef KTR_DIR_HH
#define KTR_DIR_HH

#include "KTypes.hh"

namespace Ktr {
/// @brief directory entity
struct Dir {
	/// unique id of the entity, the primary key
	DirIdType dir_id = InvalidDir;
	/// id of the parent dir, or 0
	DirIdType parent_dir_id;
	/// id of the environment of the dir
	EnvIdType env_id;	
	/// directory name: relative to root, or absolute if outside the root
	std::string dir_name;
	/// default object for the dir
	std::vector<ObjIdType> dir_defaults;

	Dir()
	: dir_id( InvalidDir )
	, parent_dir_id( InvalidDir )
	, env_id( InvalidEnv )
	{
	}
};


struct DirTable {
	/// parent model
	Model *model;

	/// map of dir id to dir object
	std::map<DirIdType, Dir*> dirs;

	DirTable( Model* m_ );

	/// next id
	DirIdType
	NextDirId() const;

	/// create a new instance of a dir
	Dir*
	NewDir( DirIdType dir_id, DirIdType parent_dir_id, const std::string &dir_name,
			EnvIdType env_id = InvalidEnv );

	/// find or add dir
	Dir*
	AddDir( const std::string& dir );

	/// find dir
	Dir*
	FindDir( const std::string& dir );

	/// find relative dir
	Dir*
	FindRelDir( Dir* dir, const std::string& odir );

	/// look-up dir id
	Dir*
	LookUpDir( DirIdType dir_id );

	/// get parent dir
	DirIdType
	GetParentDir( DirIdType dir_id );

	/// dump the dirs to the stream
	void
	Dump( std::ostream& o );
};

}

#endif // KTR_DIR_HH
