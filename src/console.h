#ifndef CONSOLE_CONSOLE_H
#define CONSOLE_CONSOLE_H

#include "history.h"

#include <string>

//------------------------------------------------------------------------------
//                          Begin namespace Console                           //
namespace Console {

//------------------------------------------------------------------------------
//--                              Class Console                               --
//------------------------------------------------------------------------------
class Console {
	// Not copyable nor assignable.
	Console(Console const &) = delete;
	Console &operator=(Console const &) = delete;
	
public:
	// Construct a console with the specified maximum command history size.
	Console(size_t historySize = 256);
	
	// Set the command prompt.
	void setPrompt(std::string prompt);
	
	// Load the command history from the specified file.
	// If homeDir is true, path is relative to the user's home directory.
	void loadHistory(std::string const &path, bool homeDir = true);
	// Save the command history to the specified file.
	// If homeDir is true, path is relative to the user's home directory.
	void saveHistory(std::string const &path, bool homeDir = true) const;
	// Add the specified string to the end of the history.
	void addHistory(std::string command);
	
	// Push a character of input to the console.
	bool putc(char c);
	
private:
	// Called when a command has been entered.
	virtual void onCommand(std::string command) = 0;
	
private:
	// Refresh the command prompt.
	void refresh() const;
	
private:
	// Command history.
	History _history;
	
	// The current command prompt.
	std::string _prompt;
	// Buffer for partial escape sequences.
	std::string _escBuffer;
	// Buffer for partial utf8 sequences.
	std::string _utf8Buffer;
	// The current command being entered.
	std::string _commandLine;
	// Position of the cursor within command.
	size_t _cursor;
	// Toggle for displaying the command line.
	bool _showPrompt;
	// Indicator of an active history search.
	bool _search;
	// The most recently pushed character.
	char _prev;
};

}
//                           End namespace Console                            //
//------------------------------------------------------------------------------

#endif
