#include "sample-gtkmm.hpp"

#include "SampleGtkmmRootPAC.hpp"

#include <memory>

int main(int argc, char *argv[])
{
	std::unique_ptr<SampleGtkmmRootPAC> root { new SampleGtkmmRootPAC };

	// root->load( argc, argv );
	// root->run();

	root->load();
	root->run( argc, argv );

	return 0;
}
