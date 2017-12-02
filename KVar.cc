
#include "KModel.hh"

namespace Ktr {

#if 0
VarTable::VarTable( Model *km )
: model(km)
{
}

Var*
VarTable::
AddVar( EnvIdType env_id, const std::string& var_name )
{
	return nullptr;
}

Var*
VarTable::
AddVar( EnvIdType env_id, const std::string& var_name, 
	const std::string& value )
{
	Var *v;

	v = this->AddVar( env_id, var_name );
	if (v)
		v->var_value = value;

	return v;
}
#endif

}
