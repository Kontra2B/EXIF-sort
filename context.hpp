#pragma once
#include <fstream>
#include <list>

struct Context {
	enum class Format	{ Year, Month, Day };
	std::list<std::string>	dirs, prefer, avoid, keys;
	std::string			out;					// recovery working an move target directory
	int64_t				count, skip;			// counters for limited output
	bool				move, recurse, force, sup, dups, all;
	static bool			verbose, debug, confirm;
	Format				format;
	Context();
	bool suppress() const { return sup && !verbose; }
};

std::ostream& operator<<(std::ostream&, const Context&);
