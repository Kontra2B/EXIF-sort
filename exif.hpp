#pragma once
#include <ostream>
#include <vector>
#include <filesystem>

using TAG = uint16_t;

const TAG MARK = 0xFF;
const TAG JPEG = 0xD8FF;
const TAG EXIF = 0xE1FF;
const TAG SOS = 0xDAFF;
const TAG END = 0xD9FF;
const TAG SIFD = 0x8769;
const TAG DATE = 0x9003;

struct File {
    bool picture, exif, sub, end;
    std::string name, path, dir, date;
    std::string year, month, day;
    uint32_t sos, psize, fsize;
    std::ifstream& operator<<(std::ifstream&);
    operator bool() const {
        return picture && exif && sub && end
            && !date.empty()
            && strtol(year.c_str(), NULL, 10)
            && strtol(month.c_str(), NULL, 10)
            && strtol(day.c_str(), NULL, 10);
    };
    File(const std::filesystem::path& entry) {
        picture = exif = sub = end = false;
        sos = psize = fsize = 0;
        name = entry.filename();
        path = entry.parent_path();
    }
    friend std::ostream& operator<<(std::ostream&, const File&);
    std::string full() const { return path + '/' + name; }
    std::string target() const { return dir + name; }
    bool move();
};
