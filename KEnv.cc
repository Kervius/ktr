
#include "KEnv.hh"

namespace Ktr {

EnvTable::
EnvTable( Model *km )
: model(km)
{
}

EnvIdType
EnvTable::
NextEnvId() const
{
	return EnvIdType( this->envs.empty() ? 1 : this->envs.rbegin()->first + 1 );
}

Env*
EnvTable::
NewEnv( EnvIdType parent_env_id )
{
	Env* env;
	env = new Env;
	env->env_id = this->NextEnvId();
	env->parent_env_id = parent_env_id;
	this->envs[ env->env_id ] = env;
	return env;
}

Env*
EnvTable::
NewEnv( Env* parent_env )
{
	return NewEnv( parent_env->env_id );
}

Env*
EnvTable::
LookUpEnv( EnvIdType env_id )
{
	if (env_id != InvalidEnv) {
		std::map<EnvIdType, Env*>::iterator ii;
		ii = this->envs.find( env_id );
		if (ii != this->envs.end())
			return ii->second;
	}
	return nullptr;
}

Var*
EnvTable::
LookUpVar( Env *env, const std::string& var_name )
{
	if (env) {
		std::map<std::string, Var*>::iterator ii;
		ii = env->vars.find( var_name );
		if (ii != env->vars.end())
			return ii->second;
	}
	return nullptr;
}

Var*
EnvTable::
AddVar( EnvIdType env_id, const std::string& var_name, 
	const std::string& value )
{
	Var *var = nullptr;
	Env *env;
	env = this->LookUpEnv( env_id );
	if (env)
		var = this->AddVar( env, var_name, value );
	return var;
}

Var*
EnvTable::
AddVar( Env *env, const std::string& var_name, 
	const std::string& value )
{
	Var *var = nullptr;
	if (env) {
		var = this->LookUpVar( env, var_name );
		if (var) {
			var->var_value = value;
		}
		else {
			var = new Var;
			var->var_name = var_name;
			var->var_value = value;
			env->vars[ var_name ] = var;
		}
	}
	return var;
}

/// find var
Var*
EnvTable::
FindVar( EnvIdType start_env_id, const std::string& var_name )
{
	return this->FindVar( LookUpEnv(start_env_id), var_name );
}

Var*
EnvTable::
FindVar( Env *env, const std::string& var_name )
{
	Var *var = nullptr;
	while (env) {
		var = LookUpVar( env, var_name );
		if (var)
			break;
		env = LookUpEnv( env->parent_env_id );
	}
	return var;
}

/// get expanded variable value
std::string
EnvTable::
ExpandVarString( EnvIdType env_id, const std::string &str )
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
				Var* v = this->FindVar( env_id, var_name );
				std::string val = v->var_value;
				std::string expanded = this->ExpandVarString( env_id, val );
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

}
