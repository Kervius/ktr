
#include "Kons.hh"

namespace Ktr {

Kons::
Kons( Model *m )
{
	this->model = m;
	this->dg = new DepGraph( m );
	this->bs = new BuildState( m, dg );
}

Kons::
~Kons()
{
	delete dg;
	delete bs;
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
