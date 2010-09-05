#include "capp.hpp"

int main(int argc, char *argv[]) {
	cApp * MyApp = eeNew( cApp, ( argc, argv ) );

	MyApp->Process();

	eeDelete( MyApp );

	EE::MemoryManager::LogResults();

	return 0;
}
