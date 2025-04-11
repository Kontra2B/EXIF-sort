#pragma once
#include <string>
#include <list>
#include <variant>

using option = std::variant<std::monostate, int64_t*, std::string*, std::list<std::string>*>;

struct Context {
	enum class Format	{ Year, Month, Day };
	std::list<std::string>	dirs, prefer, avoid, keys;
	std::string			out, dups;				// recovery working an move target directory
	int64_t				count, skip;			// counters for limited output
	bool				move, recurse, force, sup, all, help;
	static bool			verbose, debug, confirm;
	Format				format;
	Context();
	bool suppress() const { return sup && !verbose; }
	void parse(int, char**);
	void set(option*, const char*);
};

std::ostream& operator<<(std::ostream&, const Context&);
