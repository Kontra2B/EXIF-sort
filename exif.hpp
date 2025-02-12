#pragma once
#include <ostream>
#include <vector>
#include <filesystem>

using TAG = uint16_t;

const char MARK = 0xFF;
const TAG JPEG = 0xD8FF;
const TAG EXIF = 0xE1FF;
const TAG SOS = 0xDAFF;
const TAG END = 0xD9FF;
const TAG SIFD = 0x8769;
const TAG DATE = 0x9003;

struct File {
    bool picture, exif, sub;
    std::string name, path, date, time;
    std::string year, month, day;
    uint32_t sos, size;
    std::ifstream& operator<<(std::ifstream&);
    operator bool() const { return picture; };
    File(const std::filesystem::directory_entry& entry) {
        picture = exif = sub = false;
        sos = size = 0;
        name = entry.path().filename();
        path = entry.path().parent_path();
    }
    friend std::ostream& operator<<(std::ostream&, const File&);
};
