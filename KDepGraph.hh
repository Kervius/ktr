#ifndef KTR_DEP_GRAPH_HH
#define KTR_DEP_GRAPH_HH

#include "KModel.hh"

namespace Ktr {

/// @brief dependency graph for the model
struct DepGraph {
	/// model the dep graph is attached to
	Model* model;

	/// obj a is produced by task b
	std::map< ObjIdType, TaskIdType > objectProducer;

	/// task a depends on { b }
	std::map< TaskIdType, std::set<TaskIdType> > taskPrereqs;

	/// task a contributes to { b }
	std::map< TaskIdType, std::set<TaskIdType> > taskContribTo;

	/// @brief initialize the dep graph
	/// call the fill methods to populate the dep graph from the Model
	void InitDepGraph();

	/// fill object producers map
	void FillObjectProducer();

	/// fill task prerequisites map
	void FillPrereq();

	/// fill task contributions map
	void FillContrib();

	void DumpDepGraph( std::ostream& o );

	/// initialize the instance of the dep graph object
	DepGraph( Model* m );
};

}

#endif // KTR_DEP_GRAPH_HH
