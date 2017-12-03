
#include "KModel.hh"

namespace Ktr {


Task::
Task()
: task_id(InvalidTask)
, dir_id(InvalidDir)
, env_id(InvalidEnv)
, rule_id(InvalidRule)
{
}

TaskTable::
TaskTable( Model *m )
: model(m)
{
}

TaskIdType
TaskTable::
NextTaskId() const
{
	return TaskIdType( this->tasks.empty() ? 1 : this->tasks.rbegin()->first + 1 );
}

Task*
TaskTable::
LookUpTask( TaskIdType task_id )
{
	auto II = this->tasks.find( task_id );
	if (II != this->tasks.end())
		return II->second;
	return nullptr;
}

Task*
TaskTable::
AddTask( Dir* dir, const std::string& rule_name )
{
        Task* t;
        t = new Task;
        t->task_id = this->NextTaskId();
        t->dir_id = dir->dir_id;
        t->rule_id = InvalidRule;
        t->env_id = InvalidEnv;
        t->rule_name = rule_name;
        this->tasks[ t->task_id ] = t;
        return t;
}

Task*
TaskTable::
AddTask( Dir* dir, Rule* rule )
{
        Task* t;
        t = new Task;
        t->task_id = this->NextTaskId();
        t->dir_id = dir->dir_id;
        t->rule_id = rule->rule_id;
        t->env_id = InvalidEnv;
        t->rule_name = rule->rule_name;
        this->tasks[ t->task_id ] = t;
        return t;
}

void
TaskTable::
Dump( std::ostream &o )
{
	o << "dir_id\ttask_id\tenv_id\trule_id\trule_name" << std::endl;
	for ( auto I : this->tasks ) {
		o << I.second->dir_id << '\t'
			<< I.second->task_id << '\t'
			<< I.second->env_id << '\t'
			<< I.second->rule_id << '\t'
			<< I.second->rule_name << std::endl;
	}
}

}
