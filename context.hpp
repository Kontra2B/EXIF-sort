#pragma once
#include <string>
#include <list>
#include <variant>

using option = std::variant<std::monostate, int64_t*, std::string*, std::list<std::string>*>;

struct Context {
	enum class Format	{ None, Year, Month, Day };
	enum class Action	{ None, Move, Hard, Soft };
	std::list<std::string>	dirs, prefer, avoid, keys;
	std::string			out, dups;				// recovery working an move target directory
	int64_t				count, skip;			// counters for limited output
	bool				recurse, force, sup, all, help;
	Action				action;
	static bool			verbose, debug, confirm;
	Format				format;
	Context();
	bool suppress() const { return sup && !verbose; }
	void parse(int, char**);
	void set(option*, const char*);
	bool dryrun() const { return action == Action::None; }
	bool move() const { return action == Action::Move; }
	bool hard() const { return action == Action::Hard; }
	bool soft() const { return action == Action::Soft; }
};

std::ostream& operator<<(std::ostream&, const Context&);
