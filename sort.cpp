#include <iostream>
#include <fstream>
#include <cstring>
#include <map>

#include "helper.hpp"
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
        }
        else {
            directory_entry entry(arg);
            if (entry.is_regular_file()) context.dir = entry.path().parent_path().native();
            else context.dir = entry.path().native();
        }
    }

    if (help) cout << R"EOF(options:
-h      display this help message and quit, helpfull to see other argument parsed
-M      move files, dry run overwise
-f      overwrite target file if exists
-v      be verbose, if repeated be more verbose with debug info
-Y      file path under target directory will be altered to /yyyy/
-M      file path under target directory will be altered to /yyyy/mm/
-D      file path under target directory will be altered to /yyyy/mm/dd/
        based on file original modifaction time, useful for media files recovery
-c      confirm possible errors

Parsed arguments:
)EOF" << endl;

    cout << context;
    
    if (help) exit(EXIT_SUCCESS);

    if (!exists(context.dir)) {
        cerr << "wrong file or directory" << endl;
        return 1;
    }

    directory_entry entry(context.dir);
    recursive_directory_iterator iter;
    try { iter = recursive_directory_iterator(entry); }
    catch (filesystem_error& e) {}
    while (true) {
        if (iter != end(iter)) entry = *iter++;
        if (!entry.is_regular_file()) continue;
        if (!context.count--) break;
        ifstream ifs(entry.path(), ios::binary | ios::ate);
        if (!ifs) { cerr << "Could not open file: " << entry.path() << endl; continue; }
        File file(entry);
        file << ifs;
        // cout << file.path + '/' + file.name << "..." << tab;
        cout << file << " ... " << tab;
        bool skip = true;
        if (!file.picture) cout << "not JPEG file";
        else if (!file.exif) cout << "no EXIF data";
        else if (!file.sub) cout << "no original camera IDF data";
        else if (file.date.empty()) cout << "no original date";
        else skip = false;

        if (skip) {
            if(context.sup) cout << clean;
            else cout << endl;
            continue;
        }

        string dir = context.dir + '/' + file.year + '/';
        if (context.format > Context::Format::Year) dir += file.month + '/';
        if (context.format > Context::Format::Month) dir += file.day + '/';
        string target = dir + file.name;
        if (target == entry.path()) {
            cout << "already on place";
            if(context.sup) cout << clean;
            else cout << endl;
            continue;
        }

        if (context.move) {
            if (!exists(dir)) {
                if (context.verbose) cerr << "Creating file target directory: " << dir << endl;
                if (!filesystem::create_directories(dir)) {
                    if (errno != 17) {
                        cerr << "Failed to create directory: " << dir
                            << ", error/" << (int)errno << ": " << strerror(errno) << endl;
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else if (!filesystem::is_directory(dir)) {
                cerr << "The dir exists and is not a directory: " << dir << endl;
                exit(EXIT_FAILURE);
            }

            rename(entry.path(), target);
        }
        cerr << target << endl;
        if (iter == end(iter)) break;
    }
    return 0;
}

ostream& operator<<(ostream& os, const File& file)
{
    os << file.path << '/' << file.name;
    if (!file.date.empty()) os << tab << "date: " << file.date << tab
            << file.year << '/' << file.month << '/' << file.day << tab
                << "time: " << file.time;
    if (file) os << tab << "size: " << (file.size - file.sos) / (1 << 10) << 'k';
    return os;
}

ifstream& File::operator<<(ifstream& is)
{
    uint16_t tag, size;
    this->size = is.tellg();
    is.seekg(0);
    uint32_t ref = 0;
    is.read((char*)&tag, sizeof(tag));
    if (tag != JPEG) { if (Context::verbose) cout << "Not JPEG file: " << outvar(tag) << endl; return is; }
    picture = true;
    if (Context::debug) cerr << "Tags: ";
    do {
        is.read((char*)&tag, sizeof(tag));
        is.read((char*)&size, sizeof(size));
        size = __builtin_bswap16(size);
        if (Context::debug) cerr << outhex(tag) << tab;
        if (!ref && tag == EXIF) ref = is.tellg();
        if (tag == SOS) sos = is.tellg();
        is.seekg(size - sizeof(size), ios::cur);
    } while (tag != SOS);
    if (!ref) { if (Context::verbose) cerr << "EXIF marker NOT found" << endl; return is; }
    exif = true;
    is.seekg(ref + 6L, ios::beg);   // seek to TIFF allign
    string tiff;
    bool swap = false;
    is >> tiff;
    if (!strncmp(tiff.data(), "MM", 2)) swap = true;
    if (Context::debug) cerr << "Id/" << boolalpha << swap << ':' << tiff << tab;
    is.seekg(ref + 10L, ios::beg);
    ref +=6L;
    uint32_t offset;
    is.read((char*)&offset, sizeof(offset));
    if (swap) offset = __builtin_bswap32(offset);
    is.seekg(offset - 8, ios::cur);
    is.read((char*)&size, sizeof(size));
    if (swap) size = __builtin_bswap16(size);
    if (Context::debug) cerr << "tags/" << outvar(size) << '@' << outvar(offset) << ": ";
    while(size--) {
        if (!is.read((char*)&tag, sizeof(tag))) { if (Context::debug)cerr << "Read error: " << is.tellg() << endl; return is; }
        if (swap) tag = __builtin_bswap16(tag);
        if (Context::debug) cerr << outhex(tag) << tab;
        if (tag == SIFD) break;
        is.seekg(10, ios::cur);
    };
    if (tag != SIFD) { if (Context::verbose) cerr << "No original camera section" << endl; return is; }
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
    if (Context::debug) cerr << "tags/" << outvar(size) << '@' << outvar(offset) << ": ";
    while(size--) {
        if (!is.read((char*)&tag, sizeof(tag))) { if (Context::verbose) cerr << "Read error: " << is.tellg() << endl; return is; }
        if (swap) tag = __builtin_bswap16(tag);
        if (Context::debug) cerr << outhex(tag) << tab;
        if (tag == DATE) break;
        is.seekg(10, ios::cur);
    };
    if (tag != DATE) { if (Context::verbose) cerr << "No original DATE in camera section" << endl; return is; }
    is.read((char*)&format, sizeof(format));
    if (swap) format = __builtin_bswap16(format);
    if (format != 2) { if (Context::verbose) cerr << "Wrong date format: " << format << endl; return is; }
    is.read((char*)&count, sizeof(count));
    if (swap) count = __builtin_bswap32(count);
    is.read((char*)&offset, sizeof(offset));
    if (swap) offset = __builtin_bswap32(offset);
    is.seekg(ref + offset, ios::beg);
    is >> date;
    is.get();
    char time[10];
    is.read(time, sizeof(time));
    this->time = time;

    istringstream iss(date);
    getline(iss, year, ':');
    getline(iss, month, ':');
    getline(iss, day, ':');

    return is;
}
