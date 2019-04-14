#include "console.h"

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#	include <windows.h>
#	include <fcntl.h>
#	include <io.h>
#else
#	include <termios.h>
#	include <unistd.h>
#endif

#include <iostream>
#include <stdexcept>
#include <vector>

//------------------------------------------------------------------------------
//                          Begin namespace Console                           //
namespace Console {

//------------------------------------------------------------------------------
//                     Begin namespace <helper functions>                     //
namespace {

//------------------------------------------------------------------------------
//--                           Raw Mode RAII Class                            --
//------------------------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	class RawMode {
	public:
		RawMode() {
			int fdIn = _fileno(stdin);
			_setmode(fdIn, _O_BINARY);
			
			int fdOut = _fileno(stdout);
			_setmode(fdOut, _O_BINARY);
			
			_atty = _isatty(fdIn);
			if(_atty) {
				_stdin = (HANDLE)_get_osfhandle(fdIn);
				if(_stdin == INVALID_HANDLE_VALUE) {
					throw std::runtime_error(
						"Could not query terminal input."
					);
				}
				_stdout = (HANDLE)_get_osfhandle(fdOut);
				if(_stdout == INVALID_HANDLE_VALUE) {
					throw std::runtime_error(
						"Could not query terminal output."
					);
				}
				
				if(!GetConsoleMode(_stdin, &_stdinMode)) {
					throw std::runtime_error(
						"Could not query terminal input mode."
					);
				}
				if(!GetConsoleMode(_stdout, &_stdoutMode)) {
					throw std::runtime_error(
						"Could not query terminal output mode."
					);
				}
				
				if(!SetConsoleMode(_stdin, ENABLE_VIRTUAL_TERMINAL_INPUT)) {
					throw std::runtime_error(
						"Could not enable raw mode on terminal input."
					);
				}
				if(!SetConsoleMode(_stdout, ENABLE_VIRTUAL_TERMINAL_PROCESSING |
				                            ENABLE_PROCESSED_OUTPUT)) {
					throw std::runtime_error(
						"Could not enable raw mode on terminal output."
					);
				}
			} else {
				if(system("stty raw -echo opost")) {
					throw std::runtime_error(
						"Could not enable raw mode on terminal."
					);
				}
			}
		}
		
		~RawMode() {
			if(_atty) {
				SetConsoleMode(_stdin, _stdinMode);
				SetConsoleMode(_stdout, _stdoutMode);
			} else {
				system("stty sane");
			}
		}
		
	private:
		HANDLE _stdin;
		HANDLE _stdout;
		DWORD _stdinMode;
		DWORD _stdoutMode;
		bool _atty;
	};
#else
	class RawMode {
	public:
		RawMode() {
			_atty = isatty(STDIN_FILENO);
			if(_atty) {
				if(tcgetattr(STDIN_FILENO, &_termIos) == -1) {
					throw std::runtime_error(
						"Could not query terminal."
					);
				}
				
				termios raw = _termIos;
				
				// cfmakeraw(&raw), but allow output post-processing for sane
				// new-lines.
				raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR |
				                  IGNCR |ICRNL | IXON);
				raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
				raw.c_cflag &= ~(CSIZE | PARENB);
				raw.c_cflag |= CS8;
				
				// Put terminal into raw mode after flushing.
				if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) {
					throw std::runtime_error(
						"Could not enable raw mode on terminal."
					);
				}
			}
		}
		
		~RawMode() {
			// Restore original terminal settings.
			if(_atty) {
				tcsetattr(STDIN_FILENO, TCSAFLUSH, &_termIos);
			}
		}
		
	private:
		struct termios _termIos;
		bool _atty;
	};
#endif

//------------------------------------------------------------------------------
//--                           CSI Escape Sequences                           --
//------------------------------------------------------------------------------
namespace CSI {
	// Graphic parameters.
	auto constexpr resetAttributes = "\033[0m";
	auto constexpr bright = "\033[1m";
	
	// Foreground colors.
	auto constexpr black = "\033[30m";
	auto constexpr red = "\033[31m";
	auto constexpr green = "\033[32m";
	auto constexpr yellow = "\033[33m";
	auto constexpr blue = "\033[34m";
	auto constexpr magenta = "\033[35m";
	auto constexpr cyan = "\033[36m";
	auto constexpr white = "\033[37m";
	
	// Background colors.
	auto constexpr bgBlack = "\033[40m";
	auto constexpr bgRed = "\033[41m";
	auto constexpr bgGreen = "\033[42m";
	auto constexpr bgYellow = "\033[43m";
	auto constexpr bgBlue = "\033[44m";
	auto constexpr bgMagenta = "\033[45m";
	auto constexpr bgCyan = "\033[46m";
	auto constexpr bgWhite = "\033[47m";
	
	// Display refresh.
	auto constexpr clear = "\033[2K\r";
	
	// Cursor movement.
	auto constexpr up = "\033[A";
	auto constexpr down = "\033[B";
	auto constexpr right = "\033[C";
	auto constexpr left = "\033[D";
	
	// Stream manipulators for cursor movement.
	class leftN {
	public:
		leftN(size_t n) : _n(n) { }
		
		friend std::ostream &operator<<(std::ostream &os, leftN const &left) {
			if(left._n > 0) {
				os << "\033[" << std::dec << left._n << "D";
			}
			return os;
		}
		
	private:
		size_t _n;
	};
	
	// Retrieve key code corresponding to an escape sequence.
	enum Key {
		INVALID,
		INCOMPLETE,
		
		UP_ARROW,
		DOWN_ARROW,
		RIGHT_ARROW,
		LEFT_ARROW,
		
		SHIFT_UP_ARROW,
		SHIFT_DOWN_ARROW,
		SHIFT_RIGHT_ARROW,
		SHIFT_LEFT_ARROW,
		
		HOME,
		END,
		INSERT,
		DEL,
		PAGE_UP,
		PAGE_DOWN
	};
	Key getKey(std::string const &str) {
		static std::vector<std::pair<std::string, Key>> keyMap {
			{"\033[A", Key::UP_ARROW},
			{"\033[B", Key::DOWN_ARROW},
			{"\033[C", Key::RIGHT_ARROW},
			{"\033[D", Key::LEFT_ARROW},
			
			{"\033OA", Key::SHIFT_UP_ARROW},
			{"\033OB", Key::SHIFT_DOWN_ARROW},
			{"\033OC", Key::SHIFT_RIGHT_ARROW},
			{"\033OD", Key::SHIFT_LEFT_ARROW},
			{"\033OF", Key::END},
			{"\033OH", Key::HOME},
			
			{"\033[F", Key::END},
			{"\033[H", Key::HOME},
			
			{"\033[1;5A", Key::SHIFT_UP_ARROW},
			{"\033[1;5B", Key::SHIFT_DOWN_ARROW},
			{"\033[1;5C", Key::SHIFT_RIGHT_ARROW},
			{"\033[1;5D", Key::SHIFT_LEFT_ARROW},
			
			{"\033[1~", Key::HOME},
			{"\033[2~", Key::INSERT},
			{"\033[3~", Key::DEL},
			{"\033[4~", Key::END},
			{"\033[5~", Key::PAGE_UP},
			{"\033[6~", Key::PAGE_DOWN}
		};
		Key key = Key::INVALID;
		for(auto &pair : keyMap) {
			if(pair.first.find(str) == 0) {
				if(str.size() == pair.first.size()) {
					return pair.second;
				} else {
					key = Key::INCOMPLETE;
				}
			}
		}
		return key;
	}
}

//------------------------------------------------------------------------------
//--                          UTF-8 Helper Functions                          --
//------------------------------------------------------------------------------
namespace Utf8 {
	// Count number of utf8 octets at position.
	size_t countOctets(std::string const &str, size_t pos) {
		uint8_t masked = uint8_t(str[pos]) & 0xff;
		if(masked < 0x80) {
			return 1;
		} else if ((masked >> 5) == 0x6) {
			return 2;
		} else if ((masked >> 4) == 0xe) {
			return 3;
		} else if ((masked >> 3) == 0x1e) {
			return 4;
		} else {
			return 0;
		}
	}
	
	// Previous utf8 starting position.
	size_t posPrev(std::string const &str, size_t pos) {
		if(pos) {
			while((str[--pos] & 0xc0) == 0x80) {
				if(pos == 0) {
					break;
				}
			}
		}
		return pos;
	}
	
	// Next utf8 starting position.
	size_t posNext(std::string const &str, size_t pos) {
		if(pos < str.size()) {
			pos += countOctets(str, pos);
			if(pos > str.size()) {
				pos = str.size();
			}
		}
		return pos;
	}
	
	// Count number of utf8 octets in string, starting at pos.
	size_t count(std::string const &str, size_t pos = 0) {
		size_t n = 0;
		for(; pos < str.size(); pos = posNext(str, pos), ++n);
		return n;
	}
}

}
//                      End namespace <helper functions>                      //
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//--                              Class Console                               --
//------------------------------------------------------------------------------

// Construct a console with the specified maximum history size.
Console::Console(size_t historySize)
: _history(historySize)
, _prompt(": ")
, _cursor(0)
, _showPrompt(true)
, _search(false)
, _prev(0) {
	// Start console.
	static RawMode raw;
	
	// Print prompt.
	refresh();
}

// Set the command prompt.
void Console::setPrompt(std::string prompt) {
	_prompt = std::move(prompt);
	refresh();
}

// Load the command history from the specified file.
void Console::loadHistory(std::string const &path) {
	_history.load(path);
}

// Save the command history to the specified file.
void Console::saveHistory(std::string const &path) {
	_history.save(path);
}

// Add the specified string to the end of the history.
void Console::addHistory(std::string command) {
	_history.push(std::move(command));
}

// Push a character of input to the console.
bool Console::putc(char c) {
	static const char CTRL_C = 0x03;
	static const char CTRL_D = 0x04;
	static const char BS     = 0x08;
	static const char TAB    = 0x09;
	static const char LF     = 0x0A;
	static const char CR     = 0x0D;
	static const char CTRL_R = 0x12;
	static const char ESC    = 0x1B;
	static const char DEL    = 0x7F;
	
	switch(c) {
	case CTRL_C:
		std::cout << "\r\n^C" << std::endl;
		if(_commandLine.empty() && !_search) {
			_showPrompt = false;
			return false;
		} else {
			_cursor = 0;
			_commandLine.clear();
			_utf8Buffer.clear();
			_escBuffer.clear();
			_history.cancel();
			_search = false;
			refresh();
		}
		break;
	case CTRL_D:
		std::cout << "\r\n^D" << std::endl;
		_showPrompt = false;
		return false;
	case CTRL_R:
		_utf8Buffer.clear();
		_escBuffer.clear();
		if(_search) {
			_history.backward(_commandLine);
		} else {
			_search = true;
			_history.search(_commandLine);
		}
		refresh();
		break;
	case TAB:
		_utf8Buffer.clear();
		_escBuffer.clear();
		// Adopt search result.
		if(_search) {
			std::string const &result = _history.current();
			if(!result.empty()) {
				_commandLine = result;
			}
			_cursor = _commandLine.size();
			_history.cancel();
			_search = false;
			refresh();
			break;
		// Abort escaped search.
		} else if(_history.searching()) {
			_history.cancel();
		}
		
		// TODO: Autocompletion.
		break;
	case DEL:
	case BS: {
		// Erase complete utf8 codepoint.
		size_t end = _cursor;
		_cursor = Utf8::posPrev(_commandLine, _cursor);
		_commandLine.erase(_cursor, end - _cursor);
		_utf8Buffer.clear();
		_escBuffer.clear();
		if(_search) {
			_history.search(_commandLine);
		} else {
			_history.cancel();
		}
		refresh();
		break; }
	case LF:
		if(_prev == CR) {
			break;
		}
		goto CR; // [[fallthrough]]
	case CR: CR: {
		if(_search) {
			std::string const &result = _history.current();
			if(!result.empty()) {
				_commandLine = result;
			}
			// Redisplay as non-search prompt.
			_search = false;
			refresh();
		}
		std::cout << std::endl;
		
		if(!_commandLine.empty()) {
			onCommand(std::move(_commandLine));
		}
		
		_cursor = 0;
		_commandLine.clear();
		_utf8Buffer.clear();
		_escBuffer.clear();
		_history.cancel();
		_search = false;
		refresh();
		break; }
	case ESC:
		_utf8Buffer.clear();
		_escBuffer = c;
		// Cannot differentiate between ESC and an escape sequence, so
		// deactivate the search temporarily.
		if(_search) {
			_search = false;
			refresh();
		// Cancel an already escaped search.
		} else if(_history.searching()) {
			_history.cancel();
		}
		break;
	default: {
		bool escChar = !_escBuffer.empty();
		if(escChar) {
			_escBuffer.push_back(c);
			CSI::Key key = CSI::getKey(_escBuffer);
			// Restore an escaped search with a valid escape sequence.
			if(key != CSI::Key::INVALID) {
				_search = _history.searching();
			}
			switch(key) {
			case CSI::Key::INCOMPLETE:
				break;
			case CSI::Key::UP_ARROW:
				if(_search) {
					_history.backward(_commandLine);
				} else {
					_commandLine = _history.backward(_commandLine);
					_cursor = _commandLine.size();
				}
				break;
			case CSI::Key::DOWN_ARROW:
				if(_search) {
					_history.forward(_commandLine);
				} else {
					_commandLine = _history.forward(_commandLine);
					_cursor = _commandLine.size();
				}
				break;
			case CSI::Key::LEFT_ARROW:
				_cursor = Utf8::posPrev(_commandLine, _cursor);
				break;
			case CSI::Key::RIGHT_ARROW:
				_cursor = Utf8::posNext(_commandLine, _cursor);
				break;
			case CSI::Key::SHIFT_LEFT_ARROW:
				_cursor = Utf8::posPrev(_commandLine, _cursor);
				while(_cursor) {
					size_t pos = Utf8::posPrev(_commandLine, _cursor);
					if(_commandLine[pos] == ' ') {
						break;
					}
					_cursor = pos;
				}
				break;
			case CSI::Key::SHIFT_RIGHT_ARROW:
				while((_cursor = Utf8::posNext(_commandLine, _cursor))
				      < _commandLine.size()) {
					if(_commandLine[_cursor] == ' ') {
						break;
					}
				}
				break;
			case CSI::Key::HOME:
				_cursor = 0;
				break;
			case CSI::Key::END:
				_cursor = _commandLine.size();
				break;
			case CSI::Key::DEL: {
				// Erase complete utf8 codepoint.
				size_t end = Utf8::posNext(_commandLine, _cursor);
				_commandLine.erase(_cursor, end - _cursor);
				if(_search) {
					_history.search(_commandLine);
				} else {
					_history.cancel();
				}
				break; }
			case CSI::Key::INVALID:
				if(_history.searching() && !_search) {
					escChar = false;
					break;
				}
				goto DEFAULT; // [[fallthrough]]
			default: DEFAULT:
				std::cout << "\r\nUnknown escape sequence: ESC "
				          << _escBuffer.substr(1) << std::endl;
				break;
			};
			if(key != CSI::Key::INCOMPLETE) {
				_escBuffer.clear();
			}
		}
		if(!escChar) {
			_utf8Buffer.push_back(c);
			if(Utf8::countOctets(_utf8Buffer, 0) == _utf8Buffer.size()) {
				// Insert or append character to command.
				_commandLine.insert(_cursor, _utf8Buffer);
				_cursor += _utf8Buffer.size();
				_utf8Buffer.clear();
				if(_search) {
					_history.search(_commandLine);
				} else {
					_history.cancel();
				}
			}
		}
		
		refresh();
		
		break; }
	}
	_prev = c;
	return true;
}

// Refresh the command prompt.
void Console::refresh() const {
	if(_showPrompt) {
		// Prepare prompt line.
		std::cout << CSI::clear << CSI::green;
		
		if(_search) {
			std::cout << "history search : " << _commandLine;
			
			// Print search result.
			std::string display = " -> ";
			std::string const &result = _history.current();
			if(result.empty()) {
				display += "search failed";
			} else {
				display += result;
			}
			std::cout << display;
			
			// Move cursor backwards to end of command line.
			std::cout << CSI::leftN(Utf8::count(display));
		} else {
			std::cout << _prompt << _commandLine;
		}
		
		// Move cursor backwards to appropriate position.
		std::cout << CSI::leftN(Utf8::count(_commandLine, _cursor));
		
		// Reset all attributes/color.
		std::cout << CSI::resetAttributes << std::flush;
	}
}

}
//                           End namespace Console                            //
//------------------------------------------------------------------------------
