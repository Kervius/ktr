#ifndef KTR_ENV_HH
#define KTR_ENV_HH

#include "KTypes.hh"

#include "KVar.hh"

namespace Ktr {

/// @brief environment entity, a set of variables
struct Env {
	/// id of the environment, the primary key
	EnvIdType env_id;

	/// id of the parent environment
	EnvIdType parent_env_id;

	/// list of vars (as they appear)
	std::map<std::string, Var*> vars;
};

struct EnvTable {
	/// parent model
	Model *model;
	/// map env id to env object
	std::map<EnvIdType, Env*> envs;

	/// c'tor
	EnvTable( Model *km );

	/// generate unique env id
	EnvIdType
	NextEnvId() const;

	/// create new env
	Env*
	NewEnv( EnvIdType parent_env_id );

	/// create new env
	Env*
	NewEnv( Env* parent_env );

	/// add var by id, set value
	Var*
	AddVar( EnvIdType env_id, const std::string& var_name, 
		const std::string& value );

	/// add var by ref, set value
	Var*
	AddVar( Env *env, const std::string& var_name, 
		const std::string& value );

	/// map id to env
	Env*
	LookUpEnv( EnvIdType env_id );

	/// look-up var by name
	Var*
	LookUpVar( Env *env, const std::string& var_name );

	/// find var
	Var*
	FindVar( EnvIdType start_env_id, const std::string& var_name );

	/// find var
	Var*
	FindVar( Env *env, const std::string& var_name );

	/// get expanded variable value
	std::string
	ExpandVarString( EnvIdType env_id, const std::string &str );
};

}

#endif // KTR_ENV_HH
