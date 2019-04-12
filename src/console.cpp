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

}
//                      End namespace <helper functions>                      //
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//--                              Class Console                               --
//------------------------------------------------------------------------------

Console::Console() {
	// Start console.
	static RawMode raw;
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
		std::cout << "^C" << std::endl;
		break;
	case CTRL_D:
		std::cout << "^D" << std::endl;
		return false;
	case CTRL_R:
		std::cout << "^R" << std::endl;
		break;
	case TAB:
		std::cout << "TAB" << std::endl;
		break;
	case DEL:
		std::cout << "DEL" << std::endl;
		break;
	case BS:
		std::cout << "BS" << std::endl;
		break;
	case LF:
		std::cout << "LF" << std::endl;
		break;
	case CR:
		std::cout << "CR" << std::endl;
		break;
	case ESC:
		std::cout << "ESC" << std::endl;
		break;
	default:
		std::cout << c << std::endl;
		break;
	}
	return true;
}

}
//                           End namespace Console                            //
//------------------------------------------------------------------------------
