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
    bool help(false), pending(false);

    unordered_map<string, list<File>> dups;

    for (int i = 1; i < n; i++) {
        char* arg = argv[i];
        if (*arg == '-') {
            pending = false;
            while (*++arg) {
                if (*arg == 'h') help = true;
                else if (*arg == 'M') context.move = true;
                else if (*arg == 'c') context.confirm = true;
                else if (*arg == 's') context.sup = true;
                else if (*arg == 'd') context.dups = true;
                else if (*arg == 'a') context.all = true;
                else if (*arg == 'v') if (context.verbose) context.debug = true; else context.verbose = true;
                else if (*arg == 'Y') context.format = Context::Format::Year;
                else if (*arg == 'M') context.format = Context::Format::Month;
                else if (*arg == 'D') context.format = Context::Format::Day;
                else if (*arg == 'n') { context.count = strtol(++arg, nullptr, 0); break; }
                else if (*arg == 'x') { context.skip = strtol(++arg, nullptr, 0); break; }
                else if (*arg == 't') {
                    if (*++arg) context.out = arg;
                    else pending = true;
                    break;
                }
            }
        }
        else if (pending) context.out = arg;
        else context.dir = context.out = arg;
    }

    if (help) cout << R"EOF(./exif.sort DIR [OPTIONS]

OPTIONS:
-h      display this help message and quit, helpfull to see other argument parsed
-M      move files, dry run otherwise, only across one filesystem
-a      move all files, otherwise jpeg pictures only
-d      create list of duplicate pictures
-n      number of files to process
-x      number of files to skip
-t      target directory, DIR overwrites target directory
-v      be verbose, if repeated be more verbose with debug info
-Y      file path under target directory will be altered to /yyyy/
-M      file path under target directory will be altered to /yyyy/mm/
-D      file path under target directory will be altered to /yyyy/mm/dd/
-c      confirm possible errors

Parsed arguments:
)EOF" << endl;

	while (context.out.back() == '/') context.out.pop_back();
	while (context.dir.back() == '/') context.dir.pop_back();
    cout << context;
    
    if (help) exit(EXIT_SUCCESS);

    if (!exists(context.dir)) {
        cerr << "Directory not found: " << context.dir << endl;
        return 1;
    }

	confirm();

    recursive_directory_iterator iter;
    try { iter = recursive_directory_iterator(context.dir); }
    catch (filesystem_error&) {
        cerr << "Not a directory: " << context.dir << endl;
        return 2;
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
        cerr << i << ". ";
        file << ifs;		// create file object from disk file
        cout << file;

        if (file.target() == file.full()) {
            cout << tab << "on place";
            if(context.suppress()) cout << clean;
            else cout << enter;
            continue;
        }

        file.move();

        if (context.dups) {
            auto& list = dups[file.date];
            auto it = list.begin();
            while (it != list.end())
                if (file > *it) {
                    list.insert(it, file);
                    break;
                } else it++;
            if (it == list.end()) list.push_back(file);
        }
    }

    if (context.dups) {
        auto date = dups.begin();
        while (date != dups.end()) {
            if (date->second.size() < 2)
                date = dups.erase(date);
            else date++;
        }
        ofstream of("duplicate.files.log");
        // of << "Duplicate files:" << endl;
        for (auto& date: dups) {
                auto it = date.second.cbegin();
                auto best = *it++;
                while (it != date.second.end())
                    of << *it++ << tab << '#' << tab << best << endl;
            }
    }
    return 0;
}

bool File::move()
{
    if (picture || context.all) {
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
                if (skip) cout << "skipping: " << file << enter;
                else cerr << *this << tab << "OVERWRITING: " << file << endl;
                if (skip) return false;
                else confirm();
            }
            rename(full(), target());
        }
        cout << target() << enter;
    } else cout << enter;
    return context.move;
}

ostream& operator<<(ostream& os, const File& file)
{
    os << file.full();
    if (!file) os << " * ";
    if (!file.date.empty()) os << tab << file.date;
    os << tab << file.size; //  / (1 << 10) << 'k';
    if (file.res) os << tab << file.width << '/' << file.hight;
    if (file.exif) os << '/' << file.ornt;
    if (!file) os << tab;
    if (!file.picture) os << "not JPEG file";
    else if (!file.exif) os << "no EXIF data";
    else if (!file.sos) os << "no data stream marker";
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
    if (!*this) return false;
    if (!file) return true;
    if (width * hight < file.width * file.hight) return false;
    if (ornt > 1 && file.ornt <= 1) return true;
    if (hight < file.hight) return false;
    return size < file.size;
}

ifstream& File::operator<<(ifstream& is)
{
    struct stat info;
    stat(full().c_str(), &info);
    size = info.st_size;
    char buffer[20];  // YYYY-MM-DD HH:MM:SS = 19 znaków + null
    std::tm tm_date;
    localtime_r(&info.st_mtime, &tm_date);
    std::strftime(buffer, sizeof(buffer), "%Y:%m:%d %H:%M:%S", &tm_date);
    date = string(buffer);
    uint16_t tag, size;
    is.read((char*)&tag, sizeof(tag));
    if (tag == JPEG) {
        uint32_t sof(0), ref(0);
        picture = true;
        if (context.debug) cerr << "APP:";
        do {
            auto pos = is.tellg();
            if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << pos << tab; return is; }
            if ((tag & 0xFF) != MARK) break;
            is.read((char*)&size, sizeof(size));
            size = __builtin_bswap16(size);
            if (context.debug) cerr << ' ' << outhex(__builtin_bswap16(tag)) << '@' << outhex(pos)
                << (tag == EXIF || tag == SOF? "*": "");
            if (!ref && tag == EXIF) ref = is.tellg();
            if (tag == SOS) sos = true;
            if (tag == SOF) sof = is.tellg();
            if (!is.seekg(size - sizeof(size), ios::cur)) { if (context.verbose) cerr << "Seek error @" << is.tellg() << tab; return is; }
        } while (tag != SOS);
        if (context.debug) cerr << enter;
        if (ref) { 
            char id[4];
            is.seekg(ref, ios::beg);
            is.read(id, 4);
            if (!strncmp(id, EXIFID, 4)) {
                exif = true;
                is.seekg(2, ios::cur);
                char tiff[4] = {};
                bool swap = false;
                is.read(tiff, 2);
                if (!strncmp(tiff, "MM", 2)) swap = true;
                is.seekg(ref + 10L, ios::beg);
                ref +=6L;
                uint32_t offset;
                is.read((char*)&offset, sizeof(offset));
                if (swap) offset = __builtin_bswap32(offset);
                is.seekg(offset - 8, ios::cur);
                is.read((char*)&size, sizeof(size));
                if (swap) size = __builtin_bswap16(size);
                if (context.debug) cerr << "EXIF/" << outvar(size) << ','
                    << "Id" << ':' << string(tiff) << (swap? "/swap": "") << ':';
                uint16_t format;
                uint32_t count;
                while(size--) {
                    auto pos = is.tellg();
                    if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << pos << tab; return is; }
                    if (swap) tag = __builtin_bswap16(tag);
                    if (context.debug) cerr << ' ' << outhex(tag) << '@' << outhex(pos) << (tag == SIFD? "*": "");
                    if (tag == ORNT) {
                        is.read((char*)&format, sizeof(format));
                        is.read((char*)&count, sizeof(count));
                        is.read((char*)&ornt, sizeof(ornt));
                        if (swap) ornt = __builtin_bswap16(ornt);
                        if (!is.seekg(2, ios::cur)) { if (context.verbose) cerr << "Seek error @" << is.tellg() << tab; return is; }
                        continue;
                    }
                    if (tag == SIFD) break;
                    if (!is.seekg(10, ios::cur)) { if (context.verbose) cerr << "Seek error @" << is.tellg() << tab; return is; }
                };
                if (context.debug) cerr << enter;
                if (tag == SIFD) {
                    is.read((char*)&format, sizeof(format));
                    is.read((char*)&count, sizeof(count));
                    is.read((char*)&offset, sizeof(offset));
                    if (swap) offset = __builtin_bswap32(offset);

                    is.seekg(ref + offset, ios::beg);           // seek to SUB IFD
                    is.read((char*)&size, sizeof(size));
                    if (swap) size = __builtin_bswap16(size);
                    sub = true;
                    if (context.debug) cerr << "SIFD/" << outvar(size) << '@' << outhex(offset) << ':';
                    uint32_t pdate(0), pwidth(0), phight(0);
                    while(size--) {
                        auto pos = is.tellg();
                        if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << pos << tab; return is; }
                        if (swap) tag = __builtin_bswap16(tag);
                        if (context.debug) cerr << ' ' << outhex(tag) << '@' << outhex(pos) << (tag == DATE? "*":"");
                        if (tag == DATE) pdate = is.tellg();
                        if (tag == WIDTH) pwidth = is.tellg();
                        if (tag == HIGHT) phight = is.tellg();
                        if (!is.seekg(10, ios::cur)) { if (context.verbose) cerr << "Seek error @" << is.tellg() << tab; return is; }
                    }
                    if (context.debug) cerr << enter;
                    if (pdate) {
                        is.seekg(pdate);
                        is.read((char*)&format, sizeof(format));
                        if (swap) format = __builtin_bswap16(format);
                        if (format == 2) {
                            is.read((char*)&count, sizeof(count));
                            if (swap) count = __builtin_bswap32(count);
                            is.read((char*)&offset, sizeof(offset));
                            if (swap) offset = __builtin_bswap32(offset);
                            auto pos = ref + offset;
                            is.seekg(pos, ios::beg);
                            is >> date;
                            is.get();
                            char time[10];
                            is.read(time, sizeof(time));
                            date += '-' + string(time);
                            if (context.debug) cerr << "Date" << '@' << outhex(pos) << ": " << date;
                        }
                    }
                }
            }
        }

        if (sof) {
            is.seekg(++sof);
            is.read((char*)&hight, sizeof(hight));
            hight = __builtin_bswap16(hight);
            if (context.debug) cerr << tab << "hight" << '@' << outhex(sof) << ": " << outvar(hight) << tab;
            is.read((char*)&width, sizeof(width));
            width = __builtin_bswap16(width);
            if (context.debug) cerr << tab << "width" << '@' << outhex(sof + 2) << ": " << outvar(width) << enter;
            res = sof;
        }

        is.seekg(-2, ios::end);
        if (!is.read((char*)&tag, sizeof(tag))) { if (context.verbose) cerr << "Read error @" << is.tellg() << tab; }
        else if (tag == END) end = true;
    }

    istringstream iss(date);
    getline(iss, year, ':');
    getline(iss, month, ':');
    getline(iss, day, ':');

    dir = context.out + '/' + year + '/';
    if (context.format > context.Format::Year) dir += month + '/';
    if (context.format > context.Format::Month) dir += day + '/';

    return is;
}
