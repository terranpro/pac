#include <memory>
#include <thread>

#include <cassert>

#include "context.hpp"
#include "signal.hpp"
#include "runnable.hpp"
#include "signal-forward.hpp"

struct engine
{
	pac::signal< void(int) > sigconnected;
	pac::toe toe;

	engine() :
		sigconnected(), toe()
	{}

	void connect()
	{
		toe.launch( pac::toe::async );
		toe.add_callback( pac::callback<void()>( &engine::on_connected, this ) );
	}

	void on_connected()
	{
		std::cout << "engine::on_connected!\n";
		sigconnected.emit( 1 );
	}
};

struct connected_controller;
struct connected_widget;
struct root_controller;

struct connected_widget
{
	connected_controller& con;
	pac::signal<void()> connected_sig;

	connected_widget( connected_controller& c );

	void on_engine_connected()
	{
		std::cout << "Connected!\n";
		connected_sig.emit();
	}
};

struct connected_controller
{
	root_controller& parent;
	pac::toe& toe;
	pac::signal_forward< pac::signal<void(int)>, void() > sigfwd;
	connected_widget widget;
	pac::signal_forward< pac::signal<void()>, void() > sigfwdwidget;

	connected_controller( root_controller&p, pac::toe& t );

	template<class Func>
	pac::connection notify_connected( Func func, pac::toe *con_toe = nullptr  )
	{
		if ( !con_toe )
			con_toe = &toe;

		pac::callback<void()> toecb =
			pac::toe_callback( *con_toe, pac::callback< void() >(func) );

		return sigfwd.connect( toecb );
	}
};

struct root_controller
{
	using source_signal = decltype( engine::sigconnected );

	engine eng;
	pac::toe toe;
	pac::signal_forward< source_signal, void() > sigvoidfwd;
	connected_controller con;
	bool widget_said_hello = false;

	root_controller()
		: eng(),
		  sigvoidfwd( eng.sigconnected, void_inf, void_outf ),
		  con(*this, toe)
	{
		con.sigfwdwidget.connect( [&](){ this->widget_said_hello = true; quit(); } ).detach();
		//notify_connected( std::bind(&connected_widget::on_engine_connected, &widget) ).detach();
	}

	static void void_inf(int) {}
	static void void_outf() {}

	template<class Func>
	pac::connection notify_connected( Func func, pac::toe *con_toe = nullptr  )
	{
		if ( !con_toe )
			con_toe = &toe;

		pac::callback<void()> toecb =
			pac::toe_callback( *con_toe, pac::callback< void() >(func) );

		return sigvoidfwd.connect( toecb );
	}

	void start()
	{
		eng.connect();
		toe.launch();

		assert( widget_said_hello );
	}

	void quit()
	{
		toe.quit();
	}
};

connected_widget::connected_widget( connected_controller& c )
	: con(c)
{
	con.notify_connected( std::bind( &connected_widget::on_engine_connected, this ) ).detach();
}

connected_controller::connected_controller( root_controller&p, pac::toe& t )
	: parent( p ), toe( t ), sigfwd( parent.sigvoidfwd ), widget( *this ), sigfwdwidget( widget.connected_sig )
{}


int main(int argc, char *argv[])
{
	root_controller rcon;

	rcon.start();

	std::this_thread::sleep_for( std::chrono::seconds(1) );

	std::cout << "Finished!\n";

	return 0;
}
