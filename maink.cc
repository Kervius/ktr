
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "k.hh"

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
			{"build",	no_argument,		0,  'b' },
			{"clean",	no_argument,		0,  'c' },
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
			case 'D': opts.command = K::KOpt::CMD_DUMP; break;
			case 'V': opts.command = K::KOpt::CMD_VERSION; break;
			case 'h': opts.command = K::KOpt::CMD_USAGE; break;

			case '?':
			default:
				err = true;
				printf("?? getopt returned character code 0%o ??\n", c);
				break;
		}
	}

	if (err)
		opts.command = K::KOpt::CMD_USAGE;

	if (opts.command == K::KOpt::CMD_NONE)
		opts.command = K::KOpt::CMD_BUILD;

	if (optind < argc) {
		while (optind < argc)
			opts.targets.push_back( argv[optind++] );
	}

	return (int)err;
}

