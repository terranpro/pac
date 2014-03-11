#include "callback.hpp"
#include "runnable.hpp"
#include "context.hpp"

#include <iostream>

int donk( int x )
{
	std::cout << "x = " << x << "\n";

	return x + 1;
}

int main(int argc, char *argv[])
{
	double x = 2.;
	pac::callback<int(int)> cb( donk );
	pac::runnable r( donk, x );

	x = 35.;

	std::cout << pac::as_string( r.run() ) << "\n";

	return 0;
}
