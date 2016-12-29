
#include "datak.hh"
#include "utilk.hh"

#include <list>


// --------------------------------------------------
//
//  model
//
// --------------------------------------------------

using namespace Ktr;

KModel*
Ktr::KModelCreate( const std::string& root_dir )
{
	KModel* m = new KModel;
	m->km = new KModelData;
	m->km->root_dir = root_dir;
	m->add_dir( std::string() ); // create root dir object
	return m;
}

KDir*
KModel::
find_dir( const std::string& dir )
{
	if (dir.empty() || dir == this->km->root_dir) {
		if (this->km->root_kdir_id)
			return this->dirs[ this->km->root_kdir_id ];
	}
	else {
		for ( auto II : this->dirs )
			if (II.second->dir_name == dir)
				return II.second;
	}
	return 0;
}

KDir*
KModel::
find_dir( DirIdType kdir_id )
{
	auto I = this->dirs.find( kdir_id );
	return I != this->dirs.end() ? I->second : NULL;
}

KDir*
KModel::
add_dir( const std::string& dir )
{
	KDir *d = NULL;

	// to do: need context (parent dir) to expand the dir name.

	d = this->find_dir( dir );
	if (d)
		return d;

	if (dir.empty()) {
		// root dir
		if (this->km->root_kdir_id == 0) {
			d = new KDir;
			// assert( this->dirs.empty() );
			d->kdir_id = RootDirId;
			d->parent_dir_id = InvalidDir;
			d->kenv_id = 0;
			d->dir_name = std::string();
			this->dirs[ d->kdir_id ] = d;
			this->km->root_kdir_id = RootDirId;
		}
	}
	else {
		if (dir[0] == '/') {
			d = new KDir;
			d->kdir_id = next_dir_id();
			d->parent_dir_id = RootDirId; // parent to root
			d->kenv_id = 0;
			d->dir_name = dir;
			this->dirs[ d->kdir_id ] = d;
		}
		else {
			KDir* parent;
			if (dir.find('/') != std::string::npos) {
				parent = this->add_dir( dirname( dir ) ); // recurse
			}
			else {
				parent = this->dirs[ this->km->root_kdir_id ];
			}
			d = new KDir;
			d->kdir_id = next_dir_id();
			d->parent_dir_id = parent->kdir_id;
			d->kenv_id = 0;
			d->dir_name = dir;
			this->dirs[ d->kdir_id ] = d;
		}
	}

	// automatically add environment.
	if (d->kenv_id == 0) {
		KEnv *env;
		int parent_env_id = 0;

		if (d->parent_dir_id > 0) {
			parent_env_id = this->dirs[ d->parent_dir_id ]->kenv_id;
		}
		env = this->add_env( parent_env_id );
		d->kenv_id = env->kenv_id;
	}

	return d;
}

KEnv*
KModel::
add_env( int parent_env_id )
{
	KEnv* env;
	env = new KEnv;
	env->kenv_id = this->next_env_id();
	env->parent_env_id = parent_env_id;
	this->envs[ env->kenv_id ] = env;
	return env;
}

KRule*
KModel::
find_rule( KDir* dir, const std::string& rule_name, bool recurse )
{
	for ( auto II : this->rules )
		if (II.second->kdir_id == dir->kdir_id && II.second->rule_name == rule_name)
			return II.second;

	if ( recurse && dir->parent_dir_id )
		return find_rule( this->dirs[dir->parent_dir_id], rule_name );

	return NULL;
}

KRule*
KModel::
add_rule( KDir* dir, const std::string& rule_name )
{
	KRule* r = NULL;

	r = find_rule( dir, rule_name, false );
	if (r)
		return r;

	r = new KRule;
	r->krule_id = next_rule_id();
	r->kdir_id = dir->kdir_id;
	//r->kenv_id = 0; // should go to dir env instead
	r->num_inp = -1;
	r->num_outp = -1;
	r->rule_name = rule_name;

	this->rules[ r->krule_id ] = r;

	return r;
}

KVar*
KModel::
find_var( int env_id, const std::string& var_name, bool recurse )
{
	if (!env_id)
		return NULL;

	for ( auto I : this->vars[ env_id ] ) {
		if (I->var_name == var_name)
			return I;
	}

	if ( recurse ) {
		KEnv* env = this->envs[ env_id ];
		if (env)
			return this->find_var( env->parent_env_id, var_name );
	}

	return NULL;
}

KVar*
KModel::
add_var( int env_id, const std::string& var_name, const std::string& value )
{
	KVar *v;
	// assert( env_id > 0 )
	v = this->add_var( env_id, var_name );
	if (v)
		v->var_value = value;
	return v;
}

KVar*
KModel::
add_var( int env_id, const std::string& var_name )
{
	KVar* v;
	// assert( env_id > 0 )
	v = find_var( env_id, var_name, false );
	if (v)
		return v;
	v = new KVar;
	v->kenv_id = env_id;
	v->var_name = var_name;

	this->vars[ env_id ].push_back( v );

	return v;
}

std::string
KModel::
expand_var_string( int env_id, const std::string &str )
{
	size_t i;
	std::string ret;
	i = 0;
	while (i<str.length()) {
		if (str[i] != '%') {
			ret.push_back( str[i] );
			i++;
		}
		else {
			size_t s, e;
			if (str[i+1] == '{') {
				s = i+2;
				e = s;
				while (str[e] != '}')
					e++;

				std::string var_name = str.substr( s, e-s );
				KVar* v = find_var( env_id, var_name );
				std::string val = v->var_value;
				std::string expanded = expand_var_string( env_id, val );
				ret += expanded;
				i = e+1;
			}
			else {
				ret.push_back( str[i] );
				i++;
			}
		}
	}
	return ret;
}

::KObject*
KModel::
add_object( KDir* dir, const std::string& name_ )
{
	KObject* o = NULL;

	if (name_.empty())
		return o;

	std::string name = expand_var_string( dir->kenv_id, name_ );

	if (name.find('/') == std::string::npos) {
		// local file
		o = new KObject;
		o->kobj_id = this->next_obj_id();
		o->kdir_id = dir->kdir_id;
		o->obj_name = name;
		this->objects[ o->kobj_id ] = o;
	}
	else {
		// non-local file: put under correct directory
		std::string dir_name;
		std::string file_name;
		KDir* od;
		if (name[0] == '/') {
			// abs file name
			dir_name = dirname( name );
			file_name = basename( name );
		}
		else {
			std::string tmp0;
			if (not dir->dir_name.empty()) {
				tmp0 += dir->dir_name;
				tmp0 += "/";
			}
			tmp0 += name;
			std::string tmp = normalize_path( tmp0, NULL );
			dir_name = dirname( tmp );
			file_name = basename( tmp );
		}
		od = this->add_dir( dir_name );
		o = new KObject;
		o->kobj_id = this->next_obj_id();
		o->kdir_id = od->kdir_id;
		o->obj_name = file_name;
		this->objects[ o->kobj_id ] = o;
	}

	return o;
}

KObject*
KModel::
find_object( ObjIdType kobj_id )
{
	auto I = this->objects.find( kobj_id );
	return (I != this->objects.end()) ? I->second : NULL;
}

std::string
KModel::
get_object_name( ObjIdType kobj_id )
{
	KObject *obj = this->find_object( kobj_id );
	KDir *dir = obj ? this->find_dir( obj->kdir_id ) : NULL;
	std::string full_obj_name;

	if (obj && dir) {
		if (dir->dir_name.empty())
			full_obj_name = obj->obj_name;
		else
			full_obj_name = dir->dir_name + "/" + obj->obj_name;
	}
	else {
		full_obj_name = "<error>";
	}

	return full_obj_name;
}

std::string
KModel::
get_object_name( KObject *obj )
{
	return get_object_name( obj ? obj->kobj_id : InvalidObject );
}


KTask*
KModel::
add_task( KDir* dir, const std::string& rule_name )
{
	KTask* t;
	t = new KTask;
	t->ktask_id = this->next_task_id();
	t->kdir_id = dir->kdir_id;
	t->krule_id = InvalidRule;
	t->kenv_id = 0;
	t->rule_name = rule_name;
	this->tasks[ t->ktask_id ] = t;
	return t;
}

KTask*
KModel::
add_task( KDir* dir, KRule* rule )
{
	KTask* t;
	t = new KTask;
	t->ktask_id = this->next_task_id();
	t->kdir_id = dir->kdir_id;
	t->krule_id = rule->krule_id;
	t->kenv_id = 0;
	t->rule_name = rule->rule_name;
	this->tasks[ t->ktask_id ] = t;
	return t;
}

KTaskObject*
KModel::
task_add_object( KTask* task, KObject* obj,
	KTaskObject::Role role, const std::string& obj_orig_name )
{
	KTaskObject *ot;

	//ot = find_task_obj( task, obj );

	ot = new KTaskObject;
	ot->ktask_obj_id = this->next_task_obj_id();
	ot->ktask_id = task->ktask_id;
	ot->kobj_id = obj->kobj_id;
	ot->role = role;
	ot->obj_orig_name = obj_orig_name.empty() ? obj->obj_name : obj_orig_name ;

	this->obj_task_rel[ ot->ktask_obj_id ] = ot;
	this->task_objs[ ot->ktask_id ].push_back( ot );

	return ot;
}

KTaskObject*
KModel::
task_add_object( KTask* task, KTaskObject* ot,
	KTaskObject::Role role )
{
	return task_add_object
		(
			task,
			this->objects[ ot->kobj_id ],
			role,
			ot->obj_orig_name 
		);
}

KTaskObject*
KModel::
find_task_obj( KTask* t, KObject* o )
{
	return find_task_obj( t->ktask_id, o->kobj_id );
}

KTaskObject*
KModel::
find_task_obj( TaskIdType task_id, ObjIdType obj_id )
{
	for ( auto I : this->task_objs[ task_id ] ) {
		if ( I->kobj_id == obj_id )
			return I;
	}
	return NULL;
}

void
KModel::
dump( std::ostream& o, KEntityType ent )
{
	switch(ent) {
	case KDIR:
		o << "dir_id\tparent\tenv_id\tname" << std::endl;
		for ( auto I : this->dirs ) {
			const KDir* d = I.second;
			o << d->kdir_id << '\t';
			o << d->parent_dir_id << '\t';
			o << d->kenv_id << '\t';
			o << d->dir_name << std::endl;
		}
		break;
	case KENV:
		o << "env_id\tparent_env_id" << std::endl;
		for ( auto I : this->envs )
			o << I.second->kenv_id << '\t'
				<< I.second->parent_env_id << std::endl;
		break;
	case KVAR:
		o << "env_id\tname\tvalue" << std::endl;
		for ( auto I : this->vars )
			for ( auto II : I.second )
				o << I.first << '\t' 
					<< II->var_name << '\t'
					<< II->var_value << std::endl;
		break;
	case KRULE:
		o << "dir_id\trule_id\tname\tinp\toutp\tcommand" << std::endl;
		for ( auto I : this->rules ) {
			const KRule* r = I.second;
			o << r->kdir_id << '\t';
			o << r->krule_id << '\t';
			o << r->rule_name << '\t';
			//o << r->kenv_id << '\t';
			o << r->num_inp << '\t';
			o << r->num_outp << '\t';
			o << r->command << std::endl;
		}
		break;
	case KOBJECT:
		o << "dir_id\tobj_id\tsubdir\tname" << std::endl;
		for ( auto I : this->objects ) {
			o << I.second->kdir_id << 
				'\t' << I.second->kobj_id <<
				'\t' << this->dirs[ I.second->kdir_id ]->dir_name << 
				'\t' << I.second->obj_name <<
				std::endl;
		}
		break;
	case KTASK:
		o << "dir_id\ttask_id\tenv_id\trule_id\trule_name" << std::endl;
		for ( auto I : this->tasks ) {
			// KTask
			o << I.second->kdir_id << '\t'
				<< I.second->ktask_id << '\t'
				<< I.second->kenv_id << '\t'
				<< I.second->krule_id << '\t'
				<< I.second->rule_name << std::endl;
		}
		break;
	case KTASK_OBJ:
		o << "t_o_id\ttask_id\tobj_id\trole\tobj_name" << std::endl;
		for ( auto I : this->task_objs ) {
			for ( auto II : I.second ) {
				o << II->ktask_obj_id <<
					'\t' << II->ktask_id <<
					'\t' << II->kobj_id <<
					'\t' << ( II->role == KTaskObject::INPUT ? "inp" :
						II->role == KTaskObject::OUTPUT ? "outp" : "dep" ) <<
					'\t' << this->get_object_name( II->kobj_id ) <<
					std::endl;
			}
		}
		break;
	default:
		break;
	}
}


// --------------------------------------------------
//
//  DGraph
// 
// --------------------------------------------------

DGraph::
DGraph( KModel* kmm )
{
	this->km = kmm;
}

void
DGraph::
init_dgraph()
{
	fill_outp_task();
	fill_prereq();
	fill_contrib();
}

void
DGraph::
fill_outp_task()
{
	for ( auto I : this->km->task_objs ) {
		for ( auto II : I.second ) {
			KTaskObject* ko = II;
			if (ko->role == KTaskObject::OUTPUT) {
				// assert( this->objMadeByTask.count( ko->kobj_id ) == 0 );
				this->objMadeByTask[ ko->kobj_id ] = ko->ktask_id;
			}
		}
	}
}

void
DGraph::
fill_prereq()
{
	for ( auto I : this->km->task_objs ) {
		for ( auto II : I.second ) {
			KTaskObject* ko = II;
			if (ko->role == KTaskObject::INPUT || ko->role == KTaskObject::DEPNCY) {
				// kobj_id is an input for ko->task_id
				auto P = this->objMadeByTask.find( ko->kobj_id );
				if (P != this->objMadeByTask.end()) {
					// P.second is the task which produces the object
					// ko->task_id depends on P.second
					this->taskPrereqs[ ko->ktask_id ].insert( P->second );
				}
				else {
					// object is not produced. must be a 'source'.
				}
			}
		}
	}
}

void
DGraph::
fill_contrib()
{
	// reverse the prereq
	for ( auto I : this->taskPrereqs ) {
		TaskIdType task_b = I.first;
		for ( auto II : I.second ) {
			TaskIdType task_a = II;
			this->taskContribTo[ task_a ].insert( task_b );
		}
	}
}

void
DGraph::
dump_dgraph( std::ostream& o )
{
	//std::map< int, int > objMadeByTask; // obj a is produced by task b
	//std::map< int, std::set<int> > taskPrereqs; // task a depends on { b }
	//std::map< int, std::set<int> > taskContribTo; // task a contributes to { b }

	o << "out_obj\ttask_id" << std::endl;
	for ( auto I : this->objMadeByTask )
		o << I.first << '\t' << I.second << std::endl;
	o << "task\treq task" << std::endl;
	for ( auto I : this->taskPrereqs )
		for ( auto II : I.second )
			o << I.first << '\t' << II << std::endl;
	o << "task\this->taskContribTo task" << std::endl;
	for ( auto I : this->taskContribTo )
		for ( auto II : I.second )
			o << I.first << '\t' << II << std::endl;
}

// --------------------------------------------------
//
//  bstate
// 
// --------------------------------------------------

BState::
BState( KModel* kmm, DGraph* dgg )
	: dg( dgg ), km(kmm)
{
}

void
BState::
fill_states_for_obj( ObjIdType obj_id )
{
	auto I = dg->objMadeByTask.find(obj_id);
	if (I == dg->objMadeByTask.end())
		return; // no producers: must be source, nothing to do.
	fill_states_for_task( I->second );
}

void
BState::
fill_states_for_task( TaskIdType task_id )
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

		BTaskState &ts = this->taskStates[ task_id ];
		ts.ktask_id = task_id;
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
BState::
fill_build_queue()
{
	for ( auto I : this->taskStates ) {
		BTaskState &ts = I.second;
		this->buildQueue[ ts.num_prereq ].insert( ts.ktask_id );
	}
}

void
BState::
decr_prereq( TaskIdType task_id )
{
	if (this->taskStates.count(task_id) > 0) {
		BTaskState &ts = this->taskStates[task_id];
		if (ts.num_prereq > 0) {
			this->buildQueue[ ts.num_prereq ].erase( ts.ktask_id );
			ts.num_prereq--;
			this->buildQueue[ ts.num_prereq ].insert( ts.ktask_id );
		}
		else if (ts.num_prereq == 0) {
			this->buildQueue[ 0 ].erase( ts.ktask_id );
		}
		else {
			// something is wrong.
		}
	}
}

void
BState::
notify_contrib( TaskIdType task_id )
{
	if (dg->taskContribTo.count(task_id) > 0) {
		for ( TaskIdType other_task_id : dg->taskContribTo[task_id] ) {
			decr_prereq( other_task_id );
		}
	}
}

void
BState::
update_build_state( TaskIdType task_id, BTaskStateType new_state )
{
	if (this->taskStates.count(task_id)) {
		BTaskState &ts = this->taskStates[task_id];
		switch (ts.state) {
		case TASK_PENDING:
			ts.state = new_state;
			if (ts.state == TASK_FINISHED)
				notify_contrib( task_id );
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
				notify_contrib( task_id );
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
BState::
init_task_env( KTask* t )
{
	if (t->kenv_id != 0)
		return;

	KEnv* env = km->add_env( km->dirs[ t->kdir_id ]->kenv_id );
	t->kenv_id = env->kenv_id;

	std::vector<std::string> inp, outp;
	int inp_num = 1;
	int outp_num = 1;
	for ( auto I : 	km->task_objs[t->ktask_id] ) {
		std::vector<std::string> *p = NULL;
		const char *name;
		int num;
		if (I->role == KTaskObject::INPUT) {
			p = &inp;
			name = "input";
			num = inp_num++;
		}
		else if (I->role == KTaskObject::OUTPUT) {
			p = &outp;
			name = "output";
			num = outp_num++;
		}
		else {
			continue;
		}
		p->push_back( I->obj_orig_name );
		km->add_var( t->kenv_id, F("%s%d", name, num), I->obj_orig_name );
	}

	km->add_var( t->kenv_id, "input", join( ' ', inp ) );
	km->add_var( t->kenv_id, "output", join( ' ', outp ) );
}

void
BState::
init_task_env( TaskIdType task_id )
{
	init_task_env( km->tasks[ task_id ] );
}

bool
BState::
get_task_cmd( TaskIdType task_id, std::string& cmd, bool expanded )
{
	KTask* t;
	KRule* r;

	cmd.clear();

	t = km->tasks[ task_id ];
	if (t->krule_id == InvalidRule) {
		init_task_env( t );
		std::string rule_name = km->expand_var_string( t->kenv_id, t->rule_name );
		r = km->find_rule( km->dirs[ t->kdir_id ], rule_name );
		if (r)
			t->krule_id = r->krule_id;
	}
	else {
		r = km->rules[ t->krule_id ];
	}
	if (!r)
		return false;

	if (!expanded) {
		cmd = r->command;
	}
	else {
		init_task_env( t );
		cmd = km->expand_var_string( t->kenv_id, r->command );
	}
	return true;
}

void
BState::
get_task_dir( TaskIdType task_id, std::string& dir, bool relative )
{
	KTask* t;
	int dir_id;
	KDir* d;

	t = km->tasks[ task_id ];
	dir_id = t->kdir_id;
	d = km->dirs[ dir_id ];

	if (relative) {
		dir = d->dir_name;
	}
	else { // absolute
		if (d->dir_name.empty()) {
			dir = km->km->root_dir;
		}
		else if (d->dir_name[0] == '/') {
			dir = d->dir_name;
		}
		else {
			dir = km->km->root_dir + "/" + d->dir_name;
		}
	}
	if (dir.empty())
		dir += ".";
}

void
BState::
dump_bstate( std::ostream& o )
{
	//std::map< int, BTaskState > taskStates;
	o << "task\tnprereq\tstate\tdir\tcmd" << std::endl;
	for ( auto I : this->taskStates ) {
		std::string dir, cmd;
		get_task_dir( I.second.ktask_id, dir );
		get_task_cmd( I.second.ktask_id, cmd );
		o << I.second.ktask_id << '\t'
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

// --------------------------------------------------

#if defined(DATAK_SELFTEST)

int test1()
{
	KModel* m;
	KDir* d;
	KVar* v;
	KRule* r_cxx;
	KRule* r_link;
	KTask* t;
	KObject* src;
	KObject* obj;
	KObject* exe;
	std::list< KObject* > srcs;
	std::list< KObject* > objs;

	m = KModelCreate( "." );
	d = m->find_dir( std::string() );
	v = m->add_var( d->kenv_id, "CXX", "c++" );
	v = m->add_var( d->kenv_id, "CXXFLAGS", "-O0 -std=c++11 -Wall -g" );

	r_cxx = m->add_rule( d, "compile.cxx" );
	r_cxx->command = "%{CXX} %{CXXFLAGS} %{input} -c -o %{output}";

	r_link = m->add_rule( d, "link.cxx" );
	r_link->command = "%{CXX} %{CXXFLAGS} %{input} -o %{output}";

	src = m->add_object( d, "hello_world.cc" );
	obj = m->add_object( d, "hello_world.o" );
	exe = m->add_object( d, "hello_world" );

	t = m->add_task( d, "compile.cxx" );
	m->task_add_object( t, src, KTaskObject::INPUT );
	m->task_add_object( t, obj, KTaskObject::OUTPUT );

	t = m->add_task( d, "link.cxx" );
	m->task_add_object( t, obj, KTaskObject::INPUT );
	m->task_add_object( t, exe, KTaskObject::OUTPUT );

	std::cout << "--- dirs --------" << std::endl; m->dump( std::cout, Ktr::KDIR );
	std::cout << "--- envs --------" << std::endl; m->dump( std::cout, Ktr::KENV );
	std::cout << "--- vars --------" << std::endl; m->dump( std::cout, Ktr::KVAR );
	std::cout << "--- rules -------" << std::endl; m->dump( std::cout, Ktr::KRULE );
	std::cout << "--- tasks -------" << std::endl; m->dump( std::cout, Ktr::KTASK );
	std::cout << "--- task_objs ---" << std::endl; m->dump( std::cout, Ktr::KTASK_OBJ );
	std::cout << "--- objs --------" << std::endl; m->dump( std::cout, Ktr::KOBJECT );

	DGraph* gg = new DGraph( m );
	gg->init_dgraph();
	std::cout << "--- dgraph ------" << std::endl; gg->dump_dgraph( std::cout );

	BState* bs = new BState( m, gg );
	bs->fill_states_for_obj( exe->kobj_id );
	bs->fill_build_queue();
	std::cout << "--- bstate ------" << std::endl; bs->dump_bstate( std::cout );

	return 0;
}

int test2()
{
	KModel* m;
	KDir* d;
	KVar* v;
	KRule* r_cxx;
	KRule* r_link;
	KTask* t;
	KObject* exe;
	//std::list< KObject* > srcs;
	//std::list< KObject* > objs;
	std::list< KTaskObject* > objs;
	std::vector< std::string > src_names = { "ktor.cc", "utilk.cc", "maink.cc", "filek.cc" };

	m = KModelCreate( "." );
	d = m->find_dir( std::string() );
	v = m->add_var( d->kenv_id, "CXX", "c++" );
	v = m->add_var( d->kenv_id, "CXXFLAGS", "-O0 -std=c++11 -Wall -g" );

	r_cxx = m->add_rule( d, "compile.cxx" );
	r_cxx->command = "%{CXX} %{CXXFLAGS} %{input} -c -o %{output}";

	r_link = m->add_rule( d, "link.cxx" );
	r_link->command = "%{CXX} %{CXXFLAGS} %{input} -o %{output}";

	exe = m->add_object( d, "ktr" );

	for ( auto fname : src_names ) {
		KObject* src = m->add_object( d, fname );
		//srcs.push_back( src );

		std::string obj_name = "o/" + fname;
		obj_name.replace(obj_name.find(".cc"), std::string::npos, ".o");
		KObject* obj = m->add_object( d, obj_name );
		//objs.push_back( obj );

		t = m->add_task( d, r_cxx );
		m->task_add_object( t, src, KTaskObject::INPUT );
		// KTaskObject*
		auto x = m->task_add_object( t, obj, KTaskObject::OUTPUT, obj_name );
		objs.push_back( x );
	}
	t = m->add_task( d, "link.cxx" );
	m->task_add_object( t, exe, KTaskObject::OUTPUT );
	for ( auto tobj : objs ) {
		m->task_add_object( t, tobj, KTaskObject::INPUT );
	}

	std::cout << "--- dirs --------" << std::endl; m->dump( std::cout, Ktr::KDIR );
	std::cout << "--- envs --------" << std::endl; m->dump( std::cout, Ktr::KENV );
	std::cout << "--- vars --------" << std::endl; m->dump( std::cout, Ktr::KVAR );
	std::cout << "--- rules -------" << std::endl; m->dump( std::cout, Ktr::KRULE );
	std::cout << "--- tasks -------" << std::endl; m->dump( std::cout, Ktr::KTASK );
	std::cout << "--- task_objs ---" << std::endl; m->dump( std::cout, Ktr::KTASK_OBJ );
	std::cout << "--- objs --------" << std::endl; m->dump( std::cout, Ktr::KOBJECT );

	DGraph* gg = new DGraph( m );
	gg->init_dgraph();
	std::cout << "--- dgraph ------" << std::endl; gg->dump_dgraph( std::cout );

	BState* bs = new BState( m, gg );
	bs->fill_states_for_obj( exe->kobj_id );
	bs->fill_build_queue();
	std::cout << "--- bstate ------" << std::endl; bs->dump_bstate( std::cout );

	return 0;
}

int main()
{
	if (0) {
		std::cout << "=== test1 ======" << std::endl;
		test1();
		std::cout << std::endl;
	}

	std::cout << "=== test2 ======" << std::endl;
	test2();
}

#endif
