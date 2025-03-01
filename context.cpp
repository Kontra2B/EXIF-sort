#include <iostream>
#include <unistd.h>

#include "helper.hpp"
#include "context.hpp"

bool Context::verbose = false;
bool Context::debug = false;
bool Context::confirm = false;

using namespace std;

ostream& operator<<(ostream& oss, const Context& context) {
	oss << "Working directory:";
	for (const auto& dir: context.dirs) oss << dir << ",";
	oss << ' ';
	if (context.dirs.front() != context.out)
		oss << "target directory:" << context.out << ", ";
	if (!context.prefer.empty()) {
		oss << "prefer:";
		for (const auto& key: context.prefer) oss << key << ',';
		oss << ' ';
	}
	if (!context.keys.empty()) {
		oss << "names:";
		for (const auto& key: context.keys) oss << key << ',';
		oss << ' ';
	}
	if (!context.avoid.empty()) {
		oss << "avoid:";
		for (const auto& key: context.avoid) oss << key << ',';
		oss << ' ';
	}
	if (context.recurse) oss << "recursive, ";
	if (context.count >= 0) oss << "count:" << context.count << ", ";
	if (context.skip > 0) oss << "skip:" << context.skip << ", ";
	if (context.dups) oss << "save duplicate list, ";
	if (context.verbose) {
		if (context.debug) oss << "debug, ";
		else oss << "verbose, ";
	}
	if (context.suppress()) oss << "suppress, ";
	if (context.confirm) oss << "confirm, ";
	if (context.move) {
		oss << "sorting path:/yyyy/";
		if (context.format > Context::Format::Year) oss << "mm/";
		if (context.format > Context::Format::Month) oss << "dd/";
		oss << ", ";
	}
	oss << "pid:" << getpid() << endl;
	if (context.move) oss << "MOVING files: " << (context.all? "all": "pictures") << endl;
	return oss << endl;
}

Context::Context()
{
	move = recurse = verbose = debug = confirm = force = sup = dups = all = false;
	format = Context::Format::Month;
	count = -1L;
	skip = 0;
}
