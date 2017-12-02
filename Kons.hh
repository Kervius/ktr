#ifndef KTR_KONS_HH
#define KTR_KONS_HH

#include "KModel.hh"
#include "KDepGraph.hh"
#include "KBuildState.hh"

namespace Ktr {

/// @brief Kons, an adage to 'cons' build system
struct Kons {
	Model *model;
	DepGraph *dg;
	BuildState *bs;

	std::list<std::string> targetList;
public:
	Kons( Model *m );
	~Kons();
public:
	void Reset();
	void AddTarget( const std::string &target );

	bool Test();
	bool Build();
	bool Query();
	bool Clean();

	bool ProcTargetList();
};

}

#endif // KTR_KONS_HH
