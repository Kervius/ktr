
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


}
