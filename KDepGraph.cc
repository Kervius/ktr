
#include "KDepGraph.hh"

namespace Ktr {


DepGraph::
DepGraph( Model* m )
: model(m)
{
}

void
DepGraph::
InitDepGraph()
{
	FillObjectProducer();
	FillPrereq();
	FillContrib();
}

void
DepGraph::
FillObjectProducer()
{
	for ( auto I : model->taskObjs->taskObjsIndex ) {
		for ( auto II : I.second ) {
			TaskObject* ko = II.second;
			if (ko->role == TaskObject::OUTPUT) {
				// assert( this->objectProducer.count( ko->kobj_id ) == 0 );
				this->objectProducer[ ko->obj_id ] = ko->task_id;
			}
		}
	}
}

void
DepGraph::
FillPrereq()
{
	for ( auto I : model->taskObjs->taskObjsIndex ) {
		for ( auto II : I.second ) {
			TaskObject* ko = II.second;
			if (ko->role == TaskObject::INPUT || ko->role == TaskObject::DEPNCY) {
				// kobj_id is an input for ko->task_id
				auto P = this->objectProducer.find( ko->obj_id );
				if (P != this->objectProducer.end()) {
					// P.second is the task which produces the object
					// ko->task_id depends on P.second
					this->taskPrereqs[ ko->task_id ].insert( P->second );
				}
				else {
					// object is not produced. must be a 'source'.
				}
			}
		}
	}
}

void
DepGraph::
FillContrib()
{
	// reverse the prereq
	for ( auto I : this->taskPrereqs ) {
		TaskIdType task_b = I.first;
		for ( auto II : I.second ) {
			TaskIdType task_a = II;
			this->taskContribTo[ task_a ].insert( task_b );
		}
	}
}

void
DepGraph::
DumpDepGraph( std::ostream& o )
{
	//std::map< int, int > objectProducer; // obj a is produced by task b
	//std::map< int, std::set<int> > taskPrereqs; // task a depends on { b }
	//std::map< int, std::set<int> > taskContribTo; // task a contributes to { b }

	o << "out_obj\ttask_id" << std::endl;
	for ( auto I : this->objectProducer )
		o << I.first << '\t' << I.second << std::endl;
	o << "task\treq task" << std::endl;
	for ( auto I : this->taskPrereqs )
		for ( auto II : I.second )
			o << I.first << '\t' << II << std::endl;
	o << "task\this->taskContribTo task" << std::endl;
	for ( auto I : this->taskContribTo )
		for ( auto II : I.second )
			o << I.first << '\t' << II << std::endl;
}

};
