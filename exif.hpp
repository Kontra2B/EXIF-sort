#pragma once
#include <ostream>
#include <vector>

#include "helper.hpp"

using TAG = uint16_t;

const char MARK = 0xFF;
const TAG JPEG = 0xD8FF;
const TAG EXIF = 0xE1FF;
const TAG SOS = 0xDAFF;
const TAG END = 0xD9FF;
const TAG DATE = 0x132;

struct File {
    bool picture;
    std::string name, date, time;
    uint32_t sos, size;
    std::ostream& operator<<(std::ifstream&);
    operator bool() const { return picture; };
    File(std::string name): picture(false), name(name), sos(0), size(0) {}
    friend std::ostream& operator<<(std::ostream&, const File&);
};
