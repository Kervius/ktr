
#include "KModel.hh"

namespace Ktr {

RuleTable::
RuleTable( Model *m )
: model(m)
{
}

RuleIdType
RuleTable::
NextRuleId() const
{
	return RuleIdType( this->rules.empty() ? 1 : this->rules.rbegin()->first + 1 );
}

Rule*
RuleTable::
AddRule( Dir* dir, const std::string& rule_name )
{
	return AddRule( dir->dir_id, rule_name );
}

Rule*
RuleTable::
AddRule( DirIdType dir_id, const std::string& rule_name )
{
	if (dir_id == InvalidDir)
		return nullptr;

	Rule *r;
	r = LookUpRule( dir_id, rule_name );
	if (r)
		return r;

	r = new Rule;
	r->rule_id = NextRuleId();
	r->dir_id = dir_id;
	r->num_inp = -1;
	r->num_outp = -1;
	r->rule_name = rule_name;

	this->rules[ r->rule_id ] = r;
	this->tree[ dir_id ][ r->rule_name ] = r;

	return r;
}

Rule*
RuleTable::
LookUpRule( DirIdType dir_id, const std::string& rule_name )
{
	Rule *r = nullptr;
	do {
		auto II = this->tree.find( dir_id );
		if (II == this->tree.end())
			break;

		auto II2 = II->second.find( rule_name );
		if (II2 == II->second.end())
			break;

		r = II2->second;
	} while (0);
	return r;
}

Rule*
RuleTable::
LookUpRule( RuleIdType rule_id )
{
	auto I = this->rules.find( rule_id );
	return I != this->rules.end() ? I->second : nullptr;
}

Rule*
RuleTable::
FindRule( DirIdType dir_id, const std::string& rule_name )
{
	while (dir_id) {
		Rule *r = this->LookUpRule( dir_id, rule_name );
		if (r)
			return r;
		dir_id = model->dirs->GetParentDir( dir_id );
	}
	return nullptr;
}

}
