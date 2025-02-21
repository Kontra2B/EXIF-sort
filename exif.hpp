#pragma once
#include <ostream>
#include <vector>
#include <filesystem>

using TAG = uint16_t;

const TAG MARK = 0xFF;

const TAG JPEG = 0xD8FF;

const TAG EXIF = 0xE1FF;
const TAG SOF = 0xC0FF;
const TAG SOS = 0xDAFF;
const TAG END = 0xD9FF;

const TAG ORNT = 0x0112;
const TAG SIFD = 0x8769;

const TAG DATE = 0x9003;
const TAG WIDTH = 0xA002;
const TAG HIGHT = 0xA003;
const char EXIFID[] = "Exif";

struct File {
    bool picture, exif, sos, sub, res, end;
    std::string name, path, dir, date;
    std::string year, month, day;
    uint32_t size;
    uint16_t hight, width, ornt;
    std::ifstream& operator<<(std::ifstream&);
    operator bool() const {
        return picture && exif && sos && sub && end
            && !date.empty()
            && strtol(year.c_str(), NULL, 10)
            && strtol(month.c_str(), NULL, 10)
            && strtol(day.c_str(), NULL, 10);
    };
    File(const std::filesystem::path& entry) {
        picture = exif = sos = sub = end = res = false;
        size = width = hight = ornt = 0;
        name = entry.filename();
        path = entry.parent_path();
    }
    bool operator>(const File&);
    friend std::ostream& operator<<(std::ostream&, const File&);
    std::string full() const { return path + '/' + name; }
    std::string target() const { return dir + name; }
    bool move();
};
