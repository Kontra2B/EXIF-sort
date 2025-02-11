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
    if (context.verbose) {
        if (!context.debug) oss << "verbose, ";
        else oss << "debug, ";
    }
    if (context.confirm) oss << "confirm, ";
    if (context.format != Context::Format::None) {
        oss << "path:/yyyy/";
        if (context.format > Context::Format::Year) oss << "mm/";
        if (context.format > Context::Format::Month) oss << "dd/";
        oss << ", ";
    }
    oss << "pid:" << getpid() << endl;
    return oss << endl;
}

Context::Context(): dir(".") {
    verbose = debug = confirm = force = false;
    format = Context::Format::None;
    count = -1L;
    if (dir.back() == '/') dir.pop_back();
}
