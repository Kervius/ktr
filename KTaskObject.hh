#ifndef KTR_TASK_OBJECT_HH
#define KTR_TASK_OBJECT_HH

#include "KTypes.hh"

namespace Ktr {
/// @brief relation entity, linking a task with input/output/dependent objects
struct TaskObject {
	/// defines roles of the objects
	enum Role {		
		/// input object (aka input file(s))
		INPUT,
		/// output object (aka output file(s))
		OUTPUT,
		/// dependent object (extra dependencies)
		DEPNCY,
	};
	/// unique id of the entity
	TaskObjIdType task_obj_id = InvalidTaskObj;
	/// task
	TaskIdType task_id;			
	/// object id
	ObjIdType obj_id;			
	/// type of the object
	Role role;
	/// original name of the object. the Object would contain
	/// the normalized file name, which not necessarily matches
	/// the name as specified by the user when defining the task.
	/// the model in memory requires normalized names, while for command
	/// execution, the original name is required
	std::string obj_orig_name;
};

struct TaskObjectTable {
	/// parent model
	Model *model;

	/// map task-obj id to task-obj relation object
	std::map<TaskObjIdType, TaskObject*> taskObjs;

	/// map task id to list of task-obj objects
	std::map<TaskIdType, std::map<ObjIdType, TaskObject*> > taskObjsIndex;

	/// c'tor
	TaskObjectTable(Model *m);

        /// next id
	TaskObjIdType
	NextTaskObjId() const;

        /// add task-object, using object
        TaskObject*
        AddTaskObject( Task* task, Object* obj, TaskObject::Role role,
                const std::string& obj_orig_name = std::string() );
                
        /// add task-object, using another task-object
        TaskObject*
        AddTaskObject( Task* task, TaskObject* ot, TaskObject::Role role );
        
        /// find task-object
        TaskObject*
        FindTaskObj( Task* t, Object* o );
                
        /// find task-object
        TaskObject*
        FindTaskObj( TaskIdType task_id, ObjIdType obj_id );
};

}

#endif // KTR_TASK_OBJECT_HH
