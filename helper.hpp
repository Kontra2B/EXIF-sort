#pragma once

#include <cstdint>
#include <vector>

#define outchar(data) dec << static_cast<uint16_t>(data) << 'x' << hex << uppercase << static_cast<uint16_t>(data) << dec
#define outhex(data) 'x' << hex << uppercase << data << dec
#define outvar(data) dec << data << 'x' << hex << uppercase << data << dec
#define outtime(time) time << hex << 'x' << uint64_t(time) << dec
#define outpair(first, second) dec << first << '/' << second
#define outpaix(first, second) hex << uppercase << 'x' << first << "/x" << second << dec
#define outwrite(os, var) os.write((char*)&var, sizeof(var)); os
#define tab '\t'
#define enter '\n'
#define clean "\033[2K\r"

bool ldump(const void*, uint, uint = 0);
bool pdump(const void*, const void*);
bool dump(const std::vector<char>&);
void confirm();
std::string& lower(std::string&);
