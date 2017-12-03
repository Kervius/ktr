#ifndef KTR_RULE_HH
#define KTR_RULE_HH

#include "KTypes.hh"
#include "KDir.hh"

namespace Ktr {
/// @brief rule entity, a template of the command for the task entities
struct Rule {
	/// unique id of the rule, the primary key
	RuleIdType rule_id = InvalidRule;	
	/// id of the directory where the rule was defined
	DirIdType dir_id;	
	/// number of inputs the rule has
	int num_inp;	
	/// number of outputs the rule has
	int num_outp;	
	/// the template string of the comment to execute
	std::string command;
	/// the public name of the rule, used for late binding with the tasks
	std::string rule_name;

	Rule()
	: rule_id(InvalidRule)
	, dir_id(InvalidDir)
	, num_inp(0)
	, num_outp(0)
	{
	}
};

struct RuleTable {
	/// parent model
	Model *model;
	/// map rule id to rule object
	std::map<RuleIdType, Rule*>   rules;

	/// map rule to directories
	std::map<DirIdType, std::map<std::string, Rule*> >   tree;

	/// c'tor
	RuleTable( Model *m );

	/// gen unique rule id
	RuleIdType
	NextRuleId() const;

	/// get rule in the dir
	Rule*
	LookUpRule( DirIdType dir_id, const std::string& rule_name );

	/// get rule by id
	Rule*
	LookUpRule( RuleIdType rule_id );

	/// find rule recursively, starting from the dir
	Rule*
	FindRule( DirIdType dir_id, const std::string& rule_name );

	/// add or find rule
	Rule*
	AddRule( Dir* dir, const std::string& rule_name );

	/// add or find rule
	Rule*
	AddRule( DirIdType dir_id, const std::string& rule_name );

	/// dump
	void
	Dump( std::ostream& o );
};
}

#endif // KTR_RULE_HH
