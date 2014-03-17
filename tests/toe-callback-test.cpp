#include <memory>
#include <thread>

#include "context.hpp"
#include "signal.hpp"
#include "runnable.hpp"
#include "signal-forward.hpp"

struct engine
{
	pac::signal< void(int) > sigconnected;
	std::shared_ptr<pac::context> context;
	pac::toe toe;

	engine() :
		sigconnected(), context( pac::context::create() ), toe()
	{}

	void connect()
	{
		toe.launch( context, pac::toe::async );
		context->add_callback( pac::callback<void()>( &engine::on_connected, this ) );
	}

	void on_connected()
	{
		std::cout << "engine::on_connected!\n";
		sigconnected.emit( 1 );
	}
};

struct root_controller
{
	using source_signal = decltype( engine::sigconnected );

	engine eng;
	std::shared_ptr<pac::context> context;
	pac::toe toe;
	pac::signal_forward< source_signal, void(void) > sigvoidfwd;

	root_controller()
		: eng(), context( pac::context::create() ),
		  sigvoidfwd( eng.sigconnected, void_inf, void_outf )
	{}

	static void void_inf(int) {}
	static void void_outf() {}

	template<class Func>
	pac::connection notify_connected( Func func, pac::context *ctxt = nullptr  )
	{
		if ( !ctxt )
			ctxt = context.get();

		pac::callback<void()> toecb =
			pac::toe_callback( toe, pac::callback< void() >(func) );

		return sigvoidfwd.connect( toecb );
	}

	void start()
	{
		eng.connect();
		toe.launch( context );
		//		std::this_thread::sleep_for( std::chrono::seconds(1) );
	}
};

void eng_on_connected()
{
	std::cout << "Connected!\n";
}

int main(int argc, char *argv[])
{
	root_controller rcon{};

	rcon.notify_connected( eng_on_connected ).detach();

	rcon.start();

	std::this_thread::sleep_for( std::chrono::seconds(1) );

	return 0;
}
