#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <list>
#include <sys/stat.h>

#include "helper.hpp"
#include "context.hpp"
#include "exif.hpp"

using namespace std;
using namespace filesystem;

Context context;

int main(int n, char** argv) {
    bool help = false;

    unordered_map<string, map<int32_t, list<string>>> dups;

    for (int i = 1; i < n; i++) {
        char* arg = argv[i];
        if (*arg == '-') 
        while (*++arg){
            if (*arg == 'h') { help = true; continue; }
            if (*arg == 'R') { context.move = true; continue; }
            if (*arg == 'c') { context.confirm = true; continue; }
            if (*arg == 's') { context.sup = true; continue; }
            if (*arg == 'v') {
                if (context.verbose) context.debug = true;
                context.verbose = true;
                continue;
            }
            if (*arg == 'Y') { context.format = Context::Format::Year; continue; }
            if (*arg == 'M') { context.format = Context::Format::Month; continue; }
            if (*arg == 'D') { context.format = Context::Format::Day; continue; }
            if (*arg == 'n') { context.count = strtol(++arg, nullptr, 0); break; }
            if (*arg == 'S') { context.size = strtol(++arg, nullptr, 0); break; }
        }
        else {
            context.dir = arg;
            while (context.dir.back() == '/') context.dir.pop_back();
        }
    }

    if (help) cout << R"EOF(options:
-h      display this help message and quit, helpfull to see other argument parsed
-M      move files, dry run overwise
-n      process count number of files
-S      minimum size of a file being moved to overwrite existing file
-f      overwrite target file if exists
-v      be verbose, if repeated be more verbose with debug info
-Y      file path under target directory will be altered to /yyyy/
-M      file path under target directory will be altered to /yyyy/mm/
-D      file path under target directory will be altered to /yyyy/mm/dd/
-c      confirm possible errors

Parsed arguments:
)EOF" << endl;

    cout << context;
    
    if (help) exit(EXIT_SUCCESS);

    if (!exists(context.dir)) {
        cerr << "Directory not found: " << context.dir << endl;
        return 1;
    }

    recursive_directory_iterator iter;
    try { iter = recursive_directory_iterator(context.dir); }
    catch (filesystem_error&) {
        cerr << "Not a directory: " << context.dir << endl;
        return 1;
    }
    for (auto entry: iter) {
        if (!entry.is_regular_file()) continue;
        if (!context.count--) break;
        ifstream ifs(entry.path(), ios::binary);
        if (!ifs) { cerr << "Could not open file: " << entry.path() << endl; continue; }
        File file(entry.path());
        file << ifs;
        if (file) dups[file.date][file.psize].push_back(file.full());
        cout << file << " ... ";

        if (!file.picture) cerr << "not JPEG file";
        else if (!file.exif) cerr << "no EXIF data";
        else if (!file.sub) cerr << "no original camera IDF data";
        else if (file.date.empty()) cerr << "no original date";
        else if (!file.end) cerr << "bad END TAG";
        else if (!strtol(file.year.c_str(), NULL, 10)) cerr << "NULL year";
        else if (!strtol(file.month.c_str(), NULL, 10)) cerr << "NULL month";
        else if (!strtol(file.day.c_str(), NULL, 10)) cerr << "NULL day";

        if (!file) {
            if(context.suppress()) cout << clean;
            else cout << enter;
            continue;
        }

        if (file.target() == entry.path()) {
            cout << "on place";
            if(context.suppress()) cout << clean;
            else cout << enter;
            continue;
        }

        file.move();
    }

    auto date = dups.begin();
    while (date != dups.end()) {
        auto size = date->second.begin();
        while (size != date->second.end())
            if (size->second.size() < 2)
                size = date->second.erase(size);
            else size++;
       if (date->second.empty()) date = dups.erase(date);
       else date++;
    }
    cout << "Duplicate files:" << endl;
    for (auto& date: dups) {
        cout << date.first << endl;
        for (auto& size: date.second) {
            cout << tab << size.first;
            for (auto& name: size.second)
                cout << tab << name;
            }
    }
    return 0;
}

bool File::move() {
    if (context.move) {
        if (!exists(dir)) {
            if (context.confirm) {
                cerr << "Creating file target directory: " << dir << endl;
                confirm();
            }
            if (!filesystem::create_directories(dir)) {
                if (errno != 17) {
                    cerr << "Failed to create directory: " << dir
                        << ", error/" << (int)errno << ": " << strerror(errno) << endl;
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if (!filesystem::is_directory(dir)) {
            cerr << "Path exists and is not a directory: " << dir << endl;
            exit(EXIT_FAILURE);
        }

        if (exists(target())) {
            struct stat info;
            stat(target().c_str(), &info);
            if (context.confirm) {
                bool skip = fsize < info.st_size && !context.force;
                if (skip) cerr << "skipping overwrite/";
                else cerr << "OVERWRITING/";
                cerr << info.st_size / (1<< 10) << "k: " << target() << endl;
                if (skip) return false;
                else confirm();
            }
        }
        rename(full(), target());
    }
    cerr << target() << endl;
    return context.move;
}

ostream& operator<<(ostream& os, const File& file)
{
    os << file.full();
    if (!file.date.empty()) os << tab << "date: " << file.date;
    os << tab << "size: " << file.fsize / (1 << 10) << 'k';
    return os;
}

ifstream& File::operator<<(ifstream& is)
{
    struct stat info;
    stat(full().c_str(), &info);
    fsize = info.st_size;

    uint16_t tag, size;
    uint32_t ref = 0;
    is.read((char*)&tag, sizeof(tag));
    if (tag != JPEG) return is;
    picture = true;
    if (context.debug) cerr << "Tags: ";
    do {
        if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << is.tellg() << tab; return is; }
        if ((tag & 0xFF) != MARK) return is;
        is.read((char*)&size, sizeof(size));
        size = __builtin_bswap16(size);
        if (context.debug) cerr << outhex(tag) << tab;
        if (!ref && tag == EXIF) ref = is.tellg();
        if (tag == SOS) sos = is.tellg();
        if (!is.seekg(size - sizeof(size), ios::cur)) { if (context.verbose) cerr << "Seek error @" << is.tellg() << tab; return is; }
    } while (tag != SOS);
    if (!ref) return is;
    this->psize = fsize - sos;
    exif = true;
    is.seekg(ref + 6L, ios::beg);   // seek to TIFF allign
    string tiff;
    bool swap = false;
    is >> tiff;
    if (!strncmp(tiff.data(), "MM", 2)) swap = true;
    if (context.debug) cerr << "Id/" << boolalpha << swap << ':' << tiff << tab;
    is.seekg(ref + 10L, ios::beg);
    ref +=6L;
    uint32_t offset;
    is.read((char*)&offset, sizeof(offset));
    if (swap) offset = __builtin_bswap32(offset);
    is.seekg(offset - 8, ios::cur);
    is.read((char*)&size, sizeof(size));
    if (swap) size = __builtin_bswap16(size);
    if (context.debug) cerr << "tags/" << outvar(size) << '@' << outvar(offset) << ": ";
    while(size--) {
        if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << is.tellg() << tab; return is; }
        if (swap) tag = __builtin_bswap16(tag);
        if (context.debug) cerr << outhex(tag) << tab;
        if (tag == SIFD) break;
        if (!is.seekg(10, ios::cur)) { if (context.verbose) cerr << "Seek error @" << is.tellg() << tab; return is; }
    };
    if (tag != SIFD) return is;
    sub = true;
    uint16_t format;
    is.read((char*)&format, sizeof(format));
    uint32_t count;
    is.read((char*)&count, sizeof(count));
    is.read((char*)&offset, sizeof(offset));
    if (swap) offset = __builtin_bswap32(offset);

    is.seekg(ref + offset, ios::beg);           // seek to SUB IFD
    is.read((char*)&size, sizeof(size));
    if (swap) size = __builtin_bswap16(size);
    if (context.debug) cerr << "tags/" << outvar(size) << '@' << outvar(offset) << ": ";
    while(size--) {
        if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << is.tellg() << tab; return is; }
        if (swap) tag = __builtin_bswap16(tag);
        if (context.debug) cerr << outhex(tag) << tab;
        if (tag == DATE) break;
        if (!is.seekg(10, ios::cur)) { if (context.verbose) cerr << "Seek error @" << is.tellg() << tab; return is; }
    };
    if (tag != DATE) return is;
    is.read((char*)&format, sizeof(format));
    if (swap) format = __builtin_bswap16(format);
    if (format != 2) return is;
    is.read((char*)&count, sizeof(count));
    if (swap) count = __builtin_bswap32(count);
    is.read((char*)&offset, sizeof(offset));
    if (swap) offset = __builtin_bswap32(offset);
    is.seekg(ref + offset, ios::beg);
    is >> date;
    istringstream iss(date);
    getline(iss, year, ':');
    getline(iss, month, ':');
    getline(iss, day, ':');
    is.get();
    char time[10];
    is.read(time, sizeof(time));
    date += '-' + string(time);


    is.seekg(-2, ios::end);
    if (!is.read((char*)&tag, sizeof(tag))) cerr << "Read error/" << is.tellg() << ':' << *this << endl;
    else if (tag == END) end = true;

    dir = context.dir + '/' + year + '/';
    if (context.format > context.Format::Year) dir += month + '/';
    if (context.format > context.Format::Month) dir += day + '/';

    return is;
}
