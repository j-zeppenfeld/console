#include "console.h"

#include <iostream>

class MyConsole : public Console::Console {
};

int main() try {
	MyConsole console;
	while(console.putc(getchar()));
	return 0;
} catch(std::exception const &e) {
	std::cerr << "Unexpected: " << e.what() << std::endl;
} catch(...) {
	std::cerr << "Unexpected: Unknown exception type." << std::endl;
}
