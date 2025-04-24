#include <iostream>
#include <cstring>
#include <variant>
#include <list>
#include <unordered_map>
#include <sys/stat.h>
#include <fstream>

#include "helper.hpp"
#include "context.hpp"
#include "exif.hpp"

using namespace std;
using namespace filesystem;

Context context;

int main(int n, char** argv) {
	context.parse(n, argv);

	if (context.help) cout << argv[0] << R"EOF( *[OPTIONS|DIR]

DIR	working directory

OPTIONS:
	no-parameter options can be combined under one leading -
	space after parameter option may be ommited, parameter is consumed until next space
-h	display this help message and quit, helpfull to see other arguments parsed
-R	move files, dry run otherwise, only across one filesystem
-r	working directories recursive scan
-t dir	target directory, defaults to first working dir
-a	move all files, otherwise jpeg pictures only
-d file	create list of duplicate files, default file: duplicate.file.log
-n num	number of files to process
-s num 	number of files to skip
-v	be verbose, if repeated be more verbose with debug info
-N	file path under target directory will be removed
-Y	file path under target directory will be fitted to /yyyy/...
-M	file path under target directory will be fitted to /yyyy/mm/... default
-D	file path under target directory will be fitted to /yyyy/mm/dd/...
	... based on picture original EXIF or modification date
-i key	add preferred path key...
-x key	add void path key...
-f key	add preferred file name key...
	... key is any fixed substring
-S	suppress output
-c	confirm possible errors

Parsed arguments:
)EOF" << endl;

	cout << context;

	if (context.help) exit(EXIT_SUCCESS);

	confirm(context.move);

	using dir_iterator = variant<directory_iterator, recursive_directory_iterator>;
	dir_iterator it;
	directory_options opts = directory_options::skip_permission_denied;
	unordered_map<string, list<File>> dups;

	for (const auto& dir: context.dirs) {
		if (!context.count) break;
		if (!exists(dir)) { cerr << "Directory not found: " << dir << endl; continue; }
		try {
			if (context.recurse) it = recursive_directory_iterator(dir, opts);
			else it = directory_iterator(dir, opts);
		}
		catch (filesystem_error&) {
			cerr << "Not a directory: " << dir << endl;
			continue;
		}
		visit([&dups](auto&& iter){
			int i = 0;
			for (auto entry: iter) {
				if (!entry.is_regular_file()) continue;
				i++;
				if (context.skip && context.skip--) continue;
				if (!context.count || !context.count--) break;
				ifstream ifs(entry.path(), ios::binary);
				if (!ifs) { cerr << "Could not open file: " << entry.path() << endl; continue; }
				File file(entry.path());
				cerr << i << ". ";
				file << ifs;		// create file object from disk file
				cout << file;

				if (!context.dups.empty())
					if (file.pic || context.all) {
						auto& list = dups[file.cam + ':' + file.date];
						auto it = list.begin();
						while (it != list.end())
						if (file > *it) {
							list.insert(it, file);
							break;
						} else it++;
					if (it == list.end())
						if (!file.pic || file.dat)
							list.push_back(file);
					}

				file.move();
			}
		}, it);
	}

	if (!context.dups.empty()) {
		auto entry = dups.begin();
		while (entry != dups.end())
			if (entry->second.size() < 2)
				entry = dups.erase(entry);
			else entry++;
		ofstream of(context.dups);
		for (auto& entry: dups) {
			auto it = entry.second.cbegin();
			const auto best = *it++;
			while (it != entry.second.end())
				of << it++->dump(best) << endl;
		}
	}
	return 0;
}

string File::dump(const File& file) const {
	ostringstream oss;
	oss << quoted(full()) << tab << '#' << quoted(file.full()) << tab;
	if (*this ^ file) oss << '!';
	if (pic ^ file.pic) oss << "/pic";
	else if (exif ^ file.exif) oss << "/exif";
	else if (sos ^ file.sos) oss << "/stream";
	else if (sub ^ file.sub) oss << "/orignal";
	else if (end ^ file.end) oss << "/end";
	if (res ^ file.res) oss << "/res";
	else {
		if (width != file.width) oss << "width:" << width << '/' << file.width << ", ";
		if (hight != file.hight) oss << ",height:" << hight << '/' << file.hight << ", ";
	}
	if (ornt != file.ornt) oss << "orientation:" << ornt << '/' << file.ornt << ", ";
	if (size != file.size) oss << "size:" << size << '/' << file.size << ", ";
	char c;
	while (c = oss.str().back(), c == ' ' || c == ',') oss.str().pop_back();
	return std::move(oss.str());
}

bool File::move()
{
	if (pic || context.all) {
		if (target() == full()) {
			cout << tab << "on place";
			if(context.suppress()) cout << clean;
			else cout << enter;
			return false;
		}
		cout << "... " << tab;
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
				else cerr << "OVERWRITING: " << file << endl;
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
	os << file.full() << tab;
	if (!file) os << "! ";
	if (!file.cam.empty()) os << ' ' << '\'' << file.cam << '\'';
	if (!file.date.empty()) os << ' ' << file.date;
	os << ' ' << file.size;
	if (file.res) os << ' ' << file.width << '/' << file.hight;
	if (file.exif) os << '/' << file.ornt;
	if (!file) os << tab;
	if (!file.pic) os << "not JPEG file";
	else if (!file.exif) os << "no EXIF data";
	else if (!file.sos) os << "no data stream marker";
	else if (!file.sub) os << "no original camera IDF data";
	else if (file.date.empty()) os << "no original date";
	else if (!file.end) os << "bad END tag";
	return os;
}

bool File::operator>(const File& file)
{
	if (!*this) return false;
	else if (!file) return true;
	else if (width * hight > file.width * file.hight) return true;
	else if (width * hight < file.width * file.hight) return false;
	else if (ornt > 1 && file.ornt <= 1) return true;
	else if (file.ornt > 1 && ornt <= 1) return false;
	else if (hight < file.hight) return false;
	else if (file.hight < hight) return true;
	for (const auto& key: context.prefer)
		if (file.path.find(key) != string::npos) return false;
		else if (path.find(key) != string::npos) return true;
	for (const auto& key: context.avoid)
		if (full().find(key) != string::npos) return false;
		else if (file.full().find(key) != string::npos) return true;
	for (const auto& key: context.keys)
		if (file.name.find(key) != string::npos) return false;
		else if (name.find(key) == string::npos) return true;
	// if (path.size() > file.path.size()) return true;
	if (size < file.size) return true;
	else if (size > file.size) return false;
	else return path.length() < file.path.length();
}

#define IFSTREAM_READ_SIZE(tag, size) { if (!is.read((char*)&(tag), size)) { if (context.verbose) cerr << "Read error @" << is.tellg() << tab; return; }}
#define IFSTREAM_READ(tag) IFSTREAM_READ_SIZE(tag, sizeof(tag))
#define IFSTREAM_SEEK_REF(pos, rel) { if (!(is).seekg((pos), (rel))) { if (context.verbose) cerr << "Seek error @" << (pos) << tab; return; }}
#define IFSTREAM_SEEK(pos) IFSTREAM_SEEK_REF(pos, ios::beg)
#define IFSTREAM_SEEK_REL(pos) IFSTREAM_SEEK_REF(pos, ios::cur)
#define	DEBUG(text) { if (context.debug) cerr << text; }

void File::operator<<(ifstream& is)
{
	struct stat info;
	stat(full().c_str(), &info);
	size = info.st_size;
	char buffer[20];	// YYYY:MM:DD-HH:MM:SS = 19 char + null
	std::tm tm_date;
	localtime_r(&info.st_mtime, &tm_date);
	std::strftime(buffer, sizeof(buffer), "%Y:%m:%d-%H:%M:%S", &tm_date);
	date = string(buffer);
	uint16_t tag, size;
	IFSTREAM_READ(tag)
	if (tag == JPEG) {
		uint32_t sof(0), ref(0);
		pic = true;
		DEBUG("APP:");
		do {
			auto pos = is.tellg();
			IFSTREAM_READ(tag)
			if ((tag & 0xFF) != MARK) break;
			bool bold = tag == EXIF || tag == SOF;
			IFSTREAM_READ(size)
			size = __builtin_bswap16(size);
			DEBUG(' ' << (bold? bold_on: bold_off) << outhex(__builtin_bswap16(tag)) << '@' << outhex(pos) << bold_off)
			if (!ref && tag == EXIF) ref = is.tellg();
			if (tag == SOS) sos = true;
			if (tag == SOF) sof = is.tellg();
			IFSTREAM_SEEK_REL(size - sizeof(size))
		} while (tag != SOS);
		DEBUG(enter);
		if (ref) {
			char id[4];
			IFSTREAM_SEEK(ref)
			IFSTREAM_READ(id);
			if (!strncmp(id, EXIFID, 4)) {
				exif = true;
				uint32_t cof(0), csize(0);
				IFSTREAM_SEEK_REL(2);
				char tiff[4] = {};
				bool swap = false;
				IFSTREAM_READ_SIZE(tiff, 2);
				if (!strncmp(tiff, "MM", 2)) swap = true;
				IFSTREAM_SEEK(ref + 10L);
				ref +=6L;
				uint32_t offset;
				IFSTREAM_READ(offset);
				if (swap) offset = __builtin_bswap32(offset);
				IFSTREAM_SEEK_REL(offset - 8);
				IFSTREAM_READ(size);
				if (swap) size = __builtin_bswap16(size);
				DEBUG("EXIF/" << outvar(size) << ',' << "Id" << ':' << string(tiff) << (swap? "/swap": "") << ':')
				uint16_t format;
				uint32_t count(0);
				while(size--) {
					auto pos = is.tellg();
					IFSTREAM_READ(tag)
					bool bold = tag == SIFD || tag == CAM || tag == ORNT;
					if (swap) tag = __builtin_bswap16(tag);
					DEBUG(' ' << (bold? bold_on: bold_off) << outhex(tag) << '@' << outhex(pos) << bold_off);
					if (tag == CAM) {
						IFSTREAM_READ(format);
						IFSTREAM_READ(csize);
						IFSTREAM_READ(cof);
						if (swap) csize = __builtin_bswap32(csize);
						if (swap) cof = __builtin_bswap32(cof);
						continue;
					}
					if (tag == ORNT) {
						IFSTREAM_READ(format);
						IFSTREAM_READ(count);
						IFSTREAM_READ(ornt);
						if (swap) ornt = __builtin_bswap16(ornt);
						IFSTREAM_SEEK_REL(2)
						continue;
					}
					if (tag == SIFD) break;
					IFSTREAM_SEEK_REL(10)
				}
				DEBUG(enter)
				if (tag == SIFD) {
					IFSTREAM_READ(format)
					IFSTREAM_READ(count)
					IFSTREAM_READ(offset)
					if (swap) offset = __builtin_bswap32(offset);
					IFSTREAM_SEEK(ref + offset)								// seek to SUB IFD
					IFSTREAM_READ(size)
					if (swap) size = __builtin_bswap16(size);
					sub = true;
					DEBUG("SIFD/" << outvar(size) << '@' << outhex(offset) << ':')
					uint32_t pdate(0), pwidth(0), phight(0);
					while(size--) {
						auto pos = is.tellg();
						IFSTREAM_READ(tag)
						if (swap) tag = __builtin_bswap16(tag);
						bool bold = tag == DATE || tag == WIDTH || tag == HIGHT;
						DEBUG(' ' << (bold? bold_on: bold_off) << outhex(tag) << '@' << outhex(pos))
						if (tag == DATE) pdate = is.tellg();
						if (tag == WIDTH) pwidth = is.tellg();
						if (tag == HIGHT) phight = is.tellg();
						IFSTREAM_SEEK_REL(10)
					}
					DEBUG(enter)
					if (pdate) {
						IFSTREAM_SEEK(pdate)
						IFSTREAM_READ(format);
						if (swap) format = __builtin_bswap16(format);
						if (format == 2) {
							IFSTREAM_READ(count);
							if (swap) count = __builtin_bswap32(count);
							IFSTREAM_READ(offset);
							if (swap) offset = __builtin_bswap32(offset);
							auto pos = ref + offset;
							IFSTREAM_SEEK(pos)
							is >> date;
							is.get();
							char time[10];
							IFSTREAM_READ(time);
							date += '-' + string(time);
							if (date == "0000:00:00-00:00:00") date = buffer;		// revert to file modification date
							else dat = true;
							DEBUG("Date" << '@' << outhex(pos) << ": " << date)
						}
					}
				}
				if (cof) {
					char model[csize];
					IFSTREAM_SEEK(ref + cof)
						IFSTREAM_READ(model)
						cam = model;
					while (cam.back() == ' ') cam.pop_back();
					DEBUG(tab << "camera" << '@' << outhex(ref + cof) << ": " << cam << tab)
				}
			}
		}

		if (sof) {
			IFSTREAM_SEEK(++sof)
			IFSTREAM_READ(hight)
			hight = __builtin_bswap16(hight);
			DEBUG(tab << "hight" << '@' << outhex(sof) << ": " << outvar(hight) << tab)
			IFSTREAM_READ(width)
			width = __builtin_bswap16(width);
			DEBUG(tab << "width" << '@' << outhex(sof + 2) << ": " << outvar(width) << enter)
			res = sof;
		}

		IFSTREAM_SEEK_REF(-2, ios::end);
		IFSTREAM_READ(tag)
		if (is && tag == END) end = true;
	}

	istringstream iss(date);
	getline(iss, year, ':');
	getline(iss, month, ':');
	getline(iss, day, '-');

	dir = context.out + '/';
	if (context.format > Context::Format::None) dir += year + '/';
	if (context.format > Context::Format::Year) dir += month + '/';
	if (context.format > Context::Format::Month) dir += day + '/';
}
