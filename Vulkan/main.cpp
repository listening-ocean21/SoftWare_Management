#include "CMultiSample.h"
//using namespace test9;

int main() {
	CTest app;

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