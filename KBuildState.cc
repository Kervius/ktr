
#include "KBuildState.hh"

namespace Ktr {

BuildState::
BuildState( Model* m, DepGraph* dgg )
: model( m )
, dg( dgg )
{
}

void
BuildState::
FillStatesForObj( ObjIdType obj_id )
{
	auto I = dg->objectProducer.find(obj_id);
	if (I == dg->objectProducer.end())
		return; // no producers: must be source, nothing to do.
	FillStatesForTask( I->second );
}

void
BuildState::
FillStatesForTask( TaskIdType task_id )
{
	std::list<TaskIdType> task_list;

	task_list.push_back( task_id );
	while (not task_list.empty())
	{
		TaskIdType task_id = task_list.front();
		task_list.pop_front();

		// check if was already seen
		if (this->taskStates.find( task_id ) != this->taskStates.end())
			continue;

		TaskBuildState &ts = this->taskStates[ task_id ];
		ts.task_id = task_id;
		ts.num_prereq = 0;
		ts.state = TASK_PENDING;

		auto I = dg->taskPrereqs.find( task_id );
		if (I != dg->taskPrereqs.end()) {
			const std::set<TaskIdType>& prereq = I->second;
			for ( TaskIdType prereq_task_id : prereq ) {
				ts.num_prereq++;
				task_list.push_back( prereq_task_id );
			}
		}
	}
}

void
BuildState::
FillBuildQueue()
{
	// assert( this->buildQueue.empty() );
	for ( auto I : this->taskStates ) {
		TaskBuildState &ts = I.second;
		this->buildQueue[ ts.num_prereq ].insert( ts.task_id );
	}
}

void
BuildState::
DecrTaskPrereq( TaskIdType task_id )
{
	auto I = this->taskStates.find( task_id );
	if (I != this->taskStates.end()) {
		TaskBuildState &ts = I->second;
		if (ts.num_prereq > 0) {
			this->buildQueue[ ts.num_prereq ].erase( ts.task_id );
			ts.num_prereq--;
			this->buildQueue[ ts.num_prereq ].insert( ts.task_id );
		}
		else if (ts.num_prereq == 0) {
			this->buildQueue[ 0 ].erase( ts.task_id );
		}
		else {
			// something is wrong.
		}
	}
}

void
BuildState::
NotifyTaskContrib( TaskIdType task_id )
{
	auto I = dg->taskContribTo.find( task_id );
	if (I != dg->taskContribTo.end()) {
		for ( TaskIdType other_task_id : I->second ) {
			DecrTaskPrereq( other_task_id );
		}
	}
}

void
BuildState::
UpdateBuildState( TaskIdType task_id, TaskBuildStateType new_state )
{
	auto I = this->taskStates.find( task_id );
	if (I != this->taskStates.end()) {
		TaskBuildState &ts = I->second;
		switch (ts.state) {
		case TASK_PENDING:
			ts.state = new_state;
			if (ts.state == TASK_FINISHED)
				NotifyTaskContrib( task_id );
			break;
		case TASK_RUNNING:
			switch(new_state) {
			default:
			case TASK_PENDING:
			case TASK_RUNNING:
				// allow to go back waiting.
				ts.state = new_state;
				break;
			case TASK_FINISHED:
				ts.state = new_state;
				NotifyTaskContrib( task_id );
				break;
			}
			break;
		case TASK_FINISHED:
			switch(new_state) {
			default:
			case TASK_PENDING:
			case TASK_RUNNING:
				// error.
				break;
			case TASK_FINISHED:
				// nothing
				break;
			}
			break;
		}
	}
}

void
BuildState::
InitTaskEnv( Task* t )
{
	if (t->env_id != InvalidEnv)
		return;

	Env* env = model->envs->NewEnv( model->dirs->dirs[ t->dir_id ]->env_id );
	t->env_id = env->env_id;

	std::vector<std::string> inp, outp;
	int inp_num = 1;
	int outp_num = 1;
	for ( auto I : 	model->taskObjs->taskObjsIndex[t->task_id] ) {
		TaskObject *ot = I.second;
		std::vector<std::string> *p = NULL;
		const char *name;
		int num;
		if (ot->role == TaskObject::INPUT) {
			p = &inp;
			name = "input";
			num = inp_num++;
		}
		else if (ot->role == TaskObject::OUTPUT) {
			p = &outp;
			name = "output";
			num = outp_num++;
		}
		else {
			continue;
		}
		p->push_back( ot->obj_orig_name );
		model->envs->AddVar( t->env_id, Utils::F("%s%d", name, num), ot->obj_orig_name );
	}

	model->envs->AddVar( t->env_id, "input", Utils::Join( ' ', inp ) );
	model->envs->AddVar( t->env_id, "output", Utils::Join( ' ', outp ) );
}

void
BuildState::
InitTaskEnv( TaskIdType task_id )
{
	InitTaskEnv( model->tasks->LookUpTask( task_id ) );
}

bool
BuildState::
GetTaskCmd( TaskIdType task_id, std::string& cmd, bool expanded )
{
	Task* t;
	Rule* r;

	cmd.clear();

	t = model->tasks->LookUpTask( task_id );
	if (t->rule_id == InvalidRule) {
		InitTaskEnv( t );
		std::string rule_name = model->envs->ExpandVarString( t->env_id, t->rule_name );
		r = model->rules->FindRule( t->dir_id, rule_name );
		if (r)
			t->rule_id = r->rule_id;
	}
	else {
		r = model->rules->LookUpRule( t->rule_id );
	}
	if (!r)
		return false;

	if (!expanded) {
		cmd = r->command;
	}
	else {
		InitTaskEnv( t );
		cmd = model->envs->ExpandVarString( t->env_id, r->command );
	}
	return true;
}

void
BuildState::
GetTaskDir( TaskIdType task_id, std::string& dir, bool relative )
{
	Task* t;
	DirIdType dir_id;
	Dir* d;

	t = model->tasks->LookUpTask( task_id );
	dir_id = t->dir_id;
	d = model->dirs->LookUpDir( dir_id );

	if (relative) {
		dir = d->dir_name;
	}
	else { // absolute
		if (d->dir_name.empty()) {
			dir = model->globalConf->root_dir;
		}
		else if (d->dir_name[0] == '/') {
			dir = d->dir_name;
		}
		else {
			dir = model->globalConf->root_dir + "/" + d->dir_name;
		}
	}
	if (dir.empty())
		dir += ".";
}

void
BuildState::
Reset()
{
	taskStates.clear();
	buildQueue.clear();
}

void
BuildState::
DumpBuildState( std::ostream& o )
{
	//std::map< int, TaskBuildState > taskStates;
	o << "task\tnprereq\tstate\tdir\tcmd" << std::endl;
	for ( auto I : this->taskStates ) {
		std::string dir, cmd;
		GetTaskDir( I.second.task_id, dir );
		GetTaskCmd( I.second.task_id, cmd );
		o << I.second.task_id << '\t'
			<< I.second.num_prereq << '\t'
			<< ( I.second.state == TASK_FINISHED ? "fini" :
				I.second.state == TASK_RUNNING ? "run'n" : "wait'n" )
				<< '\t'
			<< dir << '\t'
			<< cmd 
				<< std::endl;
	}

	o << "nprereq\ttask id" << std::endl;
	for ( auto I : this->buildQueue )
		for ( auto II : I.second )
			o << I.first << '\t' << II << std::endl;
}


}
