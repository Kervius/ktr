#ifndef K_CKSUM_____
#define K_CKSUM_____

#include <string>
#include <vector>
#include <map>

#include <stddef.h>
#include <stdint.h>

struct CkSumEntry {
	CkSumEntry() : fsize(-1u), entry_date(-1), type(0) {};
	std::vector<uint8_t> cksum;
	size_t fsize;
	time_t entry_date;
	struct timespec fmtime;
	char type;
	std::string fname;
};


struct CkSumDB {
	const char *db_fname;
	std::string dir;
	bool modified;
	std::map<std::string, CkSumEntry *> db;
public:
	CkSumDB( const std::string &dir_ )
	:	dir( dir_ ),
		modified( false ),
		db_fname( ".krt.cksumdb" )
	{};
	CkSumEntry *find( const std::string &fn );
	void update( const std::string &fn, size_t fsize, char type, const std::string &cksum );

	void clear();
	bool load();
	bool flush( bool sync = false );
	
	static int serialize( CkSumEntry *e, char *buf, int buf_len );
	static bool deserialize( CkSumEntry *e, char *buf, int buf_len );
};

#endif // K_CKSUM_____
