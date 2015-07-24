
#include <string>
#include <map>
#include <vector>

namespace K {
	namespace model {
		struct k {
			std::string root_dir;
			int root_kdir_id;
		};

		struct kdir {
			int kdir_id;
			int parent_dir_id;
			std::string dir_name;
		};

		struct kvar {
			int kdir_id;
			std::string var_name;
			std::string var_value;
		};

		struct krule {
			int krule_id;
			std::string command;
			int num_inp;
			int num_outp;
		};

		struct kobject {
			int kobj_id;
			std::string obj_name;
			int kdir_id;
		};

		struct ktask {
			int ktask_id;
			int krule_id;
		};

		struct krel_obj_task {
			enum rel_type {
				INP, OUTP, DEP
			};
			int krel_obj_task_id;
			int kobj_id;
			int ktask_id;
			rel_type rel;
		};

		enum kentity_type {
			KDIR,
			KRULE,
			KOBJECT,
			KTASK,
			KREL_OBJ_TASK,
		};
		struct kattr {
			int kattr_id;
			int kentity_id;
			kentity_type type;
			std::string value;
		};

		struct data {
			k* k;
			std::map<int,kdir*>  dirs;
			std::map<int,vector<kvar*>>  vars;
			std::map<int,krule*>  rules;
			std::map<int,kobject*>  objects;
			std::map<int,ktask*>  tasks;
			std::map<int,krel_obj_task*> obj_task_rel;
			std::map<int,kattr*> attrs;
		};
	}
}
