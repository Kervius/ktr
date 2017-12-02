#ifndef KTR_ATTRIBUTE_HH
#define KTR_ATTRIBUTE_HH

#include "KTypes.hh"

namespace Ktr {

/// @brief attribute entity
/// kentity_id + type point to the entity the attribute is attached to
struct Attribute {
	/// unique id of the attribute
	int attr_id = 0;	
	/// id of the entity
	int entity_id;	
	/// type of the entity
	EntityType type;
	/// name of the attribute
	std::string name;
	/// optional value of the attribute
	std::string value;
};

struct AttributeTable {
	/// attr_id -> attribute object
	std::map<int, Attribute*>         attrs;
};

}

#endif // KTR_ATTRIBUTE_HH
