#include <iostream>
#include <unistd.h>

#include "context.hpp"

bool Context::verbose = false;
bool Context::debug = false;
bool Context::confirm = false;

using namespace std;

ostream& operator<<(ostream& oss, const Context& context) {
	oss << "Working director" << (context.dirs.size() < 2? "y": "ies") << '[';
	for (const auto& dir: context.dirs) oss << dir << ",";
	oss << "\b] ";
	if (context.recurse) oss << "recursive, ";
	if (!context.move && context.dirs.front() != context.out)
		oss << "target directory:" << context.out << ", ";
	if (!context.prefer.empty()) {
		oss << "prefer[";
		for (const auto& key: context.prefer) oss << key << ',';
		oss << "\b] ";
	}
	if (!context.avoid.empty()) {
		oss << "avoid[";
		for (const auto& key: context.avoid) oss << key << ',';
		oss << "\b] ";
	}
	if (!context.keys.empty()) {
		oss << "names[";
		for (const auto& key: context.keys) oss << key << ',';
		oss << "\b] ";
	}
	if (context.count >= 0) oss << "count:" << context.count << ", ";
	if (context.skip > 0) oss << "skip:" << context.skip << ", ";
	if (context.verbose) {
		if (context.debug) oss << "debug, ";
		else oss << "verbose, ";
	}
	if (context.suppress()) oss << "suppress, ";
	if (context.confirm) oss << "confirm, ";
	if (!context.dups.empty()) oss << "duplicates:" << context.dups << ", ";
	if (context.help || context.move) {
		if (context.format == Context::Format::None) oss << "remove path";
		else {
			oss << "sorting:/yyyy/";
			if (context.format > Context::Format::Year) oss << "mm/";
			if (context.format > Context::Format::Month) oss << "dd/";
		}
		oss << ", ";
	}
	oss << "pid:" << getpid() << endl;
	if (context.move)
		oss << "MOVING " << (context.all? "all files": "pictures")
			<< " to target directory: " << context.out << endl;
	return oss << endl;
}

void Context::set(option* param, const char* value)
{
	if (!*value) return;
	if (auto target = get_if<int64_t*>(param)) **target = strtol(value, nullptr, 0);
	else if (auto target = get_if<string*>(param)) **target = value;
	else if (auto target = get_if<list<string>*>(param)) (*target)->push_back(value);
	*param = monostate{};
}

void Context::parse(int argn, char** argv)
{
	option pending;

	for (int i = 1; i < argn; i++) {
		char* arg = argv[i];
		if (*arg == '-') {
			pending = monostate{};
			while (*++arg) {
				// no-parameter options
				if (*arg == 'h') help = true;
				else if (*arg == 'R') move = true;
				else if (*arg == 'r') recurse = true;
				else if (*arg == 'c') confirm = true;
				else if (*arg == 'S') sup = true;
				else if (*arg == 'a') all = true;
				else if (*arg == 'N') format = Context::Format::None;
				else if (*arg == 'Y') format = Context::Format::Year;
				else if (*arg == 'M') format = Context::Format::Month;
				else if (*arg == 'D') format = Context::Format::Day;
				else if (*arg == 'v') if (verbose) debug = true; else verbose = true;
				// parameter options
				else if (*arg == 'n') pending = &count;
				else if (*arg == 's') pending = &skip;
				else if (*arg == 'i') pending = &prefer;
				else if (*arg == 'x') pending = &avoid;
				else if (*arg == 'f') pending = &keys;
				else if (*arg == 't') pending = &out;
				else if (*arg == 'd') {
					if (dups.empty()) dups = "duplicate.file.log";
					pending = &dups;
				}
				if (pending.index()) {
					set(&pending, ++arg);
					break;				// consume remaining chars
				}
			}
		}
		else if (pending.index()) set(&pending, arg);
		else {
			string dir(arg);
			while (dir.back() == '/') dir.pop_back();
			dirs.push_back(dir);
		}
	}

	while (out.back() == '/') out.pop_back();
	if (dirs.empty()) dirs.push_back(".");
	if (out.empty()) out = dirs.front();
}

Context::Context()
{
	move = recurse = verbose = debug = confirm = force = sup = all = help = false;
	format = Context::Format::Month;
	count = -1L;
	skip = 0;
}
