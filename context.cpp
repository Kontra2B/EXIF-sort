#include <iostream>
#include <sstream>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>

#include "helper.hpp"
#include "context.hpp"

bool Context::verbose = false;
bool Context::debug = false;
bool Context::confirm = false;

using namespace std;

ostream& operator<<(ostream& oss, const Context& context) {
    oss << "Working directory:" << context.dir << ", ";
    oss << "output directory:" << context.out << ", ";
    if (context.dups) oss << "save duplicate list, ";
    if (context.count > 0) oss << "count:" << context.count << ", ";
    if (context.skip > 0) oss << "skip:" << context.skip << ", ";
    if (context.verbose) {
        if (context.debug) oss << "debug, ";
        else oss << "verbose, ";
    }
    if (context.sup) oss << "suppress, ";
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
    move = verbose = debug = confirm = force = sup = dups = all = false;
    dir.push_back('.');
    out.push_back('.');
    format = Context::Format::Month;
    count = -1L;
    skip = 0;
}
