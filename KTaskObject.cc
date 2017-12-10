
#include "KModel.hh"

namespace Ktr {

TaskObjectTable::
TaskObjectTable(Model *m)
: model(m)
{
}

TaskObjIdType
TaskObjectTable::
NextTaskObjId() const
{
	return TaskObjIdType( this->taskObjs.empty() ? 1 : this->taskObjs.rbegin()->first + 1 );
}

TaskObject*
TaskObjectTable::
AddTaskObject( Task* task, Object* obj, TaskObject::Role role,
	const std::string& obj_orig_name )
{
	TaskObject *ot;

	ot = new TaskObject;
	ot->task_obj_id = this->NextTaskObjId();
	ot->task_id = task->task_id;
	ot->obj_id = obj->obj_id;
	ot->role = role;
	ot->obj_orig_name = obj_orig_name.empty() ? obj->obj_name : obj_orig_name ;

	this->taskObjs[ ot->task_obj_id ] = ot;
	this->taskObjsIndex[ ot->task_id ][ ot->obj_id ] = ot;

	return ot;

}
	
TaskObject*
TaskObjectTable::
AddTaskObject( Task* task, TaskObject* ot, TaskObject::Role role )
{
	return AddTaskObject
		(
			task,
			model->objects->LookUpObject( ot->obj_id ),
			role,
			ot->obj_orig_name
		);
}

TaskObject*
TaskObjectTable::
AddTaskObject( Dir* d, Task* t, const std::string& obj, TaskObject::Role role )
{
	if (d == nullptr)
		d = model->dirs->LookUpDir( t->dir_id );

	Object* o = model->objects->FindObject( d, obj );
	if (!o)
		o = model->objects->AddObject( d, obj );

	if (t && d && o) {
		return AddTaskObject( t, o, role, obj );
	}

	return nullptr;
}

TaskObject*
TaskObjectTable::
FindTaskObj( Task* t, Object* o )
{
	return FindTaskObj( t->task_id, o->obj_id );
}
	
TaskObject*
TaskObjectTable::
FindTaskObj( TaskIdType task_id, ObjIdType obj_id )
{
	auto II = this->taskObjsIndex.find( task_id );
	if (II != this->taskObjsIndex.end())
	{
		auto II2 = II->second.find( obj_id );
		if (II2 != II->second.end())
			return II2->second;
	}
	return nullptr;
}

void
TaskObjectTable::
Dump( std::ostream& o )
{
	o << "t_o_id\ttask_id\tobj_id\trole\tobj_name" << std::endl;
	for ( auto I : this->taskObjsIndex ) {
		for ( auto II : I.second ) {
			o << II.second->task_obj_id <<
				'\t' << II.second->task_id <<
				'\t' << II.second->obj_id <<
				'\t' << ( II.second->role == TaskObject::INPUT ? "inp" :
					II.second->role == TaskObject::OUTPUT ? "outp" : "dep" ) <<
				'\t' << model->objects->GetObjectName( II.second->obj_id ) <<
				std::endl;
		}
	}
}

}
