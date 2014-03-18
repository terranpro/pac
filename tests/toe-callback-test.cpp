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

struct connected_widget
{
	pac::signal<void()> connected_sig;

	void on_engine_connected()
	{
		std::cout << "Connected!\n";
		connected_sig.emit();
	}
};

struct root_controller
{
	using source_signal = decltype( engine::sigconnected );

	engine eng;
	pac::toe toe;
	pac::signal_forward< source_signal, void(void) > sigvoidfwd;
	connected_widget widget;
	bool widget_said_hello = false;

	root_controller()
		: eng(),
		  sigvoidfwd( eng.sigconnected, void_inf, void_outf ),
		  widget()
	{
		widget.connected_sig.connect( [&](){ this->widget_said_hello = true; quit(); } ).detach();
		notify_connected( std::bind(&connected_widget::on_engine_connected, &widget) ).detach();
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

int main(int argc, char *argv[])
{
	root_controller rcon;

	rcon.start();

	std::this_thread::sleep_for( std::chrono::seconds(1) );

	std::cout << "Finished!\n";

	return 0;
}
