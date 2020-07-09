#include "CTest.h"
using namespace test8;

int main() {
	test8::CTest app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	system("pause");
	return EXIT_SUCCESS;
}