#include "console.h"

#include <iostream>

class MyConsole : public Console::Console {
public:
	MyConsole() {
		loadHistory(".history");
	}
	
	~MyConsole() {
		saveHistory(".history");
	}
	
private:
	// Called when a command has been entered.
	virtual void onCommand(std::string command) override {
		std::cout << command << std::endl;
		addHistory(std::move(command));
	}
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
