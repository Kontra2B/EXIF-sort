#include <iostream>
#include <cstring>
#include <variant>
#include <map>
#include <list>
#include <unordered_map>
#include <sys/stat.h>

#include "helper.hpp"
#include "context.hpp"
#include "exif.hpp"

using namespace std;
using namespace filesystem;

Context context;

using option = variant<monostate, int64_t*, string*, list<string>*>;

void set(option* param, char* value) {
	if (!*value) return;
	if (auto target = get_if<int64_t*>(param)) **target = strtol(value, nullptr, 0);
	else if (auto target = get_if<string*>(param)) **target = value;
	else if (auto target = get_if<list<string>*>(param)) (*target)->push_back(value);
	*param = monostate{};
}

int main(int n, char** argv) {
	bool help(false);

	unordered_map<string, list<File>> dups;
	option pending;

	for (int i = 1; i < n; i++) {
		char* arg = argv[i];
		if (*arg == '-') {
			pending = monostate{};
			while (*++arg) {
				if (*arg == 'h') help = true;
				else if (*arg == 'R') context.move = true;
				else if (*arg == 'r') context.recurse = true;
				else if (*arg == 'c') context.confirm = true;
				else if (*arg == 'S') context.sup = true;
				else if (*arg == 'd') context.dups = true;
				else if (*arg == 'a') context.all = true;
				else if (*arg == 'Y') context.format = Context::Format::Year;
				else if (*arg == 'M') context.format = Context::Format::Month;
				else if (*arg == 'D') context.format = Context::Format::Day;
				else if (*arg == 'n') pending = &context.count;
				else if (*arg == 's') pending = &context.skip;
				else if (*arg == 'i') pending = &context.prefer;
				else if (*arg == 'x') pending = &context.avoid;
				else if (*arg == 'f') pending = &context.keys;
				else if (*arg == 't') pending = &context.out;
				else if (*arg == 'v') if (context.verbose) context.debug = true; else context.verbose = true;
				if (pending.index()) {
					set(&pending, ++arg);
					break;					// consume remaining chars
				}
			}
		}
		else if (pending.index()) set(&pending, arg);
		else {
			string dir(arg);
			while (dir.back() == '/') dir.pop_back();
			context.dirs.push_back(dir);
		}
	}

	if (help) cout << argv[0] << R"EOF( [OPTIONS] DIR [DIR]

DIR		working directory

OPTIONS:
-h		display this help message and quit, helpfull to see other argument parsed
-R		move files, dry run otherwise, only across one filesystem
-r		working directories recursive scan
-t		target directory, defaults to first working dir
-a		move all files, otherwise jpeg pictures only
-d		create list of duplicate files
-n		number of files to process
-s		number of files to skip
-v		be verbose, if repeated be more verbose with debug info
-Y		file path under target directory will be altered to /yyyy/
-M		file path under target directory will be altered to /yyyy/mm/
-D		file path under target directory will be altered to /yyyy/mm/dd/
-i		add preferred path key
-x		add void path key
-f		add preferred file name key
-S		suppress output
-c		confirm possible errors

Parsed arguments:
)EOF" << endl;

	while (context.out.back() == '/') context.out.pop_back();
	if (context.dirs.empty()) context.dirs.push_back(".");
	if (context.out.empty()) context.out = context.dirs.front();

	cout << context;

	if (help) exit(EXIT_SUCCESS);

	confirm(context.move);

	using dir_iterator = variant<directory_iterator, recursive_directory_iterator>;
	dir_iterator it;
	directory_options opts = directory_options::skip_permission_denied;

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

				if (file.target() == file.full()) {
					cout << tab << "on place";
					if(context.suppress()) cout << clean;
					else cout << enter;
					continue;
				}

				file.move();

				if (!file.pic && !context.all) continue;

				if (context.dups) {
					auto& list = dups[file.date];
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
			}
		}, it);
	}

	if (context.dups) {
		auto date = dups.begin();
		while (date != dups.end())
			if (date->second.size() < 2)
				date = dups.erase(date);
			else date++;
		ofstream of("duplicate.files.log");
		// of << "Duplicate files:" << endl;
		for (auto& date: dups) {
			auto it = date.second.cbegin();
			auto best = *it++;
			while (it != date.second.end())
				of << *it++ << tab << '#' << best << endl;
		}
	}
	return 0;
}

bool File::move()
{
	if (pic || context.all) {
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
	else if (!file.end) os << "bad END TAG";
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
	return size < file.size;
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
					char model[csize]{};
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

	dir = context.out + '/' + year + '/';
	if (context.format > context.Format::Year) dir += month + '/';
	if (context.format > context.Format::Month) dir += day + '/';
}
