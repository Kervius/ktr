#if 0
# ktor as a library

use Ktr qw/chext/;

my $ktr = Ktr->new( kfile => 'kfile.pl' );

$ktr->env->var( CXX => "c++" );
$ktr->env->var( CXXFLAGS => '-O0 -std=c++11 -Wall -g' );

$ktr->rule( 'compile.cxx', c => q(%{CXX} %{CXXFLAGS} %{input} -c -o %{output}) );
$ktr->rule( 'link.cxx', c=> q(%{CXX} %{CXXFLAGS} %{input} -o %{output}) )

my @srcs = qw/ktor.cc utilk.cc maink.cc filek.cc/;
#$ktr->sources( @srcs );

my @objs;
for my $cxx_file (@srcs) {
	push @objs,
		$ktr->do(
			'compile.cxx',
			i => $cxx_file,
			o => 'o/'.chext( $cxx_file, '.o' ),
			d => [qw/k.hh utilk.hh/],
		);
}

$ktr->do( 'link.cxx', i => [@objs], o => 'ktor' );

#$ktr->subdir( 'contrib' ) if -d 'contrib';
#$ktr->subdir( 'extras' ) if -d 'extras';
#$ktr->eof();

return $ktr->run([@ARGV]);

__END__
#endif

namespace ktr {
	struct ktr;
	struct kfile;
	struct krule;
	struct ktask;
	struct kqueue;



	struct kminutia {
		std::string name;
		kfile* kf;
		krule* kf;
		ktask* kt;
	};

	typedef std::vector<std::string> string_list;
	typedef std::vector<kminutia> minutia_list;

	struct ktr {
		static ktr* create( const std::string& root_dir );

		kfile* root_file();

		bool check_conf();

		kqueue* build_queue( const std::string& target = std::string() );
		kqueue* build_queue( const string_list& targets = string_list() );

		void flat_sources( minutia_list& list );
		void flat_dependencies( minutia_list& list );
		void flat_inputs( minutia_list& list );
		void flat_outputs( minutia_list& list );
	};

	struct kqueue {
		struct item {
			krule *kr;
			kfile *kf;
			const std::string expanded_command();
		};

		bool next_item( item& );

		void item_done( item&, int status );
	};

	struct krule {
		const std::string& name();

		const std::string& command();
		void set_command( const std::string& cmd );
		
		void input_conf( int );
		int input();

		void output_conf( int );
		int output();
	};

	struct ktask {
		const std::string& rule_name();

		void add_input( const std::string& inp );
		void add_output( const std::string& outp );
		void add_dep( const std::string& dep );

		const string_list& inputs();
		const string_list& outputs();
		const string_list& deps();
	};

	struct kfile {
		kfile *parent();

		const std::string& path();
		const std::string& abspath();

		kfile *subdir( const std::string& dir );
		const std::map<std::string, kfile*> subdirs();

		void var_set( const std::string& name, const std::string& value );
		void var_get( const std::string& name, const std::string& defl = std::string() );
		const std::map<std::string, std::string>& vars();

		krule* add_rule( const std::string& name );
		const std::vector<krule*> rules();

		ktask* add_task( const std::string& rule_name );
		const std::vector<ktask*> tasks();

		void add_default_target( const std::string& deft );
		const string_list default_targets();

		void finished();
	};

	namespace ddl {
		struct k {
			std::string root_dir;
			int root_kdir_id;
		};

		struct kdir {
			int kdir_id;
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
		struct kobj_to_task {
			int kobj_id;
			int ktask_id;
			enum { INP, OUTP, DEP } type;
		};

		struct ddl {
			std::vector<kdir*> dirs;
			std::vector<kvar*> vars;
			std::vector<krule*> rules;
			std::vector<kobject*> objects;
			std::vector<ktask*> tasks;
			std::vector<kobj_to_task*> obj_to_task_rel;
		};
	};
};

/* - - - - - */

int main()
{
	ktr::ktr* k = new ktr::ktr::create(".");
	ktr::kfile* kf;
	ktr::krule* kr;
	ktr::ktask* kt;

	kf = k->root_file();

	kf->set_var( "CXX", "c++" );
	kf->set_var( "CXXFLAGS", "-O0 -std=c++11 -Wall -g" );

	kr = kf->add_rule( "compile.cxx" );
	kr->set_command( "%{CXX} %{CXXFLAGS} %{input} -c -o %{output}" );

	kr = kf->add_rule( "link.cxx" );
	kr->set_command( "%{CXX} %{CXXFLAGS} %{input} -o %{output}" );

	string_list srcs = std::vector{ "ktor.cc", "utilk.cc", "maink.cc", "filek.cc" };
	//kf->sources( @srcs );

	string_list hdrs = std::vector{ "h.hh", "utilk.hh" };
	string_list objs;

	for (auto cxx_file : srcs) {
		std::string obj = chext( cxx_file, ".o" );

		kt = kf->add_task( "compile.cxx" );
		kt->add_input( src );
		kt->add_output( obj );
		for (auto hdr : hdrs)
			kt->add_dep( hdr );
		objs.push_back( obj );
	}

	kt = kf->add_task( "link.cxx" );
	kt->add_output( "ktor" );
	for (auto obj : objs)
		kt->add_input( obj );

	//kf->subdir( "contrib" );
	//kf->subdir( "extras" );

	kf->finished();

	ktr::kqueue* kq = k->build_queue( "ktor" );
	ktr::kqueue::item kqi;

	while (kq->next_item(kqi)) {
		if (system( kqi.expanded_command().c_str ) == 0)
			kq->item_done( kqi, 0 );
		else
			return 1;
	}

	return 0;
}

