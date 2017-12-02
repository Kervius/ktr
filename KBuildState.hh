#ifndef KTR_BUILD_STATE_HH
#define KTR_BUILD_STATE_HH

#include "KModel.hh"
#include "KDepGraph.hh"

namespace Ktr {

/// build state for the model and the dep graph
enum TaskBuildStateType {
	/// initial task state
	TASK_PENDING,
	/// finished
	TASK_FINISHED,
	/// running
	TASK_RUNNING,
};

/// @brief build state of the task
struct TaskBuildState {
	/// task id
	TaskIdType task_id;
	/// task state
	TaskBuildStateType state;
	/// number of prerequisites yet un-finished
	int num_prereq;
};

/// @brief build state
struct BuildState
{
	/// model
	Model* model;
	/// dep graph
	DepGraph* dg;

	/// @brief task_id to task state map
	std::map< TaskIdType, TaskBuildState > taskStates;

	/// @brief fill taskStates for the given task
	/// recursively, using the prerequisites map from the dep graph, 
	/// fill taskStates with initial information
	void FillStatesForTask( TaskIdType task_id );

	/// @brief fill taskStates for the given object
	void FillStatesForObj( ObjIdType obj_id );

	/// count to set of tasks map
	std::map< int, std::set<TaskIdType> > buildQueue;

	/// @brief using the taskStates, fill build queue
	/// should be called after the FillStatesForTask()/FillStatesForTask() calls
	void FillBuildQueue();

	/// @brief update state of the given task
	void UpdateBuildState( TaskIdType task_id, TaskBuildStateType new_state );

	/// @brief decrement the number of un-finished prerequisites of the given task
	/// when the counter reaches zero, that means all the prerequisites were built
	/// and the task can be ran now.
	void DecrTaskPrereq( TaskIdType task_id );

	/// @brief when the given task finishes, notify the tasks for which it is a prerequisite
	void NotifyTaskContrib( TaskIdType task_id );

	/// @brief generate the execute commmand string for the given task
	bool GetTaskCmd( TaskIdType task_id, std::string& cmd, bool expanded = true );
	/// @brief generate the directory name where the given task should be run
	void GetTaskDir( TaskIdType task_id, std::string& dir, bool relative = true );

	/// @brief initialize task environment, if not yet
	void InitTaskEnv( Task* t );

	/// @brief initialize task environment, if not yet
	void InitTaskEnv( TaskIdType task_id );

	/// @brief dump build state in textual form
	void DumpBuildState( std::ostream& o );

	/// @brief reset build state to the original
	void Reset();

	/// @brief initialize the instance
	BuildState( Model* m, DepGraph* dgg );
};

}

#endif // KTR_BUILD_STATE_HH
