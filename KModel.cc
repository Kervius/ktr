
#include "KModel.hh"

namespace Ktr
{

Model::
Model( const std::string& root_dir )
: globalConf(new GlobalData(root_dir))
, dirs(new DirTable(this))
, envs(new EnvTable(this))
, rules(new RuleTable(this))
, objects(new ObjectTable(this))
, tasks(new TaskTable(this))
, taskObjs(new TaskObjectTable(this))
, attrs(new AttributeTable)
{
	// init root dir;
	Dir *rd = dirs->AddDir( std::string() );
	rd->dir_name = root_dir;
	envs->AddVar( rd->env_id, "KTR_ROOT", root_dir );
}

Model::
~Model()
{
	delete globalConf;
	delete dirs;
	delete envs;
	delete rules;
	delete objects;
	delete tasks;
	delete taskObjs;
	delete attrs;
}

const std::string& Model::
GetRootDir()
{
	return globalConf->root_dir;
}

Dir* Model::
GetRootDirObj()
{
	return dirs->dirs[ globalConf->root_dir_id ];
}

void Model::
Dump( std::ostream& o, EntityType ent )
{
	switch(ent) {
	case KDIR:
		/*o << "dir_id\tparent\tenv_id\tname" << std::endl;
		for ( auto I : this->dirs ) {
			const KDir* d = I.second;
			o << d->kdir_id << '\t';
			o << d->parent_dir_id << '\t';
			o << d->kenv_id << '\t';
			o << d->dir_name << std::endl;
		}*/
		this->dirs->Dump( o );
		break;
	case KENV:
		/*o << "env_id\tparent_env_id" << std::endl;
		for ( auto I : this->envs )
			o << I.second->kenv_id << '\t'
				<< I.second->parent_env_id << std::endl;
		*/
		this->envs->Dump( o );
		break;
	case KRULE:
		/*
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
		}*/
		this->rules->Dump( o );
		break;
	case KOBJECT:
		/*o << "dir_id\tobj_id\tsubdir\tname" << std::endl;
		for ( auto I : this->objects ) {
			o << I.second->kdir_id << 
				'\t' << I.second->kobj_id <<
				'\t' << this->dirs[ I.second->kdir_id ]->dir_name << 
				'\t' << I.second->obj_name <<
				std::endl;
		}*/
		this->objects->Dump( o );
		break;
	case KTASK:
		/*o << "dir_id\ttask_id\tenv_id\trule_id\trule_name" << std::endl;
		for ( auto I : this->tasks ) {
			// KTask
			o << I.second->kdir_id << '\t'
				<< I.second->ktask_id << '\t'
				<< I.second->kenv_id << '\t'
				<< I.second->krule_id << '\t'
				<< I.second->rule_name << std::endl;
		}*/
		this->tasks->Dump( o );
		break;
	case KTASK_OBJ:
		/*o << "t_o_id\ttask_id\tobj_id\trole\tobj_name" << std::endl;
		for ( auto I : this->taskObjs ) {
			for ( auto II : I.second ) {
				o << II->ktask_obj_id <<
					'\t' << II->ktask_id <<
					'\t' << II->kobj_id <<
					'\t' << ( II->role == KTaskObject::INPUT ? "inp" :
						II->role == KTaskObject::OUTPUT ? "outp" : "dep" ) <<
					'\t' << this->GetObjectName( II->kobj_id ) <<
					std::endl;
			}
		}*/
		this->taskObjs->Dump( o );
		break;
	default:
		break;
	}
}

} // ns Ktr
