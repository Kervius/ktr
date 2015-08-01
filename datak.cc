
#include "datak.hh"
#include "utilk.hh"

::k::m::model*
::k::m::
create( const std::string& root_dir )
{
	::k::m::model* m = new ::k::m::model;
	m->k = new ::k::m::k;
	m->k->root_dir = root_dir;
	m->add_dir( std::string() ); // create root dir object
	return m;
}



::k::m::kdir*
::k::m::model::
find_dir( const std::string& dir )
{
	if (dir.empty() || dir == this->k->root_dir) {
		if (this->k->root_kdir_id)
			return this->dirs[ this->k->root_kdir_id ];
	}
	else {
		for ( auto II : this->dirs )
			if (II.second->dir_name == dir)
				return II.second;
	}
	return 0;
}

::k::m::kdir*
::k::m::model::
add_dir( const std::string& dir )
{
	kdir *d = NULL;
	kenv *env = NULL;

	d = this->find_dir( dir );
	if (d)
		return d;

	if (dir.empty()) {
		// root dir
		if (this->k->root_kdir_id == 0) {
			d = new kdir;
			// assert( this->dirs.empty() );
			d->kdir_id = 1;
			d->parent_dir_id = 0;
			d->kenv_id = 0;
			d->dir_name = std::string();
			this->dirs[ d->kdir_id ] = d;
			this->k->root_kdir_id = 1;
		}
	}
	else {
		if (dir[0] == '/') {
			d = new kdir;
			d->kdir_id = this->dirs.rbegin()->first + 1;
			d->parent_dir_id = 1; // parent to root
			d->kenv_id = 0;
			d->dir_name = dir;
			this->dirs[ d->kdir_id ] = d;
		}
		else {
			kdir* parent;
			if (dir.find('/') != std::string::npos) {
				parent = this->add_dir( dirname( dir ) ); // recurse
			}
			else {
				parent = this->dirs[ this->k->root_kdir_id ];
			}
			d = new kdir;
			d->kdir_id = this->dirs.rbegin()->first + 1;
			d->parent_dir_id = parent->kdir_id;
			d->kenv_id = 0;
			d->dir_name = dir;
			this->dirs[ d->kdir_id ] = d;
		}
	}

	// automatically add environment.
	if (d->kenv_id == 0) {
		if (!env) {
			env = new kenv;
			env->kenv_id = this->next_env_id();
			env->parent_env_id = 0;
			if (d->parent_dir_id > 0) {
				env->parent_env_id = this->dirs[ d->parent_dir_id ]->kenv_id;
			}
			this->envs[ env->kenv_id ] = env;
		}
		d->kenv_id = env->kenv_id;
	}

	return d;
}

::k::m::krule*
::k::m::model::
find_rule( ::k::m::kdir* dir, const std::string& rule_name, bool recurse )
{
	for ( auto II : this->rules )
		if (II.second->kdir_id == dir->kdir_id && II.second->rule_name == rule_name)
			return II.second;

	if ( recurse && dir->parent_dir_id )
		return find_rule( this->dirs[dir->parent_dir_id], rule_name );

	return NULL;
}

::k::m::krule*
::k::m::model::
add_rule( ::k::m::kdir* dir, const std::string& rule_name )
{
	krule* r = NULL;

	r = find_rule( dir, rule_name, false );
	if (r)
		return r;

	r = new krule;
	r->krule_id = next_rule_id();
	r->kdir_id = dir->kdir_id;
	//r->kenv_id = 0; // should go to dir env instead
	r->num_inp = -1;
	r->num_outp = -1;
	r->rule_name = rule_name;

	this->rules[ r->krule_id ] = r;

	return r;
}

::k::m::kvar*
::k::m::model::
find_var( ::k::m::kdir* dir, const std::string& var_name, bool recurse )
{
	for ( auto I : this->vars[ dir->kenv_id ] ) {
		if (I->var_name == var_name)
			return I;
	}

	if ( recurse && dir->parent_dir_id )
		return this->find_var( this->dirs[dir->parent_dir_id], var_name );

	return NULL;
}

::k::m::kvar*
::k::m::model::
add_var( ::k::m::kdir* dir, const std::string& var_name, const std::string& value )
{
	kvar *v;
	v = this->add_var( dir, var_name );
	if (v)
		v->var_value = value;
	return v;
}

::k::m::kvar*
::k::m::model::
add_var( ::k::m::kdir* dir, const std::string& var_name )
{
	kvar* v;
	v = find_var( dir, var_name, false );
	if (v)
		return v;
	v = new kvar;
	v->kenv_id = dir->kenv_id;
	v->var_name = var_name;

	this->vars[ dir->kenv_id ].push_back( v );

	return v;
}

::k::m::kobject*
::k::m::model::
add_object( ::k::m::kdir* dir, const std::string& name )
{
	kobject* o = NULL;

	if (name.empty())
		return o;

	if (name.find('/') == std::string::npos) {
		// local file
		o = new kobject;
		o->kobj_id = this->next_obj_id();
		o->kdir_id = dir->kdir_id;
		o->obj_name = name;
		this->objects[ o->kobj_id ] = o;
	}
	else {
		// non-local file: put under correct directory
		std::string dir_name;
		std::string file_name;
		kdir* od;
		if (name[0] == '/') {
			// abs file name
			dir_name = dirname( name );
			file_name = basename( name );
		}
		else {
			std::string tmp = normalize_path( dir->dir_name + "/" + name, NULL );
			dir_name = dirname( tmp );
			file_name = basename( tmp );
		}
		od = this->add_dir( dir_name );
		o = new kobject;
		o->kobj_id = this->next_obj_id();
		o->kdir_id = od->kdir_id;
		o->obj_name = file_name;
		this->objects[ o->kobj_id ] = o;
	}

	return o;
}

::k::m::ktask*
::k::m::model::
add_task( ::k::m::kdir* dir, const std::string& rule_name )
{
	ktask* t;
	t = new ktask;
	t->ktask_id = this->next_task_id();
	t->kdir_id = dir->kdir_id;
	t->krule_id = 0;
	t->kenv_id = 0;
	t->rule_name = rule_name;
	this->tasks[ t->ktask_id ] = t;
	return t;
}

::k::m::ktask*
::k::m::model::
add_task( ::k::m::kdir* dir, ::k::m::krule* rule )
{
	ktask* t;
	t = new ktask;
	t->ktask_id = this->next_task_id();
	t->kdir_id = dir->kdir_id;
	t->krule_id = rule->krule_id;
	t->kenv_id = 0;
	t->rule_name = rule->rule_name;
	this->tasks[ t->ktask_id ] = t;
	return t;
}

::k::m::ktask_obj*
::k::m::model::
task_add_object( ::k::m::ktask* task, ::k::m::kobject* obj,
	::k::m::role_type role )
{
	ktask_obj *ot;

	ot = new ktask_obj;
	ot->ktask_obj_id = this->next_task_obj_id();
	ot->ktask_id = task->ktask_id;
	ot->kobj_id = obj->kobj_id;
	ot->role = role;

	this->obj_task_rel[ ot->ktask_obj_id ] = ot;
	this->task_objs[ ot->ktask_id ].push_back( ot );

	return ot;
}

void
::k::m::model::
dump( std::ostream& o, ::k::m::kentity_type ent )
{
	switch(ent) {
	case KDIR:
		o << "dir_id\tparent\tenv_id\tname" << std::endl;
		for ( auto I : this->dirs ) {
			const kdir* d = I.second;
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
			const krule* r = I.second;
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
		o << "dir_id\tobj_id\tname\tdir_name" << std::endl;
		for ( auto I : this->objects ) {
			o << I.second->kdir_id << '\t'
				<< I.second->kobj_id << '\t'
				<< I.second->obj_name << '\t'
				<< this->dirs[ I.second->kdir_id ]->dir_name << std::endl;
		}
		break;
	case KTASK:
		o << "dir_id\ttask_id\tenv_id\trule_id\trule_name" << std::endl;
		for ( auto I : this->tasks ) {
			// ktask
			o << I.second->kdir_id << '\t'
				<< I.second->ktask_id << '\t'
				<< I.second->kenv_id << '\t'
				<< I.second->krule_id << '\t'
				<< I.second->rule_name << std::endl;
		}
		break;
	case KTASK_OBJ:
		o << "t_o_id\ttask_id\tobj_id\trole" << std::endl;
		for ( auto I : this->task_objs ) {
			for ( auto II : I.second ) {
				o << II->ktask_obj_id << '\t'
					<< II->ktask_id << '\t'
					<< II->kobj_id << '\t'
					<< II->role << std::endl;
			}
		}
		break;
	default:
		break;
	}
}


// --------------------------------------------------

namespace k {
namespace m {
// dependency graph for the model

struct dgraph {
	::k::m::model* m;
	std::map< int, int > toutp_task; // obj a is produced by task b
	std::map< int, std::set<int> > tprereq; // task a depends on { b }
	std::map< int, std::set<int> > tcontrib; // task a contributes to { b }
	void init_dgraph();
	void fill_outp_task();
	void fill_prereq();
	void fill_contrib();
	dgraph( ::k::m::model* mm );
};

} // m
} // k

::k::m::dgraph::
dgraph( ::k::m::model* mm )
{
	this->m = mm;
}

void
::k::m::dgraph::
init_dgraph()
{
	fill_outp_task();
	fill_prereq();
	fill_contrib();
}

void
::k::m::dgraph::
fill_outp_task()
{
	for ( auto I : this->m->task_objs ) {
		for ( auto II : I.second ) {
			ktask_obj* ko = II;
			if (ko->role != OBJ_OUTP) 
				continue;
			// assert( this->toutp_task.count( ko->kobj_id ) == 0 );
			this->toutp_task[ ko->kobj_id ] = ko->ktask_id;
		}
	}
}

void
::k::m::dgraph::
fill_prereq()
{
	for ( auto I : this->m->task_objs ) {
		for ( auto II : I.second ) {
			ktask_obj* ko = II;

			if (ko->role != OBJ_INP)
				continue;

			// kobj_id is an input for ko->task_id
			auto P = this->toutp_task.find( ko->kobj_id );
			if (P != this->toutp_task.end()) {
				// P.second is the task which produces the object
				// ko->task_id depends on P.second
				this->tprereq[ ko->ktask_id ].insert( P->second );
			}
			else {
				// object is not produced. must be a 'source'.
			}
		}
	}
}

void
::k::m::dgraph::
fill_contrib()
{
	// reverse the prereq
	for ( auto I : this->tprereq ) {
		int task_b = I.first;
		for ( auto II : I.second ) {
			int task_a = II;
			this->tcontrib[ task_a ].insert( task_b );
		}
	}
}

// --------------------------------------------------

namespace k {
namespace m {
// build graph for the model

struct ktask_state {
	int ktask_id;
	int state;
	int num_prereq;
	int num_contrib;
};

struct bgraph : public dgraph {
	std::map< int, ktask_state > tstates;
	void init_bgraph();
	bgraph( ::k::m::model* mm );
};

} // m
} // k


::k::m::bgraph::
bgraph( ::k::m::model* mm )
	: dgraph( mm )
{
}

void
::k::m::bgraph::
init_bgraph()
{
}


// --------------------------------------------------

#if defined(DATAK_SELFTEST)

int main()
{
	k::m::model* m;
	k::m::kdir* d;
	k::m::kvar* v;
	k::m::krule* r_cxx;
	k::m::krule* r_link;
	k::m::ktask* t;
	k::m::kobject* src;
	k::m::kobject* obj;
	k::m::kobject* exe;

	m = k::m::create( "." );
	d = m->find_dir( std::string() );
	v = m->add_var( d, "CXX", "c++" );
	v = m->add_var( d, "CXXFLAGS", "-O0 -std=c++11 -Wall -g" );

	r_cxx = m->add_rule( d, "compile.cxx" );
	r_cxx->command = "%{CXX} %{CXXFLAGS} %{input} -c -o %{output}";

	r_link = m->add_rule( d, "link.cxx" );
	r_link->command = "%{CXX} %{CXXFLAGS} %{input} -o %{output}";

	src = m->add_object( d, "hello_world.cc" );
	obj = m->add_object( d, "hello_world.o" );
	exe = m->add_object( d, "hello_world" );

	t = m->add_task( d, "compile.cxx" );
	m->task_add_object( t, src, k::m::OBJ_INP );
	m->task_add_object( t, obj, k::m::OBJ_OUTP );

	t = m->add_task( d, "link.cxx" );
	m->task_add_object( t, obj, k::m::OBJ_INP );
	m->task_add_object( t, exe, k::m::OBJ_OUTP );

	std::cout << "--- dirs --------" << std::endl; m->dump( std::cout, k::m::KDIR );
	std::cout << "--- envs --------" << std::endl; m->dump( std::cout, k::m::KENV );
	std::cout << "--- vars --------" << std::endl; m->dump( std::cout, k::m::KVAR );
	std::cout << "--- rules -------" << std::endl; m->dump( std::cout, k::m::KRULE );
	std::cout << "--- tasks -------" << std::endl; m->dump( std::cout, k::m::KTASK );
	std::cout << "--- task_objs ---" << std::endl; m->dump( std::cout, k::m::KTASK_OBJ );
	std::cout << "--- objs --------" << std::endl; m->dump( std::cout, k::m::KOBJECT );
	return 0;
}

#endif
