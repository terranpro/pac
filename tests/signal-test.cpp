#include "signal.hpp"

#include <iostream>
#include <algorithm>

#include <cassert>

using namespace pac;

struct Server
{
	int even = 0;

	pac::signal<int(int)> sigadd;
	pac::signal<int(int)> sigsub;

	typename pac::signal<int(int)>::results_type
	Add( int in )
	{
		return sigadd.emit( in );
	}

	typename pac::signal<int(int)>::results_type
	Sub( int in )
	{
		return sigsub.emit( in );
	}
};

struct Client
{
	Server& server;
	connection con_add;
	connection con_sub;

	Client(Server& s)
		: server(s)
	{
		con_add =
			server.sigadd.connect(
				this, &Client::OnAdd);
		BARK;
		con_sub =
			server.sigsub.connect(
				this, &Client::OnSub);
	}

	int OnAdd( int in )
	{
		return in * 2;
	}

	int OnSub( int in )
	{
		return in - 2;
	}
};

int another_add( int in )
{
	return in + 7;
}

int another_dec( int in )
{
	return in -7;
}

int main(int, char *[])
{
	Server s;
	Client *c = new Client{s};

	auto r1 = s.Add( 5 );
	// should be 5 * 2 == { 10 }
	assert( std::accumulate( r1.begin(), r1.end(), 0 ) == 10 );

	{
		auto con = s.sigadd.connect( another_add );
		auto r2 = s.Add( 5 );
		// should be { 10, 12 }
		assert( std::accumulate( r2.begin(), r2.end(), 0 ) == 22 );

		con.disconnect();

		auto r3 = s.Add( 5 );
		// should have lost above connection - return to { 10 }
		assert( std::accumulate( r3.begin(), r3.end(), 0 ) == 10 );

		con = s.sigadd.connect( another_add );
		auto r4 = s.Add( 5 );
		// should be { 10, 12 }
		assert( std::accumulate( r4.begin(), r4.end(), 0 ) == 22 );
	}

	{
		auto r1 = s.Add( 5 );
		// should have lost above connection - return to { 10 }
		assert( std::accumulate( r1.begin(), r1.end(), 0 ) == 10 );
	}

	auto r2 = s.Sub( 3 );
	for ( auto r : r2 )
		std::cout << r << "\n";

	return 0;
}
