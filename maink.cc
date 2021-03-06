
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "k.hh"

#define p( ... ) fprintf( f, __VA_ARGS__ )
void print_usage( FILE *f )
{
	p( "ktr " KTR_VERSION "\n" );
	p( "  usage: ktr [options] [targets]\n" );
	p( "  options:\n" );
	p( "  --build    build mode; build given targets\n" );
	p( "  --test     test mode; indicate with exit status if rebuild is required\n" );
	p( "  --print    print mode; print commands instead of executing them\n" );
	p( "  --clean    clean mode; clean the projects, identified by the targets\n" );
	p( "  --query    query mode; print the targets, with wildcards expanded\n" );
	p( "  --dump     dump mode; dump the project kfiles\n" );
	p( "  --jobs     in build mode, how many build jobs to run in parallel\n" );
	p( "  --version  print version and exit\n" );
	p( "  --help     print usage and exit\n" );
}
#undef p

int kmain(int argc, char **argv, K::KOpt &opts)
{
	int c;
	char *p;
	bool err = false;

	while (!err) {
		int option_index = 0;
		static struct option long_options[] = {
			{"kfile-kpart",	required_argument,	0,  'W' },
			{"jobs",	required_argument,	0,  'j' },
			{"force",	no_argument,		0,  'f' },
			{"silent",	no_argument,		0,  'q' },
			{"queit",	no_argument,		0,  'q' },
			{"verbose",	no_argument,		0,  'v' },
			{"version",	no_argument,		0,  'V' },
			{"print",	no_argument,		0,  'p' },
			{"build",	no_argument,		0,  'b' },	// build
			{"test",	no_argument,		0,  't' },	// test if build is required
			{"check",	no_argument,		0,  'K' },	// check of the kfiles TODO
			{"clean",	no_argument,		0,  'c' },
			{"query",	no_argument,		0,  'Q' },
			{"dump",	no_argument,		0,  'D' },
			{"help",	no_argument,		0,  'h' },
			{}
		};

		c = getopt_long(argc, argv, "fqvVhj:W:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'W':
				if ((p = strchr(optarg,','))) {
					opts.kfile = std::string( optarg, (size_t)(p - optarg) );
					opts.kpart = std::string( p+1, strlen(p+1) );
				}
				else {
					fprintf( stderr, "bad\n" );
					err = true;
				}
				break;

			case 'f':
				opts.force = true;
				break;

			case 'v':
				if (opts.verbose_level < K::KOpt::VL_DEBUG)
					opts.verbose_level++;
				break;

			case 'q':
				if (opts.verbose_level > K::KOpt::VL_DEAD)
					opts.verbose_level--;
				break;

			case 'j':
				opts.jobs = atoi(optarg);
				if (opts.jobs < 1)
					opts.jobs = 1;
				break;

			case 'p': opts.command = K::KOpt::CMD_PRINT; break;
			case 'b': opts.command = K::KOpt::CMD_BUILD; break;
			case 'c': opts.command = K::KOpt::CMD_CLEAN; break;
			case 't': opts.command = K::KOpt::CMD_TEST; break;
			case 'K': opts.command = K::KOpt::CMD_CHECK; break;
			case 'D': opts.command = K::KOpt::CMD_DUMP; break;
			case 'Q': opts.command = K::KOpt::CMD_QUERY; break;
			case 'V': opts.command = K::KOpt::CMD_VERSION; break;
			case 'h': opts.command = K::KOpt::CMD_USAGE; break;

			case '?':
			default:
				err = true;
				printf("?? getopt returned character code %#x ??\n", c);
				break;
		}
	}

	if (err)
		opts.command = K::KOpt::CMD_USAGE;

	if (opts.command == K::KOpt::CMD_NONE)
		opts.command = K::KOpt::CMD_TEST;

	if (optind < argc) {
		while (optind < argc)
			opts.targets.push_back( argv[optind++] );
	}

	return (int)err;
}

