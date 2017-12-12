
#include "KModel.hh"

namespace Ktr {

/// c'tor
ObjectTable::
ObjectTable( Model *m )
: model(m)
{
}
ObjIdType
ObjectTable::
NextObjId() const
{
	return ObjIdType( this->objects.empty() ? 1 : this->objects.rbegin()->first + 1 );
}

/// add object
Object*
ObjectTable::
AddObject( Dir* dir, const std::string& name_ )
{
	Object* o = nullptr;

	if (name_.empty())
		return o;

	std::string name = model->envs->ExpandVarString( dir->env_id, name_ );

	if (name.find('/') == std::string::npos) {
		// local file
		o = new Object;
		o->obj_id = this->NextObjId();
		o->dir_id = dir->dir_id;
		o->obj_name = name;
		this->objects[ o->obj_id ] = o;
	}
	else {
		// non-local file: put under correct directory
		std::string dir_name;
		std::string file_name;
		Dir* od;
		if (name[0] == '/') {
			// abs file name
			dir_name = Utils::DirName( name );
			file_name = Utils::BaseName( name );
		}
		else {
			std::string tmp0;
			if (not dir->dir_name.empty()) {
				tmp0 += dir->dir_name;
				tmp0 += "/";
			}
			tmp0 += name;
			std::string tmp = Utils::NormalizePath( tmp0, NULL );
			dir_name = Utils::DirName( tmp );
			file_name = Utils::BaseName( tmp );
		}
		od = model->dirs->AddDir( dir_name );
		o = new Object;
		o->obj_id = this->NextObjId();
		o->dir_id = od->dir_id;
		o->obj_name = file_name;
		this->objects[ o->obj_id ] = o;
	}

	return o;
}

/// find object
Object*
ObjectTable::
LookUpObject( ObjIdType obj_id )
{
	auto I = this->objects.find( obj_id );
	return (I != this->objects.end()) ? I->second : nullptr;
}


/// find object in this dir
Object*
ObjectTable::
FindLocalObject( Dir* dir, const std::string &object_name )
{
	if (dir && not object_name.empty()) {
		for (auto X : this->objects) {
			Object* o = X.second;
			if (o->dir_id == dir->dir_id && o->obj_name == object_name) {
				return o;
			}
		}
	}
	return nullptr;
}

/// find object relative to this dir
Object*
ObjectTable::
FindObject( Dir* dir, const std::string &object_name )
{
	if (object_name.find("/") == std::string::npos) {
		//fprintf( stderr, "xxx +++ %p: %s\n", dir, object_name.c_str() );
		return FindLocalObject( dir, object_name );
	}
	else {
		std::string odir_name = Utils::DirName( object_name );
		std::string base_name = Utils::BaseName( object_name );
		//fprintf( stderr, "xxx --- %p: %s / %s\n", dir, odir_name.c_str(), base_name.c_str() );
		Dir* odir = model->dirs->FindRelDir( dir, odir_name );
		//fprintf( stderr, "xxx ~~~ %p\n", odir );
		return FindLocalObject( odir, base_name );
	}
	return nullptr;
}

/// get file name
std::string
ObjectTable::
GetObjectName( ObjIdType obj_id )
{
	Object *obj = this->LookUpObject( obj_id );
	return this->GetObjectName( obj );
}

/// get file name
std::string
ObjectTable::
GetObjectName( Object *obj )
{
	std::string full_obj_name;
	Dir *dir = obj ? model->dirs->LookUpDir( obj->dir_id ) : nullptr;
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

void
ObjectTable::
Dump( std::ostream& o )
{
	o << "dir_id\tobj_id\tsubdir\tname" << std::endl;
	for ( auto I : this->objects ) {
		o << I.second->dir_id << 
			'\t' << I.second->obj_id <<
			'\t' << model->dirs->dirs[ I.second->dir_id ]->dir_name << 
			'\t' << I.second->obj_name <<
			std::endl;
	}
}

}
