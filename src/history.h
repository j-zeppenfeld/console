#ifndef CONSOLE_HISTORY_H
#define CONSOLE_HISTORY_H

#include <string>
#include <vector>

//------------------------------------------------------------------------------
//                          Begin namespace Console                           //
namespace Console {

//------------------------------------------------------------------------------
//--                              Class History                               --
//------------------------------------------------------------------------------
class History {
public:
	// Construct a history with the specified maximum size.
	History(size_t maxSize = 256);
	
	// Load history from the specified file.
	void load(std::string const &path);
	// Save history to the specified file.
	void save(std::string const &path) const;
	
	// Append the specified command to the history.
	void push(std::string command);
	
	// Check if the history is empty.
	bool empty() const { return !(_full || _head); }
	// Retrieve the number of history entries.
	size_t size() const { return (_full ? _history.size() : _head); }
	
	// Retrieve the currently selected history entry.
	std::string const &current() const;
	// Browse or search backward to the previous history entry.
	// Stores the specified command if not already browsing.
	std::string const &backward(std::string const &command);
	// Browse or search forward to the next history entry.
	// Stores the specified command if not already browsing.
	std::string const &forward(std::string const &command);
	
	// Check if the history is being searched.
	bool searching() const { return _search; }
	// Start searching the history for the specified string.
	void search(std::string str);
	
	// Cancel any search and reset browsing position to the head of the history.
	void cancel();
	
private:
	// Retrieve the history index n ahead of pos.
	size_t next(size_t pos, size_t n = 1) const {
		return ((pos + n) % _history.size());
	}
	
	// Retrieve the history index n behind pos.
	size_t prev(size_t pos, size_t n = 1) const {
		return ((pos - n) % _history.size());
	}
	
private:
	// Store current command or search string while browsing history.
	std::string _stored;
	// Circular history queue.
	std::vector<std::string> _history;
	size_t _pos;
	size_t _head;
	// Indicator of a fully utilized history buffer.
	bool _full;
	// Indicator of an active history search.
	bool _search;
};

}
//                           End namespace Console                            //
//------------------------------------------------------------------------------

#endif
