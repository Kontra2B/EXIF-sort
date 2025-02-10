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

using namespace std;

#define sector 512

struct __attribute__ ((packed)) Entry {
    union {
        uint32_t    offset;
        struct {
            TAG         tag; 
            uint16_t    format;
            uint32_t    count;
            uint32_t    data;
        };
    };
    friend ostream& operator<<(ostream&, const Entry*);
};

struct __attribute__ ((packed)) Dir {
    uint16_t    count;
    Entry       entries[];
    friend ostream& operator<<(ostream&, const Dir*);
};

struct __attribute__ ((packed)) Tiff {
    char        ident[4];
    uint16_t    nulls;
    char        allign[2];
    uint16_t    fixed;
    uint32_t    offset;
    friend ostream& operator<<(ostream&, const Tiff*);
};

struct __attribute__ ((packed)) Tag {
    TAG         tag;
    uint16_t    size;
    union {
        char        data[0];
        const Tiff  tiff[0];
    };
    operator bool() const;
};

struct __attribute__ ((packed)) Jpeg {
    TAG         tag;
    const Tag   markers[];
    operator bool() const;
};

class File {
    ifstream        file;
    vector<char>    buffer;
    public:
    const Tag* find();
    File(ifstream&& file):file(move(file)), buffer(sector) {}
    friend istream& operator>>(istream&, File&);
};

ostream& operator<<(ostream&, const Tag*);
