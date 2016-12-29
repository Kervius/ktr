#ifndef DATAK_HH_
#define DATAK_HH_

#include <string>
#include <map>
#include <set>
#include <vector>

#include <ostream>
#include <iostream>

namespace Ktr { /// Ktr namespace

enum TaskIdType { InvalidTask = 0 };
enum RuleIdType { InvalidRule = 0 };
enum ObjIdType { InvalidObject = 0 };
enum EnvIdType { InvalidEnv = 0 };
enum DirIdType { InvalidDir = 0, RootDirId = 1, };
enum TaskObjIdType { InvalidTaskObj = 0 };

/// the root of the data model, containing the global settings
struct KGlobalData {
	std::string root_dir;	/// root directory
	DirIdType root_kdir_id = InvalidDir;	/// id of the root kdir entity
};

/// directory entity
struct KDir {
	DirIdType kdir_id = InvalidDir;	/// unique id of the entity, the primary key
	DirIdType parent_dir_id;	/// id of the parent dir, or 0
	int kenv_id;		/// id of the environment of the dir
	std::string dir_name;	/// directory name: relative to root, or absolute if outside the root
};

/// variable entity
struct KVar {
	int kenv_id;		/// id of the environment the variable belongs to
	std::string var_name;	/// variable name
	std::string var_value;	/// variable value
};

/// environment entity, a set of variables
struct KEnv {
	int kenv_id;		/// id of the environment, the primary key
	int parent_env_id;	/// id of the parent environment
};

/// rule entity, a template of the command for the task entities
struct KRule {
	RuleIdType krule_id = InvalidRule;		/// unique id of the rule, the primary key
	DirIdType kdir_id;		/// id of the directory where the rule was defined
	//int kenv_id;		/// id of the per-rule environment, if any (prio: dir <- rule <- task)
	int num_inp;		/// number of inputs the rule has
	int num_outp;		/// number of outputs the rule has
	std::string command;	/// the template string of the comment to execute
	std::string rule_name;	/// the public name of the rule, used for late binding with the tasks
};

/// object, a file/etc inside the system
struct KObject {
	ObjIdType kobj_id = InvalidObject;		/// unique id of the object, the primary key
	DirIdType kdir_id;		/// id of the directory where the object lies
	std::string obj_name;	/// name of the object, without directory components (see also KTaskObject::obj_orig_name)
};

/// task, the arguments for the rule, combined together giving the environment, the directory and the command to execute
struct KTask {
	TaskIdType ktask_id = InvalidTask;	/// unique id of the object, the primary key
	DirIdType kdir_id;	/// directory where the rule is defined and command should be executed in
	int kenv_id;		/// environment to use
	RuleIdType krule_id;	/// resolved id of the rule, or 0
	std::string rule_name;	/// name of the rule
};

/// relation entity, linking a task with input/output/dependent objects
struct KTaskObject {
	enum Role {		/// defines roles of the objects
		INPUT,		/// input object (aka input file(s))
		OUTPUT,		/// output object (aka output file(s))
		DEPNCY,		/// dependent object (extra dependencies)
	};
	TaskObjIdType ktask_obj_id = InvalidTaskObj;	/// unique id of the entity
	TaskIdType ktask_id;				/// task
	ObjIdType kobj_id;				/// object id
	Role role;					/// type of the object
	std::string obj_orig_name;	/// original name of the object. the KObject would contain
					/// the normalized file name, which not necessarily matches
					/// the name as specified by the user when defining the task.
					/// the model in memory requires normalized names, while for command
					/// execution, the original name is required
};

/// id of ktr entity
enum KEntityType {
	KATTR,
	KDIR,
	KENV,
	KVAR,
	KRULE,
	KOBJECT,
	KTASK,
	KTASK_OBJ,
};

/// attribute entity
/// kentity_id + type point to the entity the attribute is attached to
struct KAttribute {
	int kattr_id = 0;		/// unique id of the attribute
	int kentity_id;		/// id of the entity
	KEntityType type;	/// type of the entity
	std::string name;	/// name of the attribute
	std::string value;	/// optional value of the attribute
};

/// Container for the model of the build system
struct KModel {
	KGlobalData* km;

	/// map pf dir id to dir object
	std::map<int, KDir*>          dirs;

	/// map of dir id to list of default object ids
	std::map<int, std::vector<int>>   dir_defaults;

	/// map env id to list of vars (as they appear)
	std::map<int, std::vector<KVar*>> vars;

	/// map env id to env object
	std::map<int, KEnv*>          envs;

	/// map rule id to rule object
	std::map<RuleIdType, KRule*>         rules;

	/// map obj id to object
	std::map<ObjIdType, KObject*>        objects;

	/// map task id to task object
	std::map<TaskIdType, KTask*>         tasks;

	/// map task-obj id to task-obj relation object
	std::map<int, KTaskObject*> obj_task_rel;

	/// map task id to list of task-obj objects
	std::map<int, std::vector<KTaskObject*>> taskObjs;

	// attr_id -> attribute object
	std::map<int,KAttribute*>         attrs;

	// next id
	int NextEnvId() const { return ( this->envs.empty() ? 1 : this->envs.rbegin()->first + 1 ); }
	
	RuleIdType
	NextRuleId() const { return RuleIdType( this->rules.empty() ? 1 : this->rules.rbegin()->first + 1 ); }

	DirIdType
	NextDirId() const { return DirIdType( this->rules.empty() ? 1 : this->dirs.rbegin()->first + 1 ); }

	ObjIdType
	NextObjId() const { return ObjIdType( this->objects.empty() ? 1 : this->objects.rbegin()->first + 1 ); }

	TaskIdType
	NextTaskId() const { return TaskIdType( this->tasks.empty() ? 1 : this->tasks.rbegin()->first + 1 ); }

	TaskObjIdType
	NextTaskObjId() const
	{
		return TaskObjIdType( this->obj_task_rel.empty() ? 1 : this->obj_task_rel.rbegin()->first + 1 );
	}

	// kdir
	KDir*
	AddDir( const std::string& dir );

	KDir*
	FindDir( const std::string& dir );

	KDir*
	FindDir( DirIdType kdir_id );

	KEnv*
	AddEnv( int parent_env_id );

	// KRule
	KRule*
	AddRule( KDir* dir, const std::string& rule_name );

	KRule*
	FindRule( KDir* dir, const std::string& rule_name, bool recurse = true );

	// var
	KVar*
	AddVar( int env_id, const std::string& var_name );

	KVar*
	AddVar( int env_id, const std::string& var_name, 
		const std::string& value );

	KVar*
	FindVar( int env_id, const std::string& var_name, bool recurse = true );

	std::string
	ExpandVarString( int env_id, const std::string &str );

	// task
	KTask*
	AddTask( KDir* dir, const std::string& rule_name );

	KTask*
	AddTask( KDir* dir, KRule* rule );

	// object
	KObject*
	AddObject( KDir* dir, const std::string& name );

	KObject*
	FindObject( ObjIdType kobj_id );

	std::string
	GetObjectName( ObjIdType kobj_id );

	std::string
	GetObjectName( KObject *obj );

	// taskObjs
	KTaskObject*
	TaskAddObject( KTask* task, KObject* obj,
		KTaskObject::Role role, const std::string& obj_orig_name = std::string() );

	KTaskObject*
	TaskAddObject( KTask* task, KTaskObject* ot,
		KTaskObject::Role role );

	KTaskObject*
	FindTaskObj( KTask* t, KObject* o );

	KTaskObject*
	FindTaskObj( TaskIdType task_id, ObjIdType obj_id );

	// dump
	void Dump( std::ostream& o, KEntityType );
};

KModel* KModelCreate( const std::string& root_dir );

} // Ktr

// --------------------------------------------------

namespace Ktr {

/// dependency graph for the model
struct DGraph {
	KModel* km;

	DGraph( KModel* kmm );

	std::map< ObjIdType, TaskIdType > objMadeByTask; /// obj a is produced by task b
	std::map< TaskIdType, std::set<TaskIdType> > taskPrereqs; /// task a depends on { b }
	std::map< TaskIdType, std::set<TaskIdType> > taskContribTo; /// task a contributes to { b }

	void InitDepGraph();
	void FillOutpTask();
	void FillPrereq();
	void FillContrib();
	void DumpDepGraph( std::ostream& o );
};

// build state for the model and the dep graph

enum BTaskStateType {
	TASK_FINISHED,
	TASK_RUNNING,
	TASK_PENDING,
};

struct BTaskState {
	TaskIdType ktask_id;
	BTaskStateType state;
	int num_prereq;
};

struct BState
{
	KModel* km;
	DGraph* dg;

	std::map< TaskIdType, BTaskState > taskStates; // task_id to task state

	void FillStatesForObj( ObjIdType obj_id );
	void FillStatesForTask( TaskIdType task_id );

	std::map< int, std::set<TaskIdType> > buildQueue; // count to set of tasks

	void FillBuildQueue();
	void UpdateBuildState( TaskIdType task_id, BTaskStateType new_state );
	void DecrTaskPrereq( TaskIdType task_id );
	void NotifyTaskContrib( TaskIdType task_id );
	bool GetTaskCmd( TaskIdType task_id, std::string& cmd, bool expanded = true );
	void GetTaskDir( TaskIdType task_id, std::string& dir, bool relative = true );

	void InitTaskEnv( KTask* t );
	void InitTaskEnv( TaskIdType task_id );

	void DumpBuildState( std::ostream& o );
	BState( KModel* kmm, DGraph* dgg );
};

} // k

#endif // DATAK_HH_
