#ifndef KTR_OBJECT_HH
#define KTR_OBJECT_HH

#include "KTypes.hh"

namespace Ktr {

/// @brief object, a file/etc inside the system
struct Object {
	/// unique id of the object, the primary key
	ObjIdType obj_id = InvalidObject;	
	/// id of the directory where the object lies
	DirIdType dir_id;	
	/// name of the object, without directory components (see also KTaskObject::obj_orig_name)
	std::string obj_name;

	Object()
	: obj_id(InvalidObject)
	, dir_id(InvalidDir)
	{
	}
};

struct ObjectTable {
	/// parent model
	Model *model;
	/// map obj id to object
	std::map<ObjIdType, Object*>        objects;

	/// c'tor
	ObjectTable( Model *m );

	/// generate unique id
	ObjIdType
	NextObjId() const;

	/// add object
	Object*
	AddObject( Dir* dir, const std::string& name );

	/// find object
	Object*
	LookUpObject( ObjIdType obj_id );

	/// find object
	Object*
	FindObject( const std::string &object_name );

	/// get file name
	std::string
	GetObjectName( ObjIdType obj_id );

	/// get file name
	std::string
	GetObjectName( Object *obj );
};

}

#endif // KTR_OBJECT_HH
