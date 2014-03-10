#include "callback.hpp"

#include <memory>
#include <iostream>

#include <cassert>

int foo( int x, int y )
{
	return x - y + 31337;
}

struct Foo
{
	int Bar( int x, int y )
	{
		return x + y + 31337;
	}
};

void basic_func()
{
	pac::callback<int( int, int )> cb( foo );

	assert( cb( 1, 1 ) == foo( 1, 1 ) );
}

void basic_memfunc()
{
	Foo f;
	pac::callback<int( int, int )> cb( &f, &Foo::Bar );

	assert( cb( 1, 1 ) == f.Bar( 1, 1 ) );

	Foo *f2 = new Foo;
	pac::callback<int( int, int )> cb2( f2, &Foo::Bar );

	assert( cb2( 1, 1 ) == f2->Bar( 1, 1 ) );
	delete f2;

	std::unique_ptr<Foo> f3( new Foo );
	pac::callback<int( int, int )> cb3( f3, &Foo::Bar );

	assert( cb3( 1, 1 ) == f3->Bar( 1, 1 ) );

	std::cout << "f3 @ " << &f3 << "\n";

	std::shared_ptr<Foo> f4( new Foo );
	pac::callback<int( int, int )> cb4( f4, &Foo::Bar );
	std::cout << f4.use_count() << "\n";

	assert( cb4( 1, 1 ) == f4->Bar( 1, 1 ) );
}

struct Functor
{
	int operator()(int x, int y )
	{
		return x - y + 1337;
	}
};

void functor_test()
{
	std::function< int( int, int )> func = foo;
	pac::callback< int( int, int )> cb( func );

	assert( cb( 1, 1 ) == func( 1, 1 ) );

	auto bound = std::bind( func, std::placeholders::_1,
	                        std::placeholders::_2 );
	pac::callback< int( int, int )> cb2( bound );

	assert( cb2( 1, 1 ) == bound( 1, 1 ) );

	Functor f;
	pac::callback< int( int, int )> cb3( f );

	assert( cb3( 1, 1 ) == Functor()( 1, 1 ) );
}

void lambda_test()
{
	auto func = [](int x, int y) { return x + y + 200; };
	pac::callback< int( int, int )> cb( func );

	assert( cb( 1, 1 ) == func( 1, 1 ) );
}

void makecallback_test()
{
	Foo f;
	std::unique_ptr<Foo> f2( new Foo );

	auto cb = pac::make_callback( &f, &Foo::Bar );

	auto cb2 = pac::make_callback( foo );

	assert( cb2( 1, 2 ) == foo( 1, 2 ) );
}

template<class T>
T template_testme( T t )
{
	return t;
}

void templated_test()
{
	pac::callback<int( int )> cb2( template_testme<int> );
	pac::callback<int( int )> cb3( &template_testme<int> );
}

struct base
{
	virtual int op() { return 31; }
};

struct derived : base
{
	virtual int op() { return 39; }
};

void virtual_test()
{
	base b;
	derived d;

	pac::callback< int() > cb1( &b, &base::op );
	pac::callback< int() > cb2( &d, &base::op );

	pac::callback< int() > cb3( std::bind( &base::op, &b ) );
	pac::callback< int() > cb4( std::bind( &base::op, &d ) );

	assert( cb1() == b.op() );
	assert( cb2() == d.op() );

	assert( cb3() == b.op() );
	assert( cb4() == d.op() );

	assert( cb1() != cb2() );
	assert( cb1() == cb3() );
}

int main(int argc, char *argv[])
{
	basic_func();

	basic_memfunc();

	functor_test();

	lambda_test();

	makecallback_test();

	templated_test();

	virtual_test();

	std::cout << "Success: All tests passed!\n";

	return 0;
}
