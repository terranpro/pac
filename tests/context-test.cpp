#include "callback.hpp"
#include "runnable.hpp"
#include "context.hpp"

#include <iostream>
#include <thread>

int donk( int x )
{
	std::cout << "x = " << x << "\n";

	auto beg = std::chrono::high_resolution_clock::now();

	std::this_thread::sleep_for( std::chrono::nanoseconds(1) );

	auto end = std::chrono::high_resolution_clock::now();

	return x + 1;
}

int main(int argc, char *argv[])
{
	double x = 2.;
	pac::callback<int(int)> cb( donk );
	pac::runnable r( donk, x );

	x = 35.;

	std::cout << pac::as_string( r.run() ) << "\n";

	pac::toe toe;
	auto ctxt = pac::context::create();

	ctxt->add_runnable( std::make_shared< pac::runnable >( donk, x ) );
	toe.launch( ctxt, pac::toe::async );

	std::this_thread::sleep_for( std::chrono::milliseconds(500) );
	toe.pause();
	std::cout << "Paused.\n";

	std::this_thread::sleep_for( std::chrono::milliseconds(5000) );

	std::cout << "Resumed.\n";

	toe.resume();

	std::this_thread::sleep_for( std::chrono::milliseconds(500) );
	toe.pause();
	std::cout << "Paused.\n";

	std::this_thread::sleep_for( std::chrono::milliseconds(5000) );

	std::cout << "Resumed.\n";

	toe.resume();

	toe.join();

	return 0;
}
