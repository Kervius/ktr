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

	/// map env id to list of vars (as they appear)
	///VarTable* vars;

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

	Model( const std::string& root_dir )
	: globalConf(new GlobalData(root_dir))
	, dirs(new DirTable(this))
	, envs(new EnvTable(this))
	, rules(new RuleTable(this))
	, objects(new ObjectTable(this))
	, tasks(new TaskTable(this))
	, taskObjs(new TaskObjectTable(this))
	, attrs(new AttributeTable)
	{
		// XXX
		dirs->AddDir( std::string() );
		dirs->dirs[ globalConf->root_dir_id ]->dir_name = root_dir;
	}

	~Model()
	{
		delete globalConf;
		delete dirs;
		delete envs;
		delete rules;
		delete objects;
		delete tasks;
		delete taskObjs;
		delete attrs;
	}

#if 0
	/// next id
	EnvIdType
	NextEnvId() const
	{ return EnvIdType( this->envs.empty() ? 1 : this->envs.rbegin()->first + 1 ); }

	/// next id	
	RuleIdType
	NextRuleId() const
	{ return RuleIdType( this->rules.empty() ? 1 : this->rules.rbegin()->first + 1 ); }

	/// next id
	DirIdType
	NextDirId() const
	{ return DirIdType( this->dirs.empty() ? 1 : this->dirs.rbegin()->first + 1 ); }

	/// next id
	ObjIdType
	NextObjId() const
	{ return ObjIdType( this->objects.empty() ? 1 : this->objects.rbegin()->first + 1 ); }

	/// next id
	TaskIdType
	NextTaskId() const
	{ return TaskIdType( this->tasks.empty() ? 1 : this->tasks.rbegin()->first + 1 ); }

	/// next id
	TaskObjIdType
	NextTaskObjId() const
	{ return TaskObjIdType( this->objTaskRel.empty() ? 1 : this->objTaskRel.rbegin()->first + 1 ); }

	/// add or find kdir
	Dir*
	AddDir( const std::string& dir );

	/// find dir
	Dir*
	FindDir( const std::string& dir );

	/// find dir
	Dir*
	FindDir( DirIdType dir_id );

	/// create new environment
	Env*
	AddEnv( EnvIdType parent_env_id );

	/// add or find rule
	Rule*
	AddRule( Dir* dir, const std::string& rule_name );

	/// find rule
	Rule*
	FindRule( Dir* dir, const std::string& rule_name, bool recurse = true );

	/// add or find var
	Var*
	AddVar( EnvIdType env_id, const std::string& var_name );

	/// add or find var, set value
	Var*
	AddVar( EnvIdType env_id, const std::string& var_name, 
		const std::string& value );

	/// find var
	Var*
	FindVar( EnvIdType env_id, const std::string& var_name, bool recurse = true );

	/// get expanded variable value
	std::string
	ExpandVarString( EnvIdType env_id, const std::string &str );

	/// add task, with rule by name
	Task*
	AddTask( Dir* dir, const std::string& rule_name );

	/// add task, with given rule
	Task*
	AddTask( Dir* dir, Rule* rule );

	/// add object
	Object*
	AddObject( Dir* dir, const std::string& name );

	/// find object
	Object*
	FindObject( ObjIdType obj_id );

	/// find object
	Object*
	FindObject( const std::string &object_name );

	/// get file name
	std::string
	GetObjectName( ObjIdType obj_id );

	/// get file name
	std::string
	GetObjectName( Object *obj );

	/// add task-object, using object
	TaskObject*
	AddTaskObject( Task* task, Object* obj, TaskObject::Role role,
		const std::string& obj_orig_name = std::string() );

	/// add task-object, using another task-object
	TaskObject*
	AddTaskObject( Task* task, TaskObject* ot, TaskObject::Role role );

	/// find task-object
	TaskObject*
	FindTaskObj( Task* t, Object* o );

	/// find task-object
	TaskObject*
	FindTaskObj( TaskIdType task_id, ObjIdType obj_id );

	/// dump given entity in textual form
	void Dump( std::ostream& o, EntityType );
#endif
};

}

#endif // KTR_DATA_MODEL_HH
