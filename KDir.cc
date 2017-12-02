
#include "KModel.hh"

namespace Ktr {

DirTable::
DirTable( Model* km_ )
: model(km_)
{
}

DirIdType
DirTable::
NextDirId() const
{
	return DirIdType( this->dirs.empty() ? 1 : this->dirs.rbegin()->first + 1 );
}

Dir*
DirTable::
NewDir( DirIdType dir_id, DirIdType parent_dir_id, const std::string &dir_name, EnvIdType env_id )
{
	Dir *d;
	d = new Dir;
	d->dir_id = (dir_id == InvalidDir) ? NextDirId() : dir_id;
	d->parent_dir_id = parent_dir_id;
	d->env_id = env_id;
	d->dir_name = dir_name;

	this->dirs[ d->dir_id ] = d;

	return d;
}

Dir*
DirTable::
AddDir( const std::string& dir )
{
	Dir *d = NULL;

	// to do: need context (parent dir) to expand the dir name.

	d = this->FindDir( dir );
	if (d)
		return d;

	if (dir.empty()) {
		// root dir
		if (model->globalConf->root_dir_id == 0) {
			// assert( this->dirs.empty() );
			//d = NewDir( RootDirId, InvalidDir, std::string() );
			d = NewDir( RootDirId, InvalidDir, dir );
			model->globalConf->root_dir_id = RootDirId;
		}
	}
	else {
		if (dir[0] == '/') {
			// parent to root
			d = NewDir( InvalidDir, RootDirId, dir );
		}
		else {
			Dir* parent;
			if (dir.find('/') != std::string::npos) {
				parent = this->AddDir( Utils::DirName( dir ) ); // recurse
			}
			else {
				// assert( this->dirs.count(model->globalConf->root_dir_id) );
				parent = this->dirs[ model->globalConf->root_dir_id ];
			}
			d = NewDir( InvalidDir, parent->dir_id, dir );
		}
	}

	// automatically add environment.
	if (d->env_id == InvalidEnv) {
		Env *env;
		EnvIdType parent_env_id = InvalidEnv;

		if (d->parent_dir_id > 0) {
			parent_env_id = this->dirs[ d->parent_dir_id ]->env_id;
		}
		env = model->envs->NewEnv( parent_env_id );
		d->env_id = env->env_id;
	}

	return d;
}

Dir*
DirTable::
FindDir( const std::string& dir )
{
	if (dir.empty() || dir == model->globalConf->root_dir) {
		if (model->globalConf->root_dir_id)
			return this->dirs[ model->globalConf->root_dir_id ];
	}
	else {
		for ( auto II : this->dirs )
			if (II.second->dir_name == dir)
				return II.second;
	}
	return nullptr;
}

Dir*
DirTable::
LookUpDir( DirIdType dir_id )
{
	auto I = this->dirs.find( dir_id );
	return I != this->dirs.end() ? I->second : nullptr;
}

DirIdType
DirTable::
GetParentDir( DirIdType dir_id )
{
	Dir *d = LookUpDir( dir_id );
	if (d)
		return d->parent_dir_id;
	return InvalidDir;
}

}
