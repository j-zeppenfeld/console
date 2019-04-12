#ifndef CONSOLE_CONSOLE_H
#define CONSOLE_CONSOLE_H

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
	
	// Push a character of input to the console.
	bool putc(char c);
};

}
//                           End namespace Console                            //
//------------------------------------------------------------------------------

#endif
