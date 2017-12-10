#ifndef KTR_TASK_HH
#define KTR_TASK_HH

#include "KTypes.hh"

namespace Ktr {

/// @brief task, the arguments for the rule, combined together giving the environment, the directory and the command to execute
struct Task {
	/// unique id of the object, the primary key
	TaskIdType task_id;
	/// directory where the rule is defined and command should be executed in
	DirIdType dir_id;
	/// environment to use
	EnvIdType env_id;	
	/// resolved id of the rule, or 0
	RuleIdType rule_id;
	/// name of the rule
	std::string rule_name;

	Task();
};

struct TaskTable {
	/// parent model
	Model *model;
	/// map task id to task object
	std::map<TaskIdType, Task*>         tasks;

	/// c'tor
	TaskTable( Model *m );

	/// generate new unique task id
	TaskIdType
	NextTaskId() const;

	/// get task object
	Task*
	LookUpTask( TaskIdType task_id );

	/// add task, with rule by name
	Task*
	AddTask( Dir* dir, const std::string& rule_name );

	/// add task, with given rule
	Task*
	AddTask( Dir* dir, Rule* rule );

	/// dump
	void
	Dump( std::ostream &o );
};

}


#endif // KTR_TASK_HH
