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
        if (*arg == '-') while (*++arg) {
            if (*arg == 'h') help = true;
            else if (*arg == 'R') context.move = true;
            else if (*arg == 'c') context.confirm = true;
            else if (*arg == 's') context.sup = true;
            else if (*arg == 'v') if (context.verbose) context.debug = true; else context.verbose = true;
            else if (*arg == 'Y') context.format = Context::Format::Year;
            else if (*arg == 'M') context.format = Context::Format::Month;
            else if (*arg == 'D') context.format = Context::Format::Day;
            else if (*arg == 'n') { context.count = strtol(++arg, nullptr, 0); break; }
            else if (*arg == 'x') { context.skip = strtol(++arg, nullptr, 0); break; }
            else if (*arg == 'S') { context.size = strtol(++arg, nullptr, 0); break; }
        }
        else {
            context.dir = arg;
            while (context.dir.back() == '/') context.dir.pop_back();
        }
    }

    if (help) cout << R"EOF(options:
-h      display this help message and quit, helpfull to see other argument parsed
-M      move files, dry run overwise
-n      process number of files
-x      skip number of files
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
    int i = 0;
    for (auto entry: iter) {
        if (!entry.is_regular_file()) continue;
        i++;
        if (context.skip && context.skip--) continue;
        if (!context.count--) break;
        ifstream ifs(entry.path(), ios::binary);
        if (!ifs) { cerr << "Could not open file: " << entry.path() << endl; continue; }
        File file(entry.path());
        file << ifs;
        if (file) dups[file.date][file.psize].push_back(file.full());
        cout << i << ". " << file;

        if (!file) {
            if(context.suppress()) cout << clean;
            else cout << enter;
            continue;
        }

        if (file.target() == file.full()) {
            cout << tab << "on place";
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
    ofstream of("duplicate.files.log");
    of << "Duplicate files:" << endl;
    for (auto& date: dups) {
        of << date.first;
        for (auto& size: date.second) {
            of << tab << size.first / (1 << 10) << 'k';
            for (auto& name: size.second)
                of << tab << name;
            }
        of << endl;
    }
    return 0;
}

bool File::move() {
    cout << " ... " << tab;
    if (context.move) {
        if (!exists(dir)) {
            if (context.verbose || context.confirm) cerr << "Creating file target directory: " << dir << endl;
            confirm();
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
            filesystem::path exist(target());
            ifstream ifs(exist, ios::binary);
            File file(exist);
            if (ifs) file << ifs;

            bool skip = file > *this;
            if (skip) cout << "skipping: " << file;
            else cerr << *this << tab << "OVERWRITING: " << file << endl;
            if (skip) return false;
            else confirm();
        }
        rename(full(), target());
    }
    cout << target() << enter;
    return context.move;
}

ostream& operator<<(ostream& os, const File& file)
{
    if (!file) os << "! ";
    os << file.full();
    if (!file.date.empty()) os << " date: " << file.date;
    os << " size: " << file.fsize / (1 << 10) << 'k';
    if (file.res) os << " res: " << file.width << '/' << file.hight;
    if (!file) os << tab;
    if (!file.picture) os << "not JPEG file";
    else if (!file.exif) os << "no EXIF data";
    else if (!file.sub) os << "no original camera IDF data";
    else if (file.date.empty()) os << "no original date";
    else if (!file.end) os << "bad END TAG";
    else if (!strtol(file.year.c_str(), NULL, 10)) os << "NULL year";
    else if (!strtol(file.month.c_str(), NULL, 10)) os << "NULL month";
    else if (!strtol(file.day.c_str(), NULL, 10)) os << "NULL day";
    return os;
}

bool File::operator>(const File& file)
{
    if (*this && !file) return true;
    if (width * hight > file.width * file.hight) return true;
    return hight > file.hight;
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
    if (context.debug) cerr << endl << "APP:";
    do {
        auto pos = is.tellg();
        if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << pos << tab; return is; }
        if ((tag & 0xFF) != MARK) return is;
        is.read((char*)&size, sizeof(size));
        size = __builtin_bswap16(size);
        if (context.debug) { cerr << ' ' << outhex(__builtin_bswap16(tag)) << '@' << outhex(pos); if (tag == EXIF) cerr << '!'; }
        if (!ref && tag == EXIF) ref = is.tellg();
        if (tag == SOS) psize = fsize - is.tellg();
        if (tag == SOF) sof = is.tellg();
        if (!is.seekg(size - sizeof(size), ios::cur)) { if (context.verbose) cerr << "Seek error @" << is.tellg() << tab; return is; }
    } while (tag != SOS);
    if (!ref) return is;
    char id[4];
    is.seekg(ref, ios::beg);
    is.read(id, 4);
    if (strncmp(id, EXIFID, 4)) return is;
    exif = true;
    is.seekg(2, ios::cur);
    char tiff[2];
    bool swap = false;
    is.read(tiff, 2);
    if (!strncmp(tiff, "MM", 2)) swap = true;
    if (context.debug) cerr << ' ' << "Id" << ':' << tiff;
    if (swap ) cerr << "/swap";
    is.seekg(ref + 10L, ios::beg);
    ref +=6L;
    uint32_t offset;
    is.read((char*)&offset, sizeof(offset));
    if (swap) offset = __builtin_bswap32(offset);
    is.seekg(offset - 8, ios::cur);
    is.read((char*)&size, sizeof(size));
    if (swap) size = __builtin_bswap16(size);
    if (context.debug) cerr << endl << "EXIF/" << outvar(size) << ':';
    while(size--) {
        auto pos = is.tellg();
        if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << pos << tab; return is; }
        if (swap) tag = __builtin_bswap16(tag);
        if (context.debug) { cerr << ' ' << outhex(tag) << '@' << outhex(pos); if (tag == SIFD) cerr << '!'; }
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
    if (context.debug) cerr << endl << "SIFD/" << outvar(size) << '@' << outhex(offset) << ':';
    uint32_t pdate(0), pwidth(0), phight(0);
    while(size--) {
        auto pos = is.tellg();
        if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << pos << tab; return is; }
        if (swap) tag = __builtin_bswap16(tag);
        if (context.debug) { cerr << ' ' << outhex(tag) << '@' << outhex(pos); if (tag == DATE) cerr << '!'; }
        if (tag == DATE) pdate = is.tellg();
        if (tag == WIDTH) pwidth = is.tellg();
        if (tag == HIGHT) phight = is.tellg();
        if (!is.seekg(10, ios::cur)) { if (context.verbose) cerr << "Seek error @" << is.tellg() << tab; return is; }
    }
    cerr << endl;
    if (pdate) {
        is.seekg(pdate);
        is.read((char*)&format, sizeof(format));
        if (swap) format = __builtin_bswap16(format);
        if (format != 2) return is;
        is.read((char*)&count, sizeof(count));
        if (swap) count = __builtin_bswap32(count);
        is.read((char*)&offset, sizeof(offset));
        if (swap) offset = __builtin_bswap32(offset);
        auto pos = ref + offset;
        is.seekg(pos, ios::beg);
        is >> date;
        istringstream iss(date);
        getline(iss, year, ':');
        getline(iss, month, ':');
        getline(iss, day, ':');
        is.get();
        char time[10];
        is.read(time, sizeof(time));
        date += '-' + string(time);
        if (context.debug) cerr << "Date" << '@' << outhex(pos) << ": " << date;
    }
    res = pwidth && phight;
    if (pwidth) {
        is.seekg(pwidth);
        is.read((char*)&format, sizeof(format));
        if (swap) format = __builtin_bswap16(format);
        is.read((char*)&count, sizeof(count));
        if (swap) count = __builtin_bswap32(count);
        is.read((char*)&width, sizeof(width));
        if (swap) width = __builtin_bswap32(width);
        if (format == 3) width = width >> 16;
        if (context.debug) cerr << tab << "width" << '@' << outhex(pwidth + 6) << ": " << outvar(width);
    }
    if (phight) {
        is.seekg(phight);
        is.read((char*)&format, sizeof(format));
        if (swap) format = __builtin_bswap16(format);
        is.read((char*)&count, sizeof(count));
        if (swap) count = __builtin_bswap32(count);
        is.read((char*)&hight, sizeof(hight));
        if (swap) hight = __builtin_bswap32(hight);
        if (format == 3) hight = hight >> 16;
        if (context.debug) cerr << tab << "hight" << '@' << outhex(phight + 6) << ": " << outvar(hight) << enter;
    }
    
    is.seekg(-2, ios::end);
    if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << is.tellg() << tab; }
    else if (tag == END) end = true;

    dir = context.dir + '/' + year + '/';
    if (context.format > context.Format::Year) dir += month + '/';
    if (context.format > context.Format::Month) dir += day + '/';

    return is;
}
