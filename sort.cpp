#include <iostream>
#include <fstream>
#include <filesystem>

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
        for (const auto& dirEntry: recursive_directory_iterator(context.dir)) {
            if (!dirEntry.is_regular_file()) continue;
            ifstream stream(dirEntry.path());
            File file(move(stream));
            stream >> file;
            auto exif = file.find();
            if (!exif) continue;
            cout << "File: " << dirEntry.path() << ':' << tab << exif << endl;
            auto tiff = exif->tiff;
            cerr << tiff << endl;
        }
    } catch (exception e) {
        cerr << "try running as root" << endl;
        return 1;
    }
    return 0;
}

istream& operator>>(istream& is, File& file)
{
    TAG tag; is >> tag;
    if (tag != JPEG) { cerr << "Not JPEG file: " << hex << 'x' << tag << endl; return is; }
    union {
        uint16_t size;
        struct {
            uint8_t hsize;
            uint8_t lsize;
        };
    } size;
    is >> tag >> size.hsize >> size.lsize;
    cerr << "Section: " << outpaix(tag, size.size) << endl;
    if (tag != EXIF) { cerr << "Not EXIF\n"; return is; }
    return is; 
}

Jpeg::operator bool() const { return tag == JPEG; }

Tag::operator bool() const { return tag == EXIF; }

const Tag* File::find()
{
    file.read(buffer.data(), buffer.size());
    auto* jpeg = reinterpret_cast<const Jpeg*>(buffer.data());
    if (!*jpeg) cerr << "NOT JPEG FILE" << endl;
    uint64_t exifOffset = 0;
    const Tag* exif = nullptr;
    size_t size = offsetof(Jpeg, markers);
    const auto* marker = jpeg->markers;
    while ((marker->tag & 0xFF) == 0xFF) {
        auto tagSize = littleEndian(marker->size);
        auto tagOffset = (char*) marker - buffer.data();
        size += sizeof(marker->tag) + tagSize + offsetof(Tag, size) + sizeof(marker->size);
        size_t alloc = ((size - 1)/sector + 1) * sector;
        int64_t more = alloc - buffer.size();
        if (more > 0) {
            auto old = buffer.size();
            buffer.resize(alloc);
            file.read(buffer.data() + old, more);
            marker = reinterpret_cast<const Tag*>(buffer.data() + tagOffset);
        }
        if (*marker) exifOffset = tagOffset;
        if (!marker->size) break;
        // cerr << "Buffer size:" << outpaix(buffer.size(), size) << tab;
        if (Context::verbose) cerr << marker << endl;
        if (marker->tag == SOS) break;
        marker = reinterpret_cast<const Tag*>((char*)marker + tagSize + offsetof(Tag, size));
    }
    if (exifOffset) exif = reinterpret_cast<const Tag*>(buffer.data() + exifOffset);
    return exif;
}

ostream& operator<<(ostream& os, const Entry& entry)
{
    os << "tag: " << 'x' << hex << uppercase << entry.tag << tab
        << "format: " << entry.format << tab
        << "count: " << outvar(entry.count) << tab;
    ldump(&entry, sizeof(Entry));
        os << endl;
    return os;
}

ostream& operator<<(ostream& os, const Dir* dir)
{
    return os << "Dir count: " << outvar(dir->count) << endl;
}

ostream& operator<<(ostream& os, const Tiff* tiff)
{
    os << "Tiff ident:" << tiff->ident << tab
        << outvar(tiff->nulls) << tab;
    outwrite(os, tiff->allign)
        << tab << hex << tiff->fixed << tab
        << outvar(tiff->offset);
    if (ldump(tiff, sizeof(Tiff))) os << endl;
    auto dir = reinterpret_cast<const Dir*>((char*)tiff->allign + tiff->offset);
    os << dir << endl;
    int i = dir->count;
    auto entry = dir->entries;
    while (i--) {
        const char* data = (char*)entry->data;
        if (entry->count > sizeof(entry->data)) data = (char*)tiff->allign + entry->data;
        os << "tag: " << 'x' << hex << uppercase << entry->tag << tab
            << "format: " << outvar(entry->format) << tab
            << "count: " << outvar(entry->count) << tab;
        if (entry->format == 2) os << data;
        else ldump(data, entry->count);
        os << endl;
        entry++;
    }
    return os;
}

ostream& operator<<(ostream& os, const Tag* tag)
{
    os << "Marker tag:" << uppercase << 'x' << hex << tag->tag << tab
        << "size:" << outvar(littleEndian(tag->size)) << tab;
    if (ldump(tag, sector)) os << endl;
    // if (ldump(tag, littleEndian(tag->size) + offsetof(Tag, size))) os << endl;
    return os;
}
