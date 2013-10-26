#include "capp.hpp"

int main(int argc, char *argv[]) {
	cApp * MyApp = eeNew( cApp, ( argc, argv ) );

	MyApp->Process();

	eeDelete( MyApp );

	cEngine::DestroySingleton();

	EE::MemoryManager::ShowResults();

	return 0;
}
