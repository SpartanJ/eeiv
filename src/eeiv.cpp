#include "capp.hpp"

EE_MAIN_FUNC int main(int argc, char *argv[]) {
	cApp * MyApp = eeNew( cApp, ( argc, argv ) );

	MyApp->Process();

	eeDelete( MyApp );

	Engine::destroySingleton();

	EE::MemoryManager::showResults();

	return 0;
}
