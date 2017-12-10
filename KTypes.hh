#ifndef KTR_TYPES_HH
#define KTR_TYPES_HH

#include <cstddef>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>

#include "utilk.hh"

namespace Ktr {

/// type of task id 
enum TaskIdType { InvalidTask = 0 };
/// type of rule id 
enum RuleIdType { InvalidRule = 0 };
/// type of object id 
enum ObjIdType { InvalidObject = 0 };
/// type of environment id 
enum EnvIdType { InvalidEnv = 0 };
/// type of directory id 
enum DirIdType { InvalidDir = 0, RootDirId = 1, };
/// type of task-object id 
enum TaskObjIdType { InvalidTaskObj = 0 };

/// id of ktr entity
enum EntityType {
	/// attribute
	KATTR,
	/// directory
	KDIR,
	/// environment
	KENV,
	/// variable
	KVAR,
	/// rule
	KRULE,
	/// object
	KOBJECT,
	/// task
	KTASK,
	/// task-object
	KTASK_OBJ,
};

struct Model;

}

#endif // KTR_TYPES_HH
