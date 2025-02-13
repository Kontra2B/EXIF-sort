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
    oss << "Directory:" << context.dir << ", ";
    if (context.count > 0) oss << "count:" << context.count << ", ";
    if (context.size > 0) oss << "size:" << context.size << "k, ";
    if (context.verbose) {
        if (!context.debug) oss << "verbose, ";
        else oss << "debug, ";
    }
    if (context.sup) oss << "suppress, ";
    if (context.confirm) oss << "confirm, ";
	oss << "path:/yyyy/";
	if (context.format > Context::Format::Year) oss << "mm/";
	if (context.format > Context::Format::Month) oss << "dd/";
	oss << ", ";
    oss << "pid:" << getpid() << endl;
	if (context.move) oss << "MOVING files" << endl;
    return oss << endl;
}

Context::Context(): dir(".") {
    move = verbose = debug = confirm = force = sup = false;
    format = Context::Format::Month;
    count = -1L;
    size = 0L;
}
