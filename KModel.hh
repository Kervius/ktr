#ifndef KTR_DATA_MODEL_HH
#define KTR_DATA_MODEL_HH

#include "KTypes.hh"

#include "KGlobalData.hh"
#include "KDir.hh"
#include "KVar.hh"
#include "KEnv.hh"
#include "KRule.hh"
#include "KObject.hh"
#include "KTask.hh"
#include "KTaskObject.hh"
#include "KAttribute.hh"

namespace Ktr {

/// @brief Container for the model of the build system
struct Model {
	/// global data of the ktr model
	GlobalData* globalConf;

	/// map of dir id to dir object
	DirTable* dirs;

	/// map env id to env object
	EnvTable* envs;

	/// map rule id to rule object
	RuleTable* rules;

	/// map obj id to object
	ObjectTable* objects;

	/// map task id to task object
	TaskTable* tasks;

	/// map task-obj id to task-obj relation object
	//ObjectTaskRel* objTaskRel;

	/// map task id to list of task-obj objects
	TaskObjectTable* taskObjs;

	/// attr_id -> attribute object
	AttributeTable* attrs;
public:
	Model( const std::string& root_dir );

	~Model();

	const std::string& GetRootDir();

	Dir* GetRootDirObj();

	void Dump( std::ostream& o, EntityType );
};

}

#endif // KTR_DATA_MODEL_HH
