#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>

#include "context.hpp"
#include "exif.hpp"

using namespace std;
using namespace filesystem;

int main(int n, char** argv) {
    bool help = false;

    Context context;

    for (int i = 1; i < n; i++) {
        char* arg = argv[i];
        if (*arg == '-') 
        while (*++arg){
            if (*arg == 'h') { help = true; continue; }
            if (*arg == 'c') { context.confirm = true; continue; }
            if (*arg == 'v') {
                if (context.verbose) context.debug = true;
                context.verbose = true;
                continue;
            }
            if (*arg == 'Y') { context.format = Context::Format::Year; continue; }
            if (*arg == 'M') { context.format = Context::Format::Month; continue; }
            if (*arg == 'D') { context.format = Context::Format::Day; continue; }
            // if (*arg == 'n') { context.count = strtol(++arg, nullptr, 0); break; }
        }
        else context.dir = arg;
    }

    if (help) cout << R"EOF(options:
-h      display this help message and quit, helpfull to see other argument parsed
-f      overwrite target file if exists
-v      be verbose, if repeated be more verbose with debug info
-Y      file path under target directory will be altered to /yyyy/
-M      file path under target directory will be altered to /yyyy/mm/
-D      file path under target directory will be altered to /yyyy/mm/dd/
        based on file original modifaction time, useful for media files recovery
-c      confirm possible errors

Parsed arguments:
)EOF" << endl;

    cerr << context;
    
    if (help) exit(EXIT_SUCCESS);

    if (!exists(context.dir)) {
        cerr << "wrong directory" << endl;
        return 1;
    }

    try {
        for (const auto& entry: recursive_directory_iterator(context.dir)) {
            if (!entry.is_regular_file()) continue;
            ifstream ifs(entry.path(), ios::binary | ios::ate);
            if (!ifs) { cerr << "Could not open file: " << entry.path() << endl; continue; }
            File file(entry.path());
            file << ifs;
            cerr << file << endl;
        }
    } catch (exception e) {
        cerr << "try running as root" << endl;
        return 1;
    }
    return 0;
}

ostream& operator<<(ostream& os, const File& file)
{
    os << "File: " << file.name << tab;
    if (file) os << "date: " << file.date << tab
        << "time: " << file.time << tab
            << "size: " << file.size - file.sos;
    return os;
}

ostream& File::operator<<(ifstream& is)
{
    uint16_t tag, size;
    this->size = is.tellg();
    is.seekg(0);
    uint32_t ref = 0;
    is.read((char*)&tag, sizeof(tag));
    if (tag != JPEG) return cerr << "Not JPEG file: " << outvar(tag) << endl;
    cerr << "Tags: ";
    do {
        is.read((char*)&tag, sizeof(tag));
        is.read((char*)&size, sizeof(size));
        size = outswap(size);
        cerr << outhex(tag) << tab;
        if (!ref && tag == EXIF) ref = is.tellg();
        if (tag == SOS) sos = is.tellg();
        is.seekg(size - sizeof(size), ios::cur);
    } while (tag != SOS);
    if (!ref) return cerr << "Exif marker NOT found" << endl;
    is.seekg(ref, ios::beg);
    ref = is.tellg() + 6L;
    is.seekg(10, ios::cur);
    uint32_t offset;
    is.read((char*)&offset, sizeof(offset));
    is.seekg(offset - 8, ios::cur);
    is.read((char*)&size, sizeof(size));
    cerr << "tags/" << outvar(size) << ": ";
    while(size--) {
        if (!is.read((char*)&tag, sizeof(tag))) return cerr << "Read error: " << is.tellg(); else
        cerr << outhex(tag) << tab;
        if (tag == DATE) break;
        is.seekg(10, ios::cur);
    };
    uint16_t format;
    is.read((char*)&format, sizeof(format));
    if (format != 2) return cerr << "Wrong date format: " << format << endl;
    uint32_t count;
    is.read((char*)&count, sizeof(count));
    is.read((char*)&offset, sizeof(offset));
    is.seekg(ref + offset, ios::beg);
    is >> date;
    is.get();
    string time(count - date.size() - 1, 0);
    is.read(time.data(), time.size());
    this->time = time;
    picture = true;
    return cerr;
}
