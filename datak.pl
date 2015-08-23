#!/usr/bin/perl
use strict;
use warnings;
use Data::Dumper;


 #    #   #####  #####
 #   #      #    #    #
 ####       #    #    #
 #  #       #    #####
 #   #      #    #   #
 #    #     #    #    #

##########################


{package ktr::k;
sub new
{
	my ($class) = @_;
	return bless { 
		root_dir => undef, 
		root_kdir => undef 
	}, $class;
}
};

{package ktr::kdir;
sub new
{
	my ($class) = @_;
	return bless {
		kdir_id => 0,
		parent_dir_id => 0,
		kenv_id => 0,
		dir_name => undef,
	}, $class;
}
};

{package ktr::kvar;
sub new
{
	my ($class) = @_;
	return bless {
		kenv_id => 0,
		var_name => undef,
		var_value => undef,
	}, $class;
}
};

{package ktr::kenv;
sub new
{
	my ($class) = @_;
	return bless {
		kenv_id => 0,
		parent_env_id => 0,
	}, $class;
}
};

{package ktr::krule;
sub new
{
	my ($class) = @_;
	return bless {
		krule_id => 0,
		kdir_id => 0,
		num_inp => -1,
		num_outp => -1,
		command => undef,
		rule_name => undef,
	}, $class;
}
};

{package ktr::kobject;
sub new
{
	my ($class) = @_;
	return bless {
		kobj_id => 0,
		kdir_id => 0,
		obj_name => undef,
	}, $class;
}
};

{package ktr::ktask;
sub new
{
	my ($class) = @_;
	return bless {
		ktask_id => 0,
		kdir_id => 0,
		kenv_id => 0,
		krule_id => 0,
		rule_name => undef,
	}, $class;
}
};

{package ktr;
use constant {
	OBJ_INP  => 1,
	OBJ_OUTP => 2,
	OBJ_DEP  => 3,
}
};

{package ktr::ktask_obj;
sub new
{
	my ($class) = @_;
	return bless {
		ktask_obj_id => 0,
		ktask_id => 0,
		kobj_id => 0,
		role => undef,
		obj_orig_name => undef,
	}, $class;
}
};

{package ktr;
use constant {
	KNONE	 => 0,
	KATTR	 => 1,
	KDIR	 => 2,
	KENV	 => 3,
	KVAR	 => 4,
	KRULE	 => 5,
	KOBJECT	 => 6,
	KTASK	 => 7,
	KTASK_OBJ  => 8,
};
};

{package ktr::kattr;
sub new
{
	my ($class) = @_;
	return bless {
		kattr_id => 0,
		kentity_id => 0,
		type => undef,
		value => undef,
	}, $class;
}
};


{package ktr::model;
sub new
{
	my ($class, $root_dir) = @_;

	die unless $root_dir;

	my $m = bless {
		k => undef,
			#std::map<int,kdir*>          dirs;
		dirs => {},
			#std::map<int,std::vector<int>>   dir_defaults;
		dir_defaults => {},
			#std::map<int,std::vector<kvar*>> vars;
		vars => {},
			#std::map<int,kenv*>          envs;
		envs => {},
			#std::map<int,krule*>         rules;
		rules => {},
			#std::map<int,kobject*>       objects;
		objects => {},
			#std::map<int,ktask*>         tasks;
		tasks => {},
			#std::map<int,ktask_obj*> obj_task_rel;
		obj_task_rel => {},
			#std::map<int,std::vector<ktask_obj*>> task_objs;
		task_objs => {},
			#std::map<int,kattr*>         attrs;
		attrs => {},

		next_dir_id => 1,
		next_env_id => 1,
		next_rule_id => 1,
		next_obj_id => 1,
		next_task_id => 1,
		next_task_obj_id => 1,
	}, $class;

	$m->{k} = ktr::k->new;
	$m->{k}->{root_dir} = $root_dir;
	$m->add_dir( '' );

	return $m;
}

#	::k::m::kdir*
#	find_dir( const std::string& dir );
sub find_dir
{
	my ($m, $dir) = @_;
	if (!$dir || $dir eq '.' || $dir eq $m->{k}->{root_dir}) {
		if ($m->{k}->{root_kdir}) {
			return $m->{dirs}->{ $m->{k}->{root_kdir} };
		}
	}
	else {
		for my $kdir ( values %{ $m->{dirs} } ) {
			if ($kdir->{dir_name} eq $dir) {
				return $kdir;
			}
		}
	}
	return undef;
}

#	::k::m::kdir*
#	add_dir( const std::string& dir );
sub add_dir
{
	my ($m, $dir) = @_;
	my $kdir = $m->find_dir( $dir );
	return $kdir if $kdir;
	if ($dir eq '') {
		$kdir = $m->{k}->{root_kdir};
		unless ($kdir) {
			$kdir = ktr::kdir->new;
			$kdir->{kdir_id} = $m->{next_dir_id}++;
			$kdir->{parent_dir_id} = 0;
			$kdir->{kenv_id} = 0;
			$kdir->{dir_name} = $dir;
			$m->{dirs}{ $kdir->{kdir_id} } = $kdir;
			$m->{k}->{root_kdir} = $kdir->{kdir_id};
		}
		else {
			die;
		}
	}
	elsif ($dir =~ m{^/}) {
		$kdir = ktr::kdir->new;
		$kdir->{kdir_id} = $m->{next_dir_id}++;
		$kdir->{parent_dir_id} = $m->{k}->{root_kdir}->{kdir_id};
		$kdir->{kenv_id} = 0;
		$kdir->{dir_name} = $dir;
		$m->{dirs}{ $kdir->{kdir_id} } = $kdir;
	}
	else {
		my $parent;
		if ($dir =~ m{^(.+)/(.+?)$}) {
			$parent = $m->add_dir( $1 )->{kdir_id};
		}
		else {
			$parent = $m->{k}->{root_kdir};
		}
		die unless $parent;
		$kdir = ktr::kdir->new;
		$kdir->{kdir_id} = $m->{next_dir_id}++;
		$kdir->{parent_dir_id} = $parent;
		$kdir->{kenv_id} = 0;
		$kdir->{dir_name} = $dir;
		$m->{dirs}{ $kdir->{kdir_id} } = $kdir;
	}
	die unless $kdir;

	if ($kdir->{kenv_id} == 0) {
		my $parent_env_id = 
			$kdir->{parent_dir_id}
				? $m->{dirs}->{ $kdir->{parent_dir_id} }->{kenv_id}
				: 0 ;
		my $env = $m->add_env( $parent_env_id );
		$kdir->{kenv_id} = $env->{kenv_id};
	}
	return $kdir;
}

#	::k::m::kenv*
#	add_env( int parent_env_id );
sub add_env
{
	my ($m, $parent_env_id) = @_;
	my $kenv = new ktr::kenv;
	$parent_env_id = 0 unless $parent_env_id;
	$kenv->{kenv_id} = $m->{next_env_id}++;
	$kenv->{parent_env_id} = $parent_env_id;
	$m->{envs}->{ $kenv->{kenv_id} } = $kenv;
	return $kenv;
}

#	// krule
#	::k::m::krule*
#	find_rule( ::k::m::kdir* dir, const std::string& rule_name, bool recurse = true );
sub find_rule
{
	my ($m, $dir, $rule_name, $recurse) = @_;
	$recurse = 1 unless defined $recurse;

	$dir = $m->{dirs}->{ $dir } unless ref $dir;

	for my $krule ( values %{ $m->{rules} }) {
		if ($krule->{kdir_id} == $dir->{kdir_id} && $krule->{rule_name} eq $rule_name) {
			return $krule;
		}
	}
	if ($recurse && $dir->{parent_dir_id}) {
		return $m->find_rule( $dir->{parent_dir_id}, $rule_name, $recurse );
	}
	return undef;
}

#	::k::m::krule*
#	add_rule( ::k::m::kdir* dir, const std::string& rule_name );
sub add_rule
{
	my ($m, $dir, $rule_name, $command) = @_;
	die unless $rule_name;
	$dir = $m->{dirs}->{ $dir } unless ref $dir;
	my $krule = $m->find_rule( $dir, $rule_name, 0 );
	return $krule if $krule;

	$krule = new ktr::krule;
	$krule->{krule_id} = $m->{next_rule_id}++;
	$krule->{kdir_id} = $dir->{kdir_id};
	$krule->{rule_name} = $rule_name;
	$krule->{command} = $command || '';
	$m->{rules}->{ $krule->{krule_id} } = $krule;
	return $krule;
}


#	// var
#	::k::m::kvar*
#	find_var( int env_id, const std::string& var_name, bool recurse = true );
sub find_var
{
	my ($m, $env, $var_name, $recurse) = @_;
	$recurse = 1 unless defined $recurse;

	my $env_id = ref $env ? $env->{kenv_id} : $env;

	return undef unless $env_id;

	for my $kvar (@{ $m->{vars}{$env_id} }) {
		if ($kvar->{var_name} eq $var_name) {
			return $kvar;
		}
	}
	if ($recurse) {
		my $p = $m->{envs}{$env_id}{parent_env_id};
		if ($p) {
			return $m->find_var( $p, $var_name, $recurse );
		}
	}
	return undef;
}

#	::k::m::kvar*
#	add_var( int env_id, const std::string& var_name );
#	::k::m::kvar*
#	add_var( int env_id, const std::string& var_name, 
#		const std::string& value );
sub add_var
{
	my ($m, $env_id, $var_name, $var_value) = @_;
	$env_id = $env_id->{kenv_id} if ref $env_id;
	my $kvar = $m->find_var( $env_id, $var_name );
	if ($kvar) {
		$kvar->{var_value} = $var_value if defined $var_value;
		return $kvar;
	}
	$kvar = new ktr::kvar;
	$kvar->{kenv_id} = $env_id;
	$kvar->{var_name} = $var_name;
	$kvar->{var_value} = $var_value;
	$m->{vars}{$env_id} = [] unless $m->{vars}{$env_id};
	push @{ $m->{vars}{$env_id} }, $kvar;
	return $kvar;
}

#	std::string
#	expand_var_string( int env_id, const std::string &str );
sub expand_var_string
{
	my ($m, $env_id, $str) = @_;
	$str =~ s(%{(.*?)})(my $v=$m->find_var($1); $v ? $m->expand_var_string($env_id,$v->{var_value}) : '')ge;
	return $str;
}


#	// object
#	::k::m::kobject*
#	add_object( ::k::m::kdir* dir, const std::string& name );
sub add_object
{
	my ($m, $dir, $name_) = @_;
	die unless $name_;
	$dir = $m->{dirs}{$dir} unless ref $dir;
	my $name = $m->expand_var_string( $dir->{kenv_id}, $name_ );

	my $kobj;
	if ($name !~ m{/}) {
		$kobj = new ktr::kobject;
		$kobj->{kobj_id} = $m->{next_obj_id}++;
		$kobj->{kdir_id} = $dir->{kdir_id};
		$kobj->{obj_name} = $name;
		$m->{objects}{$kobj->{kobj_id}} = $kobj;
	}
	else {
		my $dirname;
		my $basename;
		if ($name =~ m{^(/.+)/([^/]+?)$}) {
			$dirname = $1;
			$basename = $2;
		}
		else {
			my $tmp0 = '';
			$tmp0 = $dir->{dir_name} + '/' if $dir->{dir_name};
			$tmp0 .= $name;
			$tmp0 = ktr::tl::normalize_path($tmp0);
			$tmp0 =~ m{^(.+/)?([^/]+?)$} or die "tmp0: $tmp0";
			$dirname = $1;
			$basename = $2;
		}
		my $kdir = $m->add_dir( $dirname );
		$kobj = new ktr::kobject;
		$kobj->{kobj_id} = $m->{next_obj_id}++;
		$kobj->{kdir_id} = $kdir->{kdir_id};
		$kobj->{obj_name} = $basename;
		$m->{objects}{$kobj->{kobj_id}} = $kobj;
	}
	return $kobj;
}

#	// task
#	::k::m::ktask*
#	add_task( ::k::m::kdir* dir, const std::string& rule_name );
#	::k::m::ktask*
#	add_task( ::k::m::kdir* dir, ::k::m::krule* rule );
sub add_task
{
	my ($m, $dir, $rule, $li, $lo, $ld) = @_;
	$dir = $m->{dirs}{$dir} unless ref $dir;

	my $t = new ktr::ktask;
	$t->{ktask_id} = $m->{next_task_id}++;
	$t->{kdir_id} = $dir->{kdir_id};
	$t->{kenv_id} = 0;
	$t->{krule_id} = ref $rule ? $rule->{krule_id} : 0 ;
	$t->{rule_name} = ref $rule ? $rule->{rule_name} : $rule;
	$m->{tasks}{$t->{ktask_id}} = $t;

	if ($li && ref($li) eq 'ARRAY') {
		$m->task_add_object( $t, ref $_ ? $_ : $m->add_object( $dir, $_ ), ktr::OBJ_INP ) 
			for @$li;
	}
	if ($lo && ref($lo) eq 'ARRAY') {
		$m->task_add_object( $t, ref $_ ? $_ : $m->add_object( $dir, $_ ), ktr::OBJ_OUTP ) 
			for @$lo;
	}
	if ($ld && ref($ld) eq 'ARRAY') {
		$m->task_add_object( $t, ref $_ ? $_ : $m->add_object( $dir, $_ ), ktr::OBJ_DEP ) 
			for @$ld;
	}

	return $t;
}

#	// task_objs
#	::k::m::ktask_obj*
#	task_add_object( ::k::m::ktask* task, ::k::m::kobject* obj,
#		::k::m::role_type role, const std::string& obj_orig_name = std::string() );
#	::k::m::ktask_obj*
#	task_add_object( ::k::m::ktask* task, ::k::m::ktask_obj* ot,
#		::k::m::role_type role );
sub task_add_object
{
	my ($m, $task, $object, $role, $obj_orig_name) = @_;
	if (ref($object) eq 'ktr::ktask_obj') {
		my $tt = $object;
		$object = $m->{objects}{$tt->{kobj_id}};
		$obj_orig_name = $tt->{obj_orig_name};
	}

	my $kobj = ref $object ? $object : $m->{objects}{$object};
	my $ktask_id = ref $task ? $task->{ktask_id} : $task;

	my $ot = new ktr::ktask_obj;
	$ot->{ktask_obj_id} = $m->{next_task_obj_id}++;
	$ot->{ktask_id} = $ktask_id;
	$ot->{kobj_id} = $kobj->{kobj_id};
	$ot->{role} = $role;
	$ot->{obj_orig_name} = $obj_orig_name || $kobj->{obj_name} ;

	$m->{obj_task_rel}{ $ot->{ktask_obj_id} } = $ot;
	$m->{task_objs}{$ktask_id} = [] unless $m->{task_objs}{$ktask_id};
	push @{ $m->{task_objs}{$ktask_id} }, $ot;

	return $ot;
}

#	::k::m::ktask_obj*
#	find_task_obj( ::k::m::ktask* t, ::k::m::kobject* o );
#	::k::m::ktask_obj*
#	find_task_obj( int task_id, int obj_id );
sub find_task_obj
{
	my ($m, $task, $object) = @_;
	my $ktask_id = ref $task ? $task->{ktask_id} : $task;
	my $kobj_id = ref $object ? $object->{kobj_id} : $object;
	for my $ot ( @{ $m->{task_objs}{$ktask_id} } ) {
		if ($ot->{kobj_id} == $kobj_id) {
			return $ot;
		}
	}
}

sub find_task_obj_drr
{
	my ($m, $dir, $rule_name, $role) = @_;
	my @ret;
	$dir = $dir->{kdir_id} if ref $dir;
	for my $task ( values %{ $m->{tasks} } ) {
		next unless $task->{kdir_id} == $dir;
		next unless $task->{rule_name} eq $rule_name;
		for my $ot ( @{ $m->{task_objs}{$task->{ktask_id}} } ) {
			next unless $ot->{role} == $role;
			push @ret, $ot;
		}
	}
	return @ret;
}

sub dump
{
	my ($m, $entt, $io) = @_;

	my $sav_io;
	$sav_io = select $io if $io;

	$entt = ktr::KNONE unless defined $entt;

	if ($entt == ktr::KNONE) {
		print "--- dirs --------\n"; $m->dump( ktr::KDIR );
		print "--- envs --------\n"; $m->dump( ktr::KENV );
		print "--- vars --------\n"; $m->dump( ktr::KVAR );
		print "--- rules -------\n"; $m->dump( ktr::KRULE );
		print "--- tasks -------\n"; $m->dump( ktr::KTASK );
		print "--- task_objs ---\n"; $m->dump( ktr::KTASK_OBJ );
		print "--- objs --------\n"; $m->dump( ktr::KOBJECT );
	}
	elsif ($entt == ktr::KATTR) {
		print "# here go the attrs\n";
	}
	elsif ($entt == ktr::KDIR) {
		print "dir_id\tparent\tenv_id\tname\n";
		for my $o ( sort {$a->{kdir_id}<=>$b->{kdir_id}} values %{$m->{dirs}} ) {
			print $o->{kdir_id}, "\t";
			print $o->{parent_dir_id}, "\t";
			print $o->{kenv_id}, "\t";
			print $o->{dir_name}, "\n";
		}
	}
	elsif ($entt == ktr::KENV) {
		print "env_id\tparent_env_id\n";
		for my $o ( sort {$a->{kenv_id}<=>$b->{kenv_id}} values %{$m->{envs}} ) {
			print $o->{kenv_id}, "\t";
			print $o->{parent_env_id}, "\n";
		}
	}
	elsif ($entt == ktr::KVAR) {
		print "env_id\tname\tvalue\n";
		for my $kenv_id ( sort {$a<=>$b} keys %{$m->{vars}} ) {
			for my $o ( @{$m->{vars}{$kenv_id}} ) {
				print $kenv_id, "\t";
				print $o->{var_name}, "\t";
				print defined $o->{var_value}
					? $o->{var_value}
					: '<<undefined>>', "\n"
			}
		}
	}
	elsif ($entt == ktr::KRULE) {
		print "dir_id\trule_id\tname\tinp\toutp\tcommand\n";
		for my $o ( sort {$a->{krule_id}<=>$b->{krule_id}} values %{$m->{rules}} ) {
			print $o->{kdir_id}, "\t";
			print $o->{krule_id}, "\t";
			print $o->{rule_name}, "\t";
			print $o->{num_inp}, "\t";
			print $o->{num_outp}, "\t";
			print $o->{command}, "\n";
		}
	}
	elsif ($entt == ktr::KOBJECT) {
		print "dir_id\tobj_id\tname\tdir_name\n";
		for my $o ( sort {$a->{kobj_id}<=>$b->{kobj_id}} values %{$m->{objects}} ) {
			print $o->{kdir_id}, "\t";
			print $o->{kobj_id}, "\t";
			print $o->{obj_name}, "\t";
			print $m->{dirs}{ $o->{kdir_id} }{dir_name}, "\n";
		}
	}
	elsif ($entt == ktr::KTASK) {
		print "dir_id\ttask_id\tenv_id\trule_id\trule_name\n";
		for my $o ( sort {$a->{ktask_id}<=>$b->{ktask_id}} values %{$m->{tasks}} ) {
			print $o->{kdir_id}, "\t";
			print $o->{ktask_id}, "\t";
			print $o->{kenv_id}, "\t";
			print $o->{krule_id}, "\t";
			print $o->{rule_name}, "\n";
		}
	}
	elsif ($entt == ktr::KTASK_OBJ) {
		print "t_o_id\ttask_id\tobj_id\trole\n";
		for my $task_id ( sort {$a<=>$b} keys %{$m->{task_objs}} ) {
			for my $o ( @{ $m->{task_objs}{$task_id} } ) {
				print $o->{ktask_obj_id}, "\t";
				print $o->{ktask_id}, "\t";
				print $o->{kobj_id}, "\t";
				print "input" if $o->{role} == ktr::OBJ_INP;
				print "output" if $o->{role} == ktr::OBJ_OUTP;
				print "dependency" if $o->{role} == ktr::OBJ_DEP;
				print "\n";
			}
		}
	}

	select $sav_io if $io;
}

};

{package ktr::tl;
sub normalize_path
{
	my ($str, $do_die) = @_;
	my $orig_str = $str;

	1 while $str =~ s!/\./!/!g;

	$str =~ s!//+!/!g;

	while ($str =~ m!/([^/]+)/\.\./!) {
		if ($1 eq '..') {
			die "bad path: $orig_str" if $do_die;
			return undef;
		}
		$str =~ s!/([^/]+)/\.\./!/! or die;
	}
	return $str;
}
sub chext
{
	my ($str, $a, $b) = @_;
	s/^\.// for grep {defined} ($a, $b);
	if (defined $b && defined $b) {
		$str =~ s!\.\Q$a\E$!\.$b! or die "can't replace the ext: .$a -> .$b: $str";
	}
	elsif (defined $a) {
		$str =~ s!\.[^\.]+$!\.$a! or die "can't replace the ext: to .$a: $str";
	}
	else {
		# simply strip extension
		$str =~ s!\.[^\.]+$!!;
	}
	return $str;
}
};

 #####    ####   #####     ##    #####   #    #
 #    #  #    #  #    #   #  #   #    #  #    #
 #    #  #       #    #  #    #  #    #  ######
 #    #  #  ###  #####   ######  #####   #    #
 #    #  #    #  #   #   #    #  #       #    #
 #####    ####   #    #  #    #  #       #    #

####################################################

{package ktr::dgraph;
sub new
{
	my ($class, $m) = @_;
	return bless {
		m => $m,
		toutp_task => {},
		tprereq => {},
		tcontrib => {},
	}, $class;
}

sub init_dgraph
{
	my ($dg) = @_;
	$dg->fill_outp_task;
	$dg->fill_prereq;
	$dg->fill_contrib;
}

sub fill_outp_task
{
	my ($dg) = @_;
	my $m = $dg->{m};
	for my $kov ( values %{ $m->{task_objs} } ) {
		for my $ko ( @$kov ) {
			if ($ko->{role} == ktr::OBJ_OUTP) {
				# die if $dg->{toutp_task}{$ko->{kobj_id}}
				$dg->{toutp_task}{$ko->{kobj_id}} = $ko->{ktask_id};
			}
		}
	}
}

sub fill_prereq
{
	my ($dg) = @_;
	my $m = $dg->{m};
	for my $kov ( values %{ $m->{task_objs} } ) {
		for my $ko ( @$kov ) {
			if ($ko->{role} == ktr::OBJ_INP || $ko->{role} == ktr::OBJ_DEP) {
				my $ktask_id = $dg->{toutp_task}{ $ko->{kobj_id} } || 0;
				if ($ktask_id) {
					# $ktask_id is the task which produces the object
					# $ko->{task_id} depends on the $ktask_id
					$dg->{tprereq}{ $ko->{task_id} }{ $ktask_id } = 1;
				}
				else {
					# object is not produced. must be a 'source'.
				}
			}
		}
	}
}

sub fill_contrib
{
	my ($dg) = @_;
	# reverse the prereq
	my $co = $dg->{contrib};
	my $pr = $dg->{tprereq};
	for my $task_b ( keys %{ $pr } ) {
		for my $task_a ( keys %{ $pr->{$task_b} } ) {
			$co->{ $task_a } = [] unless exists $co->{$task_a};
			push @{ $co->{$task_a} }, $task_b;
		}
	}
}

};



 #####    ####    #####    ##     #####  ######
 #    #  #          #     #  #      #    #
 #####    ####      #    #    #     #    #####
 #    #       #     #    ######     #    #
 #    #  #    #     #    #    #     #    #
 #####    ####      #    #    #     #    ######

####################################################


{package ktr::bstate;
use constant {
	TASK_FINISHED => 0,
	TASK_RUNNING => 1,
	TASK_PENDING => 2,
};
sub new
{
	my ($class, $m, $dg) = @_;
	return bless {
		m => $m,
		dg => $dg,
		tstates => {},
		build_queue => {},
	}, $class;
}

#
#  TASK STATES
#

sub update_task_state
{
	my ($bs, $ktask_id, $state, $num_prereq) = @_;
	my $hr = { 
		ktask_id => $ktask_id,
		st => $state,
		num_prereq => $num_prereq 
	};
	$bs->{tstates}{$ktask_id} = $hr;
	return $hr;
}
sub get_task_state
{
	my ($bs, $ktask_id) = @_;
	if (exists $bs->{tstates}{$ktask_id}) {
		my $hr = $bs->{tstates}{$ktask_id};
		return wantarray ? ( $hr->{st}, $hr->{num_prereq} ) : ( $hr );
	}
	return undef;
}

sub fill_states_for_obj
{
	my ($bs, $obj_id) = @_;
	my $dg = $bs->{dg};
	if (exists $dg->{toutp_task}{$obj_id}) {
		$bs->fill_states_for_task( $dg->{toutp_task}{$obj_id} );
	}
}

sub fill_states_for_task
{
	my ($bs, $task_id_) = @_;
	my $pr = $bs->{dg}{tprereq};
	my @list = ($task_id_);
	while (my $task_id = shift @list) {
		next if exists $bs->{tstates}{$task_id};
		my $ts = $bs->update_task_state( $task_id, TASK_PENDING, 0 );
		next unless exists $pr->{$task_id};
		for my $ptid ( keys %{ $pr->{$task_id} } ) {
			$ts->{num_prereq}++;
			push @list, $ptid;
		}
	}
}

#
#  BUILD QUEUE
#

sub fill_build_queue
{
	my ($bs) = @_;
	my $bq = $bs->{build_queue};
	for my $ts ( values %{ $bs->{tstates} } ) {
		$bq->{ $ts->{num_prereq} }{ $ts->{ktask_id} } = 1;
	}
}

sub decr_prereq
{
	my ($bs, $ktask_id) = @_;
	my $ts = $bs->get_task_state( $ktask_id );
	die unless $ts;
	my $bq = $bs->{build_queue};
	if ($ts->{num_prereq} > 0) {
		delete $bq->{ $ts->{num_prereq} }{ $ts->{ktask_id} };
		$ts->{num_prereq}--;
		$bq->{ $ts->{num_prereq} }{ $ts->{ktask_id} } = 1;
	}
	elsif ($ts->{num_prereq} == 0) {
		delete $bq->{ $ts->{num_prereq} }{ $ts->{ktask_id} };
	}
	else {
		die;
	}
}

sub notify_contrib
{
	my ($bs, $ktask_id) = @_;
	my $co = $bs->{dg}->{tcontrib};
	if (exists $co->{$ktask_id}) {
		for my $tid ( @{ $co->{$ktask_id} } ) {
			$bs->decr_prereq( $tid );
		}
	}
}
sub update_build_state
{
	my ($bs, $task_id, $new_state) = @_;
	my $ts = $bs->get_task_state( $task_id );
	die unless $ts;

	if ($ts->{st} == TASK_PENDING) {
		$ts->{st} = $new_state;
		if ($ts->{st} == TASK_FINISHED) {
			$bs->notify_contrib( $task_id );
		}
	}
	elsif ($ts->{st} == TASK_RUNNING) {
		#  allow to go back waiting.
		$ts->{st} = $new_state;
		if ($new_state == TASK_FINISHED) {
			$bs->notify_contrib( $task_id );
		}
	}
	elsif ($ts->{st} == TASK_FINISHED) {
		if ($new_state == TASK_FINISHED) {
			# nothing
		}
		else {
			die;
		}
	}
	else {
		die;
	}
}

sub init_task_env
{
	my ($bs, $task) = @_;
	my $m = $bs->{m};

	$task = $m->{tasks}{$task} unless ref $task;

	return if $task->{kenv_id};

	my $tenv = $m->add_env( $m->{dirs}{$task->{kdir_id}}{kenv_id} );
	$task->{kenv_id} = $tenv->{kenv_id};

	my $inp_num = 1;
	my $outp_num = 1;
	my @inp;
	my @outp;
	for my $to ( @{ $m->{task_objs}{ $task->{ktask_id} } } ) {
		if ($to->{role} == ktr::OBJ_INP) {
			push @inp, $to->{obj_orig_name};
			$m->add_var( $tenv, "input$inp_num", $to->{obj_orig_name} );
		}
		elsif ($to->{role} == ktr::OBJ_OUTP) {
			push @outp, $to->{obj_orig_name};
			$m->add_var( $tenv, "output$inp_num", $to->{obj_orig_name} );
		}
	}
	$m->add_var( $tenv, "input", join( ' ', @inp ) );
	$m->add_var( $tenv, "output", join( ' ', @outp ) );
}

sub get_task_cmd
{
	my ($bs, $task_id, $expand) = @_;
	my $m = $bs->{m};
	my $task = ref $task_id ? $task_id : $m->{tasks}{$task_id};
	$expand = 1 unless defined $expand;

	my $rule;
	if (!$task->{krule_id}) {
		$bs->init_task_env( $task );
		my $rule_name = $m->expand_var_string( $task->{kenv_id}, $task->{rule_name} );
		die unless $rule_name;
		$rule = $m->find_rule( $task->{kdir_id}, $rule_name );
		$task->{krule_id} = $rule->{krule_id} if $rule;
	}
	else {
		$rule = $m->{rules}{ $task->{krule_id} };
	}

	die unless $rule;
	return undef unless $rule;

	my $cmd;
	if ($expand) {
		$bs->init_task_env( $task );
		$cmd = $m->expand_var_string( $task->{kenv_id}, $rule->{command} );
	}
	else {
		$cmd = $rule->{command};
	}
	return $cmd;
}

sub get_task_dir
{
	my ($bs, $task_id, $relative) = @_;
	$relative = 1 unless defined $relative;

	my $m = $bs->{m};
	my $task = ref $task_id ? $task_id : $m->{tasks}{$task_id};
	my $dir_id = $task->{kdir_id};
	my $dir = $m->{dirs}{$dir_id};

	my $dir_name;
	if ($relative) {
		$dir_name = $dir->{dir_name};
	}
	else {
		if (!$dir->{dir_name}) {
			$dir_name = $m->{k}{root_dir};
		}
		elsif ($dir->{dir_name} =~ m{^/}) {
			$dir_name = $dir->{dir_name};
		}
		else {
			$dir_name = $m->{k}{root_dir}.'/'.$dir->{dir_name};
		}
	}
	$dir_name = '.' unless $dir_name;
	return $dir_name;
}

};

sub main
{
	my $m = ktr::model->new( $ENV{PWD} );
	my $d = $m->find_dir('.');

	$m->add_var( $d, 'CXX', 'c++' );
	$m->add_var( $d, "CXXFLAGS", '-O0 -std=c++11 -Wall -g' );

	$m->add_rule( $d, 'compile.cxx', '%{CXX} %{CXXFLAGS} %{input} -c -o %{output}' );
	$m->add_rule( $d, 'link.cxx', '%{CXX} %{CXXFLAGS} %{input} -o %{output}' );

	for (qw/ktor.cc utilk.cc maink.cc filek.cc/) {
		$m->add_task( $d, 'compile.cxx', 
				[$_], 
				['o/'.ktr::tl::chext($_, '.o')]
			);
	}

	my $t = $m->add_task( $d, "link.cxx", 
			['ktr'], 
			[ $m->find_task_obj_drr( $d, "compile.cxx", ktr::OBJ_OUTP ) ]
		);

	$m->dump();
}

main();
