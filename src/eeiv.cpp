#include "capp.hpp"

EE_MAIN_FUNC int main(int argc, char *argv[]) {
	cApp * MyApp = eeNew( cApp, ( argc, argv ) );

	MyApp->Process();

	eeDelete( MyApp );

	Engine::DestroySingleton();

	EE::MemoryManager::ShowResults();

	return 0;
}