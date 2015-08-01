#ifndef DATAK_HH_
#define DATAK_HH_

#include <string>
#include <map>
#include <vector>

#include <ostream>
#include <iostream>

namespace k {
namespace m {

// the root of the data model, containing the global settings
struct k {
	std::string root_dir;
	int root_kdir_id = 0;
};

// directory entity
// kdir_id is the primary key
// kenv_id is the id of the environment
// parent_dir_id is the id of the parent dir, 0 if none
// if dir_name starts with '/', then it is an absolute dir name outside the project.
struct kdir {
	int kdir_id;
	int parent_dir_id;
	int kenv_id;
	std::string dir_name;
};

// variable entity, kenv_id binds the var to env
struct kvar {
	int kenv_id;
	std::string var_name;
	std::string var_value;
};

// environment (collection of variables) entity (dummy)
struct kenv {
	int kenv_id;
	int parent_env_id;
};

// rule entity, describing the rule
// krule_id - primary key, kdir_id - directory it is bound to
// kenv_id - environment it is bound to (generally inherited from the dir).
struct krule {
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
struct kobject {
	int kobj_id;
	int kdir_id;
	std::string obj_name;
};

// task : link rule (template) to actual command
struct ktask {
	int ktask_id;
	int kdir_id;
	int kenv_id;
	int krule_id;
	std::string rule_name;
};

// link task with the objects
enum role_type {
	OBJ_INP, OBJ_OUTP, OBJ_DEP
};
struct ktask_obj {
	int ktask_obj_id;
	int ktask_id;
	int kobj_id;
	role_type role;
};

// generic attribute support
// type of entity the attribute is attached to
enum kentity_type {
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
struct kattr {
	int kattr_id;
	int kentity_id;
	kentity_type type;
	std::string value;
};

struct model {
	k* k;
	std::map<int,kdir*>          dirs;
	std::map<int,std::vector<kvar*>> vars;
	std::map<int,kenv*>         envs;
	std::map<int,krule*>         rules;
	std::map<int,kobject*>       objects;
	std::map<int,ktask*>         tasks;
	std::map<int,ktask_obj*> obj_task_rel;
	std::map<int,std::vector<ktask_obj*>> task_objs;
	std::map<int,kattr*>         attrs;

	// next id
	int next_env_id() const { return ( this->envs.empty() ? 1 : this->envs.rbegin()->first + 1 ); }
	int next_rule_id() const { return ( this->rules.empty() ? 1 : this->rules.rbegin()->first + 1 ); }
	int next_obj_id() const { return ( this->objects.empty() ? 1 : this->objects.rbegin()->first + 1 ); }
	int next_task_id() const { return ( this->tasks.empty() ? 1 : this->tasks.rbegin()->first + 1 ); }
	int next_task_obj_id() const { return ( this->obj_task_rel.empty() ? 1 : this->obj_task_rel.rbegin()->first + 1 ); }

	// kdir
	::k::m::kdir*
	add_dir( const std::string& dir );

	::k::m::kdir*
	find_dir( const std::string& dir );

	// krule
	::k::m::krule*
	add_rule( ::k::m::kdir* dir, const std::string& rule_name );

	::k::m::krule*
	find_rule( ::k::m::kdir* dir, const std::string& rule_name, bool recurse = true );

	// var
	::k::m::kvar*
	add_var( ::k::m::kdir* dir, const std::string& var_name );

	::k::m::kvar*
	add_var( ::k::m::kdir* dir, const std::string& var_name, 
		const std::string& value );

	::k::m::kvar*
	find_var( ::k::m::kdir* dir, const std::string& var_name, bool recurse = true );

	// task
	::k::m::ktask*
	add_task( ::k::m::kdir* dir, const std::string& rule_name );

	::k::m::ktask*
	add_task( ::k::m::kdir* dir, ::k::m::krule* rule );

	// object
	::k::m::kobject*
	add_object( ::k::m::kdir* dir, const std::string& name );

	// task_objs
	::k::m::ktask_obj*
	task_add_object( ::k::m::ktask* task, ::k::m::kobject* obj,
		::k::m::role_type role );

	// dump
	void
	dump( std::ostream& o, ::k::m::kentity_type );
};

::k::m::model* create( const std::string& root_dir );

} // m
} // k



#endif // DATAK_HH_
