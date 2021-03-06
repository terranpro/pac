#include "signal.hpp"
#include "signal-forward.hpp"

#include <iostream>
#include <algorithm>
#include <memory>

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
			server.sigadd.connect( &Client::OnAdd, this );
		con_sub =
			server.sigsub.connect( &Client::OnSub, this );
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

int sigfwd_cb( int x, int y )
{
	std::cout << "Point @ " << x << "," << y << "\n";
	return x + y;
}

int sigfwd2_cb( int x )
{
	auto ret = x + 111;
	return std::move(ret);
}

int main(int, char *[])
{
	Server s;
	auto c = std::make_shared<Client>( s );

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

	// just for fun, made the transform functions 'hard'
	auto infunc = std::bind( []( int in ) { return in != 0; }, std::placeholders::_1 );
	//	std::function<int(bool)> outfunc = []( bool in ) { return in ? 6969 : -1069; };
	auto outfunc = []( bool in ) { return in ? 6969 : -1069; };

	for ( auto r : s.Add( 5 ) )
		std::cout << "r = " << r << "\n";

	// forwarded slot test
	auto testn1 = 3;

	std::function< std::tuple<int,int>(int) > inf =
		[]( int in ) { return std::make_tuple( in + 3, in + 5 ); };

	pac::callback<int(int,int)> mycb =
		[]( int in1, int in2 ) { return in1 * in2 + 137; };

	// std::function< int( bool ) > outf =
	// 	[]( bool out ) { int r = out ? 1001 : 2002; return r; };
	pac::callback< int( int ) > outf =
		[]( int out ) { return out - 1; };

	// forwarded signal test
	auto testn2 = 5;
	pac::signal< int( int ) > origsig;

	for ( auto r : origsig.emit( testn2 ) )
		std::cout << "s = " << r << "\n";

	pac::signal_forward< decltype( origsig ),
	                     int( int,int ) >
		sigf( origsig, inf, outf );

	auto sigfcon = sigf.connect( sigfwd_cb );

	auto rr =
		[](std::vector<int> con)
		{
			return std::accumulate( con.begin(), con.end(), 0 );
		}(origsig.emit( testn2 ));

	auto expected_rr = sigfwd_cb( testn2 + 3, testn2 + 5 ) - 1;

	std::cout << rr << " ==? " << expected_rr << "\n";

	assert( rr == expected_rr );

	auto sigfcon2 = sigf.connect( sigfwd_cb );
	{
		pac::connection_block block
		{ sigfcon };

		auto rr2 =
			[](std::vector<int> con)
			{
				return std::accumulate( con.begin(), con.end(), 0 );
			}(origsig.emit( testn2 ));

		assert( rr2 == expected_rr );
	}

	sigfcon.disconnect();
	sigfcon.disconnect();
	sigfcon.disconnect();
	sigfcon.disconnect();
	sigfcon.disconnect();
	sigfcon.disconnect();
	sigfcon.disconnect();

	origsig.emit( testn2 );

	// test default fwd funcs
	pac::signal_forward< decltype(origsig ),
	                     int( int ) > sigf2( origsig );


	auto sigf2con = sigf2.connect( sigfwd2_cb );

	auto rr3 =
		[](std::vector<int> con)
		{
			return std::accumulate( con.begin(), con.end(), 0 );
		}( origsig.emit( testn2 ) );

	auto expected_rr3 =
		expected_rr + sigfwd2_cb( testn2 );

	assert( rr3 == expected_rr3 );

	// void parameter signal fwd test - this one's tricky
	pac::callback<void( void )> void_inf = [](){};
	pac::callback<int()> void_outf = [](){ return 1337; };

	pac::signal<int( void )> sigvoid;
	// pac::signal_forward< decltype( sigvoid ), void() > sigvoidfwd(
	// 	sigvoid );

	pac::signal_forward< decltype( sigvoid ), void() > sigvoidfwd(
		sigvoid,
		void_inf,
		void_outf );

	//	pac::callback<void()> void_usercb =
	auto void_usercb =
		[](){ std::cout << "Amazing?!\n"; };

	auto sigvoid_con = sigvoidfwd.connect( void_usercb );

	auto void_res = sigvoid.emit();
	for ( auto& v : void_res )
		std::cout << "v = " << v << "\n";

	assert( void_res[0] == 1337 );

	// signal forward int(void) case:
	pac::signal< int( void ) > s99;
	pac::signal_forward< decltype( s99 ), int( void ) > s99fwd( s99 );

	// signal connect using pac::callback
	pac::signal< int( char, char, std::string ) > s66;
	pac::callback< int( char, char, std::string ) > cb66 =
		[]( char, char, std::string str )
		{
			std::cout << str << " : Bitchin'\n";
			return 1;
		};

	auto con66 = s66.connect( cb66 );
	s66.emit( 'c', '+', "c++" );

	// multi signal forward test
	pac::signal< void() > srcsig;
	pac::signal_forward< decltype(srcsig), void() > fwd1( srcsig );
	pac::signal_forward< decltype(fwd1), void() > fwd2( fwd1 );
	pac::signal_forward< decltype(fwd2), void() > fwd3( fwd2 );
	pac::signal_forward< decltype(fwd3), void() > fwd4( fwd3 );

	pac::callback< void() > getemcb = []() { std::cout << "GET EM!\n"; };
	auto getemcon = fwd4.connect( getemcb );
	fwd2.connect(getemcb).detach();

	pac::signal_forward< decltype(fwd4), int() > fwd5( fwd4 );
	pac::callback< int() > getitcb = []() { std::cout << "GET IT!\n"; return 99; };
	fwd5.connect(getitcb).detach();

	srcsig.emit( );

	return 0;
}
