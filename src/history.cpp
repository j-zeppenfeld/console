#include "history.h"

#include <fstream>

//------------------------------------------------------------------------------
//                          Begin namespace Console                           //
namespace Console {

//------------------------------------------------------------------------------
//                     Begin namespace <helper functions>                     //
namespace {

//------------------------------------------------------------------------------
//--                      Home Directory Helper Function                      --
//------------------------------------------------------------------------------
std::string toHomePath(std::string const &path) {
	// Return path if it is absolute.
	if(path.empty() || path[0]=='/'
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	   || (path.size()>0 && path[1]==':')
#endif
	) {
		return path;
	}
	
	// Prefix path with home directory.
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable:4996) // Disable check for "unsafe" functions.
#endif
	char const *homePath = getenv(
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
		"USERPROFILE"
#else
		"HOME"
#endif
	);
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
	if(!homePath) {
		return path;
	}
	return std::string(homePath) + '/' + path;
}

//------------------------------------------------------------------------------
//--                         GetLine Helper Function                          --
//------------------------------------------------------------------------------
// See https://stackoverflow.com/a/6089413/694509.
std::istream &safeGetLine(std::istream &is, std::string &line) {
	line.clear();
	
	std::istream::sentry se(is, true);
	std::streambuf *sb = is.rdbuf();
	while(true) {
		int c = sb->sbumpc();
		switch(c) {
		case '\n':
			return is;
		case '\r':
			if(sb->sgetc() == '\n') {
				sb->sbumpc();
			}
			return is;
		case std::streambuf::traits_type::eof():
			if(line.empty()) {
				is.setstate(std::ios::eofbit);
			}
			return is;
		default:
			line += (char)c;
		}
	}
}

}
//                      End namespace <helper functions>                      //
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//--                              Class History                               --
//------------------------------------------------------------------------------

// Construct a history with the specified maximum size.
History::History(size_t maxSize)
: _history(maxSize > 1 ? maxSize : 2)
, _pos(0)
, _head(0)
, _full(false)
, _search(false) { }

// Load history from the specified file.
void History::load(std::string const &path, bool homeDir) {
	_pos = 0;
	_head = 0;
	_full = false;
	
	std::ifstream file(homeDir ? toHomePath(path) : path);
	for(std::string line; safeGetLine(file, line);) {
		if(!line.empty()) {
			push(line);
		}
	}
}

// Save history to the specified file.
void History::save(std::string const &path, bool homeDir) const {
	if(!empty()) {
		std::ofstream file(homeDir ? toHomePath(path) : path);
		size_t i = 0;
		if(_full) {
			file << _history[_head] << std::endl;
			i = next(_head);
		}
		for(; i != _head; i = next(i)) {
			file << _history[i] << std::endl;
		}
	}
}

// Append the specified command to the history.
void History::push(std::string command) {
	// Ignore duplicate entries.
	if(!empty() && command == _history[prev(_head)]) {
		return;
	}
	
	_history[_head] = std::move(command);
	++_head;
	if(_head >= _history.size()) {
		_head = 0;
		_full = true;
	}
}

// Retrieve the currently selected history entry.
std::string const &History::current() const {
	if(!_pos) {
		return _stored;
	} else {
		return _history[prev(_head, _pos)];
	}
}

// Browse or search backward to the previous history entry.
std::string const &History::backward(std::string const &command) {
	if(_search) {
		// String to search for is emptied when there are no results.
		if(_stored.empty()) {
			return _stored;
		}
		// Search backward through history for the search string.
		for(size_t pos = _pos; ++pos <= size();) {
			std::string const &entry = _history[prev(_head, pos)];
			if(entry.find(_stored) != std::string::npos) {
				_pos = pos;
				return entry;
			}
		}
		// Clear search string if there are no results.
		if(!_pos) {
			_stored.clear();
		}
	} else {
		if(!_pos) {
			_stored = command;
		}
		if(_pos < size()) {
			++_pos;
		}
	}
	return current();
}

// Browse or search forward to the next history entry.
std::string const &History::forward(std::string const &command) {
	if(_search) {
		// String to search for is emptied when there are no results.
		// Since search initially calls backward, if _stored is not empty there
		// is at least one result.
		if(_stored.empty() || !_pos) {
			return _stored;
		}
		// Search forward through history for the search string.
		for(size_t pos = _pos; --pos;) {
			std::string const &entry = _history[prev(_head, pos)];
			if(entry.find(_stored) != std::string::npos) {
				_pos = pos;
				return entry;
			}
		}
	} else {
		if(!_pos) {
			return command;
		}
		--_pos;
	}
	return current();
}

// Start searching the history for the specified string.
void History::search(std::string str) {
	_stored = std::move(str);
	_pos = 0;
	_search = true;
	backward(str);
}

// Cancel any search and reset browsing position to the head of the history.
void History::cancel() {
	_stored.clear();
	_pos = 0;
	_search = false;
}

}
//                           End namespace Console                            //
//------------------------------------------------------------------------------
