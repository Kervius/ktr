#ifndef DATAK_HH_
#define DATAK_HH_

#include <string>
#include <map>
#include <set>
#include <vector>

#include <ostream>
#include <iostream>

/// @file datak.hh
/// ktr classes definitions

/// Ktr namespace
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

/// @brief the root of the data model, containing the global settings
struct KGlobalData {
	/// root directory
	std::string root_dir;
	/// id of the root kdir entity
	DirIdType root_kdir_id = InvalidDir;
};

/// @brief directory entity
struct KDir {
	/// unique id of the entity, the primary key
	DirIdType kdir_id = InvalidDir;
	/// id of the parent dir, or 0
	DirIdType parent_dir_id;
	/// id of the environment of the dir
	EnvIdType kenv_id;	
	/// directory name: relative to root, or absolute if outside the root
	std::string dir_name;
};

/// @brief variable entity
struct KVar {
	/// id of the environment the variable belongs to
	EnvIdType kenv_id;	
	/// variable name
	std::string var_name;
	/// variable value
	std::string var_value;
};

/// @brief environment entity, a set of variables
struct KEnv {
	/// id of the environment, the primary key
	EnvIdType kenv_id;	
	/// id of the parent environment
	EnvIdType parent_env_id;
};

/// @brief rule entity, a template of the command for the task entities
struct KRule {
	/// unique id of the rule, the primary key
	RuleIdType krule_id = InvalidRule;	
	/// id of the directory where the rule was defined
	DirIdType kdir_id;	
	/// number of inputs the rule has
	int num_inp;	
	/// number of outputs the rule has
	int num_outp;	
	/// the template string of the comment to execute
	std::string command;
	/// the public name of the rule, used for late binding with the tasks
	std::string rule_name;
};

/// @brief object, a file/etc inside the system
struct KObject {
	/// unique id of the object, the primary key
	ObjIdType kobj_id = InvalidObject;	
	/// id of the directory where the object lies
	DirIdType kdir_id;	
	/// name of the object, without directory components (see also KTaskObject::obj_orig_name)
	std::string obj_name;
};

/// @brief task, the arguments for the rule, combined together giving the environment, the directory and the command to execute
struct KTask {
	/// unique id of the object, the primary key
	TaskIdType ktask_id = InvalidTask;
	/// directory where the rule is defined and command should be executed in
	DirIdType kdir_id;
	/// environment to use
	EnvIdType kenv_id;	
	/// resolved id of the rule, or 0
	RuleIdType krule_id;
	/// name of the rule
	std::string rule_name;
};

/// @brief relation entity, linking a task with input/output/dependent objects
struct KTaskObject {
	/// defines roles of the objects
	enum Role {		
		/// input object (aka input file(s))
		INPUT,
		/// output object (aka output file(s))
		OUTPUT,
		/// dependent object (extra dependencies)
		DEPNCY,
	};
	/// unique id of the entity
	TaskObjIdType ktask_obj_id = InvalidTaskObj;
	/// task
	TaskIdType ktask_id;			
	/// object id
	ObjIdType kobj_id;			
	/// type of the object
	Role role;				
	/// original name of the object. the KObject would contain
	/// the normalized file name, which not necessarily matches
	/// the name as specified by the user when defining the task.
	/// the model in memory requires normalized names, while for command
	/// execution, the original name is required
	std::string obj_orig_name;
};

/// id of ktr entity
enum KEntityType {
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

/// @brief attribute entity
/// kentity_id + type point to the entity the attribute is attached to
struct KAttribute {
	/// unique id of the attribute
	int kattr_id = 0;	
	/// id of the entity
	int kentity_id;	
	/// type of the entity
	KEntityType type;
	/// name of the attribute
	std::string name;
	/// optional value of the attribute
	std::string value;
};

/// @brief Container for the model of the build system
struct KModel {
	/// global data of the ktr model
	KGlobalData* km;

	/// map of dir id to dir object
	std::map<DirIdType, KDir*>          dirs;

	/// map of dir id to list of default object ids
	std::map<DirIdType, std::vector<ObjIdType>>   dirDefaults;

	/// map env id to list of vars (as they appear)
	std::map<EnvIdType, std::vector<KVar*>> vars;

	/// map env id to env object
	std::map<EnvIdType, KEnv*>          envs;

	/// map rule id to rule object
	std::map<RuleIdType, KRule*>         rules;

	/// map obj id to object
	std::map<ObjIdType, KObject*>        objects;

	/// map task id to task object
	std::map<TaskIdType, KTask*>         tasks;

	/// map task-obj id to task-obj relation object
	std::map<TaskObjIdType, KTaskObject*> objTaskRel;

	/// map task id to list of task-obj objects
	std::map<TaskIdType, std::vector<KTaskObject*>> taskObjs;

	/// attr_id -> attribute object
	std::map<int, KAttribute*>         attrs;

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
	{ return DirIdType( this->rules.empty() ? 1 : this->dirs.rbegin()->first + 1 ); }

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
	KDir*
	AddDir( const std::string& dir );

	/// find dir
	KDir*
	FindDir( const std::string& dir );

	/// find dir
	KDir*
	FindDir( DirIdType kdir_id );

	/// create new environment
	KEnv*
	AddEnv( EnvIdType parent_env_id );

	/// add or find rule
	KRule*
	AddRule( KDir* dir, const std::string& rule_name );

	/// find rule
	KRule*
	FindRule( KDir* dir, const std::string& rule_name, bool recurse = true );

	/// add or find var
	KVar*
	AddVar( EnvIdType env_id, const std::string& var_name );

	/// add or find var, set value
	KVar*
	AddVar( EnvIdType env_id, const std::string& var_name, 
		const std::string& value );

	/// find var
	KVar*
	FindVar( EnvIdType env_id, const std::string& var_name, bool recurse = true );

	/// get expanded variable value
	std::string
	ExpandVarString( EnvIdType env_id, const std::string &str );

	/// add task, with rule by name
	KTask*
	AddTask( KDir* dir, const std::string& rule_name );

	/// add task, with given rule
	KTask*
	AddTask( KDir* dir, KRule* rule );

	/// add object
	KObject*
	AddObject( KDir* dir, const std::string& name );

	/// find object
	KObject*
	FindObject( ObjIdType kobj_id );

	/// get file name
	std::string
	GetObjectName( ObjIdType kobj_id );

	/// get file name
	std::string
	GetObjectName( KObject *obj );

	/// add task-object, using object
	KTaskObject*
	AddTaskObject( KTask* task, KObject* obj, KTaskObject::Role role,
		const std::string& obj_orig_name = std::string() );

	/// add task-object, using another task-object
	KTaskObject*
	AddTaskObject( KTask* task, KTaskObject* ot, KTaskObject::Role role );

	/// find task-object
	KTaskObject*
	FindTaskObj( KTask* t, KObject* o );

	/// find task-object
	KTaskObject*
	FindTaskObj( TaskIdType task_id, ObjIdType obj_id );

	/// dump given entity in textual form
	void Dump( std::ostream& o, KEntityType );
};

/// create instance of a ktr model
KModel* KModelCreate( const std::string& root_dir );

} // Ktr

// --------------------------------------------------

namespace Ktr {

/// @brief dependency graph for the model
struct DGraph {
	/// model the dep graph is attached to
	KModel* km;

	/// obj a is produced by task b
	std::map< ObjIdType, TaskIdType > objectProducer;

	/// task a depends on { b }
	std::map< TaskIdType, std::set<TaskIdType> > taskPrereqs;

	/// task a contributes to { b }
	std::map< TaskIdType, std::set<TaskIdType> > taskContribTo;

	/// @brief initialize the dep graph
	/// call the fill methods to populate the dep graph from the KModel
	void InitDepGraph();

	/// fill object producers map
	void FillObjectProducer();

	/// fill task prerequisites map
	void FillPrereq();

	/// fill task contributions map
	void FillContrib();

	/// dump textual representation of the dep graph
	void DumpDepGraph( std::ostream& o );

	/// initialize the instance of the dep graph object
	DGraph( KModel* kmm );
};

/// build state for the model and the dep graph
enum BTaskStateType {
	/// initial task state
	TASK_PENDING,
	/// finished
	TASK_FINISHED,
	/// running
	TASK_RUNNING,
};

/// @brief build state of the task
struct BTaskState {
	/// task id
	TaskIdType ktask_id;
	/// task state
	BTaskStateType state;
	/// number of prerequisites yet un-finished
	int num_prereq;
};

/// @brief build state
struct BState
{
	/// model
	KModel* km;
	/// dep graph
	DGraph* dg;

	/// @brief task_id to task state map
	std::map< TaskIdType, BTaskState > taskStates;

	/// @brief fill taskStates for the given task
	/// recursively, using the prerequisites map from the dep graph, 
	/// fill taskStates with initial information
	void FillStatesForTask( TaskIdType task_id );

	/// @brief fill taskStates for the given object
	void FillStatesForObj( ObjIdType obj_id );

	/// count to set of tasks map
	std::map< int, std::set<TaskIdType> > buildQueue;

	/// @brief using the taskStates, fill build queue
	void FillBuildQueue();

	/// @brief update state of the given task
	void UpdateBuildState( TaskIdType task_id, BTaskStateType new_state );

	/// @brief decrement the number of un-finished prerequisites of the given task
	/// when the counter reaches zero, that means all the prerequisites were built
	/// and the task can be ran now.
	void DecrTaskPrereq( TaskIdType task_id );

	/// @brief when the given task finishes, notify the tasks for which it is a prerequisite
	void NotifyTaskContrib( TaskIdType task_id );

	/// @brief generate the execute commmand string for the given task
	bool GetTaskCmd( TaskIdType task_id, std::string& cmd, bool expanded = true );
	/// @brief generate the directory name where the given task should be run
	void GetTaskDir( TaskIdType task_id, std::string& dir, bool relative = true );

	/// @brief initialize task environment, if not yet
	void InitTaskEnv( KTask* t );

	/// @brief initialize task environment, if not yet
	void InitTaskEnv( TaskIdType task_id );

	/// @brief dump build state in textual form
	void DumpBuildState( std::ostream& o );

	/// @brief initialize the instance
	BState( KModel* kmm, DGraph* dgg );
};

} // k

#endif // DATAK_HH_
