
#include "Kons.hh"

namespace Ktr {

Kons::
Kons( Model *m )
{
	this->model = m;
	this->dg = new DepGraph( m );
	this->bs = new BuildState( m, dg );
	Init();
}

Kons::
~Kons()
{
	delete dg;
	delete bs;
}

void
Kons::
Init()
{
	dg->InitDepGraph();
}

void
Kons::
Reset()
{
	targetList.clear();
	bs->Reset();
}

void
Kons::
AddTarget( const std::string &target )
{
	targetList.push_back( target );
}

bool
Kons::
Test()
{
	Object *o;
	for (auto targ : targetList) {
		o = model->objects->FindObject( model->GetRootDirObj(), targ );
		if (o) {
			bs->FillStatesForObj( o->obj_id );
			fprintf( stderr, "added target [%s]\n", targ.c_str() );
		}
		else {
			fprintf( stderr, "can't find object for target [%s]\n", targ.c_str() );
		}
	}
	return true;
}

bool
Kons::
Build()
{
	return true;
}

bool
Kons::
Query()
{
	return true;
}

bool
Kons::
Clean()
{
	return true;
}

bool
Kons::
ProcTargetList()
{
	return true;
}

}
