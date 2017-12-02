#ifndef KTR_VAR_HH
#define KTR_VAR_HH

#include "KTypes.hh"

namespace Ktr {
/// @brief variable entity
struct Var {
	///// id of the environment the variable belongs to
	//EnvIdType env_id;
	/// variable name
	std::string var_name;
	/// variable value
	std::string var_value;
};

#if 0
struct VarTable {
	/// parent model
	Model *model;
	/// map env id to list of vars (as they appear)
	std::map<EnvIdType, std::vector<Var*>> vars;

	VarTable( Model *m );

	/// add or find var
	Var*
	AddVar( EnvIdType env_id, const std::string& var_name );

	/// add or find var, set value
	Var*
	AddVar( EnvIdType env_id, const std::string& var_name, 
		const std::string& value );
};
#endif

}


#endif // KTR_VAR_HH
