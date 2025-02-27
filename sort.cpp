#include <iostream>
#include <cstring>
#include <variant>
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
				else if (*arg == 'R') context.move = true;
				else if (*arg == 'r') context.recurse = true;
				else if (*arg == 'c') context.confirm = true;
				else if (*arg == 'S') context.sup = true;
				else if (*arg == 'd') context.dups = true;
				else if (*arg == 'a') context.all = true;
				else if (*arg == 'v') if (context.verbose) context.debug = true; else context.verbose = true;
				else if (*arg == 'Y') context.format = Context::Format::Year;
				else if (*arg == 'M') context.format = Context::Format::Month;
				else if (*arg == 'D') context.format = Context::Format::Day;
				else if (*arg == 'n') { context.count = strtol(++arg, nullptr, 0); break; }
				else if (*arg == 's') { context.skip = strtol(++arg, nullptr, 0); break; }
				else if (*arg == 'i') { context.prefer.push_back(++arg); break; }
				else if (*arg == 'x') { context.avoid.push_back(++arg); break; }
				else if (*arg == 'f') { context.name.push_back(++arg); break; }
				else if (*arg == 't') {
					if (*++arg) context.out = arg;
					else pending = true;
					break;
				}
			}
		}
		else if (pending) context.out = arg;
		else {
			string dir(arg);
			while (dir.back() == '/') dir.pop_back();
			context.dirs.push_back(dir);
		}
	}

	if (help) cout << argv[0] << R"EOF( [OPTIONS] DIR

DIR		working directory

OPTIONS:
-h		display this help message and quit, helpfull to see other argument parsed
-R		move files, dry run otherwise, only across one filesystem
-r		move files, dry run otherwise, only across one filesystem
-a		move all files, otherwise jpeg pictures only
-d		create list of duplicate pictures
-n		number of files to process
-s		number of files to skip
-t		target directory, defaults to working dir
-v		be verbose, if repeated be more verbose with debug info
-Y		file path under target directory will be altered to /yyyy/
-M		file path under target directory will be altered to /yyyy/mm/
-D		file path under target directory will be altered to /yyyy/mm/dd/
-i		add preferred path key
-x		add void path key
-f		add preferred file name key
-S		suppres output
-c		confirm possible errors

Parsed arguments:
)EOF" << endl;

	while (context.out.back() == '/') context.out.pop_back();
	if (context.dirs.empty()) context.dirs.push_back(".");
	if (context.out.empty()) context.out = context.dirs.front();

	cout << context;

	if (help) exit(EXIT_SUCCESS);

	confirm(context.move);
	using any_iterator = variant<directory_iterator, recursive_directory_iterator>;
	any_iterator it;
	directory_options opts = directory_options::skip_permission_denied;

	for (const auto& dir: context.dirs) {
		if (!exists(dir)) {
			cerr << "Directory not found: " << dir << endl;
			continue;
		}
		if (!context.count) break;
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

					if (!file.picture && !context.all) continue;

					if (context.dups) {
						auto& list = dups[file.date];
						auto it = list.begin();
						while (it != list.end())
							if (file > *it) {
								list.insert(it, file);
								break;
							} else it++;
						if (it == list.end())
							if (!file.picture
									|| file.dat)
								list.push_back(file);
					}
				}
		}, it);
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
				of << *it++ << tab << '#' << best << endl;
		}
	}
	return 0;
}

bool File::move()
{
	if (picture || context.all) {
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
	os << file.full() << tab;
	if (!file) os << "! ";
	if (!file.make.empty()) os << ' ' << '\'' << file.make << '\'';
	if (!file.date.empty()) os << ' ' << file.date;
	os << ' ' << file.size;
	if (file.res) os << ' ' << file.width << '/' << file.hight;
	if (file.exif) os << '/' << file.ornt;
	if (!file) os << tab;
	if (!file.picture) os << "not JPEG file";
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
	if (!file) return true;
	if (width * hight > file.width * file.hight) return true;
	if (width * hight < file.width * file.hight) return false;
	if (ornt > 1 && file.ornt <= 1) return true;
	if (hight < file.hight) return false;
	for (const auto& key: context.prefer)
		if (file.path.find(key) != string::npos) return false;
		else if (path.find(key) != string::npos) return true;
	for (const auto& key: context.avoid)
		if (full().find(key) != string::npos) return false;
		else if (file.full().find(key) != string::npos) return true;
	for (const auto& key: context.name)
		if (file.name.find(key) != string::npos) return false;
		else if (name.find(key) == string::npos) return true;
	// if (path.size() > file.path.size()) return true;
	return size < file.size;
}

#define IFSTREAM_READ_SIZE(tag, size) { if (!is.read((char*)&(tag), size)) { if (context.verbose) cerr << "Read error @" << is.tellg() << tab; return is; }}
#define IFSTREAM_READ(tag) IFSTREAM_READ_SIZE(tag, sizeof(tag))
#define IFSTREAM_SEEK_REF(pos, rel) { if (!(is).seekg((pos), (rel))) { if (context.verbose) cerr << "Seek error @" << (pos) << tab; return is; }}
#define IFSTREAM_SEEK(pos) IFSTREAM_SEEK_REF(pos, ios::beg)
#define IFSTREAM_SEEK_REL(pos) IFSTREAM_SEEK_REF(pos, ios::cur)
#define	DEBUG(text) { if (context.debug) cerr << text; }

ifstream& File::operator<<(ifstream& is)
{
	struct stat info;
	stat(full().c_str(), &info);
	size = info.st_size;
	char buffer[20];	// YYYY-MM-DD HH:MM:SS = 19 znaków + null
	std::tm tm_date;
	localtime_r(&info.st_mtime, &tm_date);
	std::strftime(buffer, sizeof(buffer), "%Y:%m:%d-%H:%M:%S", &tm_date);
	date = string(buffer);
	uint16_t tag, size;
	uint32_t mkof(0), mksize(0);
	IFSTREAM_READ(tag)
	if (tag == JPEG) {
		uint32_t sof(0), ref(0);
		picture = true;
		DEBUG("APP:");
		do {
			auto pos = is.tellg();
			IFSTREAM_READ(tag)
			if ((tag & 0xFF) != MARK) break;
			IFSTREAM_READ(size)
			size = __builtin_bswap16(size);
			DEBUG(' ' << outhex(__builtin_bswap16(tag)) << '@' << outhex(pos) << (tag == EXIF || tag == SOF? "*": ""))
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
					if (swap) tag = __builtin_bswap16(tag);
					DEBUG(' ' << outhex(tag) << '@' << outhex(pos) << (tag == SIFD? "*": ""))
					if (tag == MAKE) {
						IFSTREAM_READ(format);
						IFSTREAM_READ(mksize);
						IFSTREAM_READ(mkof);
						if (swap) mksize = __builtin_bswap32(mksize);
						if (swap) mkof = __builtin_bswap32(mkof);
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
				};
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
						DEBUG(' ' << outhex(tag) << '@' << outhex(pos) << (tag == DATE? "*":""))
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
			}
		}

		if (mkof) {
			char model[mksize]{};
			IFSTREAM_SEEK(ref + mkof)
			IFSTREAM_READ(model)
			make = model;
			while (make.back() == ' ') make.pop_back();
			DEBUG(tab << "camera" << '@' << outhex(ref + mkof) << ": " << make << tab)
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

	return is;
}
