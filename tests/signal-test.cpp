#include "signal.hpp"
#include "signal-forward.hpp"

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

bool sigfwd_cb( double x, double y )
{
	std::cout << "Point @ " << x << "," << y << "\n";
	return x > y;
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

	// std::function<void()> transformer =
	// 	[](){ };

	// signal forwarding tests
	// signal_forward< pac::signal<int(int)>, void(bool) > fwd( s.sigadd,
	//                                                          transformer
	//                                                        );

	// just for fun, made the transform functions 'hard'
	auto infunc = std::bind( []( int in ) { return in != 0; }, std::placeholders::_1 );
	//	std::function<int(bool)> outfunc = []( bool in ) { return in ? 6969 : -1069; };
	auto outfunc = []( bool in ) { return in ? 6969 : -1069; };
	signal_catcher< decltype( s.sigadd ),
	                decltype(infunc),
	                decltype(outfunc) >
		catcher( s.sigadd, infunc, outfunc );

	for ( auto r : s.Add( 5 ) )
		std::cout << "r = " << r << "\n";

	// forwarded slot test

	std::function< std::tuple<double,double>(int) > inf =
		[]( int in ) { BARK; return std::make_tuple( in * 3.14, in * 2 * 3.14 ); };

	pac::callback<bool,double,double> mycb =
		[]( double in1, double in2 ) { return in1 > in2; };

	std::function< int( bool ) > outf =
		[]( bool out ) { BARK; int r = out ? 1001 : 2002; return r; };


	pac::forwarded_slot< decltype( mycb ), decltype( inf ), decltype( outf ) >
		fslot( mycb, inf, outf );

	std::cout << pac::apply( mycb, std::make_tuple( 3.141, 3.14 ) ) << "\n";

	auto *castslot =
		dynamic_cast<pac::slot< callback<int, int> > *>( &fslot );

	assert( castslot );

	std::cout << castslot->callback( 99 ) << "\n";

	pac::signal< int( int ) > origsig;

	auto savemecon = origsig.connect_slot( *castslot );

	for ( auto r : origsig.emit( 5 ) )
		std::cout << "s = " << r << "\n";

	pac::signal_forward< decltype( origsig ),
	                     bool( double,double ),
	                     decltype( inf ),
	                     decltype( outf ) > sigf( origsig,
	                                              inf,
	                                              outf );

	// This works
	auto sigfcon = sigf.connect_slot( *castslot );

	// But this is broken currently
	//auto sigfcon = sigf.connect( sigfwd_cb );

	origsig.emit( 5 );

	return 0;
}
