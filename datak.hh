#ifndef DATAK_HH_
#define DATAK_HH_

#include <string>
#include <map>
#include <set>
#include <vector>

#include <ostream>
#include <iostream>

namespace Ktr {

// the root of the data model, containing the global settings
struct KModelData {
	std::string root_dir;
	int root_kdir_id = 0;
};

// directory entity
// kdir_id is the primary key
// kenv_id is the id of the environment
// parent_dir_id is the id of the parent dir, 0 if none
// if dir_name starts with '/', then it is an absolute dir name outside the project.
struct KDir {
	int kdir_id;
	int parent_dir_id;
	int kenv_id;
	std::string dir_name;
};

// variable entity, kenv_id binds the var to env
struct KVar {
	int kenv_id;
	std::string var_name;
	std::string var_value;
};

// environment (collection of variables) entity (dummy)
struct KEnv {
	int kenv_id;
	int parent_env_id;
};

// rule entity, describing the rule
// krule_id - primary key, kdir_id - directory it is bound to
// kenv_id - environment it is bound to (generally inherited from the dir).
struct KRule {
	int krule_id;
	int kdir_id;
	//int kenv_id;
	int num_inp;
	int num_outp;
	std::string command;
	std::string rule_name;
};

// object - a file/etc - inside the system
// obj_name is always pure file name
// kdir_id refers to the directory where the object is situated
struct KObject {
	int kobj_id;
	int kdir_id;
	std::string obj_name;
};

// task : link rule (template) to actual command
struct KTask {
	int ktask_id;
	int kdir_id;
	int kenv_id;
	int krule_id;
	std::string rule_name;
};

// link task with the objects
enum KRoleType {
	OBJ_INP, OBJ_OUTP, OBJ_DEP
};
struct KTaskObject {
	int ktask_obj_id;
	int ktask_id;
	int kobj_id;
	KRoleType role;
	std::string obj_orig_name;
};

// generic attribute support
// type of entity the attribute is attached to
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
// attribute, kattr_id is the primary key
// kentity_id + type is the entity the attr is for
struct KAttribute {
	int kattr_id;
	int kentity_id;
	KEntityType type;
	std::string value;
};

struct KModel {
	KModelData* km;
	// kdir_id -> dir object
	std::map<int,KDir*>          dirs;

	// kdir_id -> list of default object ids
	std::map<int,std::vector<int>>   dir_defaults;

	// kenv_id -> list of vars (as they appear)
	std::map<int,std::vector<KVar*>> vars;

	// kenv_id -> env object
	std::map<int,KEnv*>          envs;

	// rule_id -> rule object
	std::map<int,KRule*>         rules;

	// obj_id -> object
	std::map<int,KObject*>       objects;

	// task_id -> task object
	std::map<int,KTask*>         tasks;

	// task_obj_id -> task_obj relation object
	std::map<int,KTaskObject*> obj_task_rel;

	// task_id -> list of task_obj objects
	std::map<int,std::vector<KTaskObject*>> task_objs;

	// attr_id -> attribute object
	std::map<int,KAttribute*>         attrs;

	// next id
	int next_env_id() const { return ( this->envs.empty() ? 1 : this->envs.rbegin()->first + 1 ); }
	int next_rule_id() const { return ( this->rules.empty() ? 1 : this->rules.rbegin()->first + 1 ); }
	int next_obj_id() const { return ( this->objects.empty() ? 1 : this->objects.rbegin()->first + 1 ); }
	int next_task_id() const { return ( this->tasks.empty() ? 1 : this->tasks.rbegin()->first + 1 ); }
	int next_task_obj_id() const { return ( this->obj_task_rel.empty() ? 1 : this->obj_task_rel.rbegin()->first + 1 ); }

	// kdir
	KDir*
	add_dir( const std::string& dir );

	KDir*
	find_dir( const std::string& dir );

	KEnv*
	add_env( int parent_env_id );

	// KRule
	KRule*
	add_rule( KDir* dir, const std::string& rule_name );

	KRule*
	find_rule( KDir* dir, const std::string& rule_name, bool recurse = true );

	// var
	KVar*
	add_var( int env_id, const std::string& var_name );

	KVar*
	add_var( int env_id, const std::string& var_name, 
		const std::string& value );

	KVar*
	find_var( int env_id, const std::string& var_name, bool recurse = true );

	std::string
	expand_var_string( int env_id, const std::string &str );

	// task
	KTask*
	add_task( KDir* dir, const std::string& rule_name );

	KTask*
	add_task( KDir* dir, KRule* rule );

	// object
	KObject*
	add_object( KDir* dir, const std::string& name );

	// task_objs
	KTaskObject*
	task_add_object( KTask* task, KObject* obj,
		KRoleType role, const std::string& obj_orig_name = std::string() );

	KTaskObject*
	task_add_object( KTask* task, KTaskObject* ot,
		KRoleType role );

	KTaskObject*
	find_task_obj( KTask* t, KObject* o );

	KTaskObject*
	find_task_obj( int task_id, int obj_id );

	// dump
	void dump( std::ostream& o, KEntityType );
};

KModel* KModelCreate( const std::string& root_dir );

} // Ktr

// --------------------------------------------------

namespace Ktr {
// dependency graph for the model

struct DGraph {
	KModel* km;
	DGraph( KModel* kmm );
	std::map< int, int > toutp_task; // obj a is produced by task b
	std::map< int, std::set<int> > tprereq; // task a depends on { b }
	std::map< int, std::set<int> > tcontrib; // task a contributes to { b }
	void init_dgraph();
	void fill_outp_task();
	void fill_prereq();
	void fill_contrib();
	void dump_dgraph( std::ostream& o );
};

} // Ktr

// --------------------------------------------------

namespace Ktr {
// build state for the model and the dep graph

enum {
	TASK_FINISHED,
	TASK_RUNNING,
	TASK_PENDING,
};

struct BTaskState {
	int ktask_id;
	int state;
	int num_prereq;
};

struct BState
{
	KModel* km;
	DGraph* dg;

	std::map< int, BTaskState > tstates; // task_id to task state

	void fill_states_for_obj( int obj_id );
	void fill_states_for_task( int task_id );

	std::map< int, std::set<int> > build_queue; // count to set of tasks

	void fill_build_queue();
	void update_build_state( int task_id, int new_state );
	void decr_prereq( int task_id );
	void notify_contrib( int task_id );
	bool get_task_cmd( int task_id, std::string& cmd, bool expanded = true );
	void get_task_dir( int task_id, std::string& dir, bool relative = true );

	void init_task_env( KTask* t );
	void init_task_env( int task_id );

	void dump_bstate( std::ostream& o );
	BState( KModel* kmm, DGraph* dgg );
};

} // k

#endif // DATAK_HH_
