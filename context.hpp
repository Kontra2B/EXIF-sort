#pragma once
#include <fstream>
#include <semaphore.h>
#include <unordered_map>
#include <set>
#include <mutex>

using LBA = uint64_t;

// enum class Verbose { Normal, Verbose, Debug };

struct Context {
	enum class Format{ Year, Month, Day };
	std::set<std::string>	dirs, prefer, avoid, name;
	std::string			out;					// recovery working an move target directory
	int64_t				count, skip;			// counters for limited output
	bool				move, force, sup, dups, all;
	static bool			verbose, debug, confirm;
	Format				format;
	Context();
	bool suppress() { return sup && !verbose; }
};

std::ostream& operator<<(std::ostream&, const Context&);
