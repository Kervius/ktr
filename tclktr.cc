
// prototype

struct KtrTclData
{
	Ktr::KModel *km;
	Ktr::DGraph *dg;
	Ktr::BState *bs;

	Ktr::KDir *curDir;

	std::string rootDirName;

	std::string curDirName;

	std::list<std::string> subDirList;
};


static KtrTclData *K;

static const struct {
	char name[12];
	Jim_CmdProc proc;
} KtrTclCmdNameMap[] = {
	{ "var",	KtrTclCmdVar, 	},
	{ "do",		KtrTclCmdDo, 	},
	{ "rule",	KtrTclCmdRule, 	},
	{ "default",	KtrTclCmdDefault,	},
	{ "subdir",	KtrTclCmdSubdir,	},
};


static int
KtrTclCmdVar(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	//K->km->AddVar()
}

static int
KtrTclCmdDo(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	//K->km->AddTask()
}

static int
KtrTclCmdRule(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	//K->km->AddRule()
}

static int
KtrTclCmdDefault(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	//K->km->dirDefaults
}

static int
KtrTclCmdSubdir(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	const char *str;
	int len;

	for (int i=1; i<argc; i++)
	{
		str = Jim_GetString(argv[i], &len);
		if (K->curDirName == ".")
		{
			K->subDirList.push_back( str );
		}
		else
		{
			K->subDirList.push_back( K->curDirName + str );
		}
	}
}

static int
KtrTclCmdKtr(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	const char *str;
	int len;

	str = Jim_GetString(argv[1], &len);

	for (size_t i = 0; i<sizeof(KtrTclCmdNameMap)/sizeof(KtrTclCmdNameMap[0]); i++)
	{
		if (strcmp( str, KtrTclCmdNameMap[i].name ) == 0)
		{
			return KtrTclCmdNameMap[i].proc( interp, argc-1, argv+1 );
		}
	}

	return JIM_ERR;
}

void KtrTclRegister()
{
	//Jim_CreateCommand(interp, "ktr",     KtrTclCmdKtr, NULL, NULL);
	for (size_t i = 0; i<sizeof(KtrTclCmdNameMap)/sizeof(KtrTclCmdNameMap[0]); i++)
	{
		Jim_CreateCommand( interp, KtrTclCmdNameMap[i].name, KtrTclCmdNameMap[i].proc, NULL, NULL );
	}
}

void KtrTclInit()
{
	K = new KtrTclData;

	K->rootDirName = Ktr::Utils::AbsFileName(".");
	K->curDirName = ".";

	K->km = Ktr::KModelCreate( K->rootDirName );

	K->curDur = K->km->FindDir( std::string() );

	K->dg = new DGraph( K->km );

	K->bs = new BState( K->km, K->dg );
}

bool KtrTclNextSubDir()
{
	if (K->subDirList.empty())
		return false;

	std::string nextDirName = K->subDirList.front();
	K->subDirList.pop_front();

	KDir *nextDir = K->km->AddDir( nextDirName );

	if (nextDir)
	{
		K->curDirName = nextDirName;
		K->curDir = nextDir;
		return true;
	}
	else
	{
		return false;
	}
}

int main(int argc, char **argv)
{

	KtrTclInit();

	// load the model

	Jim_EvalFile( interp, "kfile" );

	while ( KtrTclNextSubDir() )
	{
		chdir( K->curDirName.c_str() );

		Jim_EvalFile( interp, subdir + "/kpart" );

		chdir( K->rootDirName.c_str() );
	}

	// process the model
	K->dg->InitDepGraph();

	if (IsBuildMode() || IsQueryMode())
	{
		// prepare build
		if (hasTarget())
			K->bs->FillStatesForObj( GetTarget() );
		else
			K->bs->FillStatesForDefaults( ); // xxx

		// build
		K->bs->FillBuildQueue();

		// xxx

		//K->builder->Build();
	}
	else if (IsCleanMode())
	{
		// delete all task-objects with which are output
	}
	else
	{
		// xxx
	}

	return 0;
}

