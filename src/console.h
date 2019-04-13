#ifndef CONSOLE_CONSOLE_H
#define CONSOLE_CONSOLE_H

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
	Console();
	
	// Set the command prompt.
	void setPrompt(std::string prompt);
	
	// Push a character of input to the console.
	bool putc(char c);
	
private:
	// Called when a command has been entered.
	virtual void onCommand(std::string command) = 0;
	
private:
	// Refresh the command prompt.
	void refresh() const;
	
private:
	// The current command prompt.
	std::string _prompt;
	// Buffer for partial escape sequences.
	std::string _escBuffer;
	// The current command being entered.
	std::string _commandLine;
	// Position of the cursor within command.
	size_t _cursor;
	// Toggle for displaying the command line.
	bool _showPrompt;
	// The most recently pushed character.
	char _prev;
};

}
//                           End namespace Console                            //
//------------------------------------------------------------------------------

#endif
