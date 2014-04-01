#include "callback.hpp"
#include "runnable.hpp"
#include "context.hpp"
#include "signal.hpp"

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

void basic_runnable_test()
{
	double x = 2.;
	pac::callback<int(int)> cb( donk );
	pac::runnable r( donk, x );

	x = 35.;

	std::cout << pac::as_string( r.run() ) << "\n";
}

struct player
{
	std::size_t id;
	pac::toe toe;
	std::atomic<bool> primary;
	//pac::callback<void()> cb;
	player *prev;

	player( std::size_t i )
		: id(i), toe{}, primary{false}, prev{nullptr}
	{
		init();
	}

	player( std::size_t i, player *r, bool p = false )
		: id(i), toe{}, primary{p}, prev{r}
	{
		init();
	}

	void init()
	{
		pac::callback<void()> cb = pac::callback<void()>( &player::running, this );
		toe.add_callback( cb );
		toe.launch( pac::toe::launch_type::async );
	}

	void running()
	{
		if (!primary)
			toe.pause();

		std::cout << "player " << id << " is running with the ball...\n";
		std::this_thread::sleep_for( std::chrono::seconds(1) );
		if (prev)
			pass();
		else
			touchdown();
	}

	void pass()
	{
		primary = false;
		prev->heycatch();
		toe.pause();
	}

	void heycatch()
	{
		primary = true;
		toe.resume();
	}

	void touchdown()
	{
		std::cout << "player " << id << " has scored! TOUCHDOWN!!\n";
		toe.pause();
	}

	void quit()
	{
		toe.quit();
		if (prev)
			prev->quit();
	}

};

void football_test()
{
	std::unique_ptr<player> p1 { new player( 1 ) };
	std::unique_ptr<player> p2 { new player( 2, p1.get() ) };
	std::unique_ptr<player> p3 { new player( 3, p2.get() ) };
	std::unique_ptr<player> p4 { new player( 4, p3.get(), true ) };

	std::this_thread::sleep_for( std::chrono::seconds(6) );

	p4->quit();
	p3->quit();
	p2->quit();
	p1->quit();
}

void toe_callback_test()
{
	pac::callback<void( int )> cb(
		[](int x)
		{
			std::cout << std::this_thread::get_id() << ": " << x+3 << "\n";
		} );

	pac::signal<void( int )> sig;
	pac::toe toe;

	toe.launch( pac::toe::launch_type::async );

	pac::callback<void( int )> toecb = pac::toe_callback( toe, cb );

	std::cout << std::this_thread::get_id() << ": START!\n";

	auto con = sig.connect( toecb );
	for ( auto count = 0; count < 5; ++count ) {
		sig.emit( 11 );
		std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
	}
}

int main(int argc, char *argv[])
{
	basic_runnable_test();

	football_test();

	toe_callback_test();

	return 0;
}
