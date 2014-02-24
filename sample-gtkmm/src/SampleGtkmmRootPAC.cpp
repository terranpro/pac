#include "SampleGtkmmRootPAC.hpp"

#include <gtkmm.h>

#include <string>

#include "signal.hpp"

struct RootWindow : public Gtk::Window
{
	Gtk::Button button;
	using SignalButtonClickedType = pac::signal<void()>;
	SignalButtonClickedType button_clicked;

	RootWindow()
		: Gtk::Window(), button( "OK" ), button_clicked()
	{
		add( button );
		auto callback = sigc::mem_fun( *this,
		                               &RootWindow::OnButtonClicked );
		button.signal_clicked()
			.connect( callback );
		button.show();
		button.signal_draw().connect(
			sigc::mem_fun( *this,
			               &RootWindow::OnButtonDraw ) );
	}

	void OnButtonClicked()
	{
		button_clicked.emit();
	}

	bool OnButtonDraw(::Cairo::RefPtr<::Cairo::Context> const& c)
	{
		static Gdk::RGBA color("gray");
		double min = -0.01;
		double max = 0.01;

		auto delta_r = g_random_double_range( min, max );
		auto delta_g = g_random_double_range( min, max );
		auto delta_b = g_random_double_range( min, max );
		auto delta_a = g_random_double_range( min, max );
		auto red = color.get_red();
		auto green = color.get_green();
		auto blue = color.get_blue();
		auto alpha = color.get_alpha();

		red += delta_r;
		green += delta_g;
		blue += delta_b;
		//		alpha += delta_a;

		if ( red < 0 ) red = 0.0;
		if ( green < 0 ) green = 0.0;
		if ( blue < 0 ) blue = 0.0;
		if ( red > 1. ) red = 1.;
		if ( green > 1. ) green = 1.;
		if ( blue > 1. ) blue = 1.;

		if ( alpha < 0 ) alpha = 0;
		if ( alpha > 1 ) alpha = 1;

		color.set_rgba( red, green, blue, alpha );
		button.override_background_color( color );
		return false;
	}
};

struct RootAbstraction
{

};

struct RootPresentation : public sigc::trackable
{
	Glib::RefPtr<Gtk::Application> app;
	std::unique_ptr<RootWindow> window;
	std::string appid;

	RootPresentation()
		: app{}, window{}, appid{ "org.sample-gtkmm.app" }
	{}

	typename RootWindow::SignalButtonClickedType&
	SignalButtonClicked()
	{
		return window->button_clicked;
	}

	void on_activated()
	{
		app->add_window( *window );
		window->show();
	}

	void load_internal()
	{
		window.reset( new RootWindow );

		auto callback =
			sigc::mem_fun( *this,
			               &RootPresentation::on_activated );
		app->signal_activate().connect( callback );
	}

	void load()
	{
		int argc = 0;
		char *argv[] = { (char*)"sample-gtkmm.exe" , NULL };

		app = Gtk::Application::create(
			argc,
			reinterpret_cast<char **&>( argv ),
			appid,
			Gio::APPLICATION_HANDLES_OPEN );

		load_internal();
	}

	void load( int& argc, char **& argv )
	{
		app = Gtk::Application::create(argc, argv, appid,
		                               Gio::APPLICATION_HANDLES_OPEN );
		load_internal();
	}

	void run()
	{
		app->run();
	}

	void run( int& argc, char **& argv )
	{
		app->run( argc, argv );
	}
};

struct RootController
{
	//ExecutionContext& context;
	RootPresentation *pre;
	RootAbstraction *abs;

	RootController( RootPresentation *p,
	                RootAbstraction *a )
		: pre(p), abs(a)
	{}

	void signal_init()
	{
		pre->SignalButtonClicked().connect(
			this, &RootController::OnButtonClicked );
	}

	void OnButtonClicked()
	{
		g_print( "Button Clicked!\n" );
	}

	void RecvMsg()
	{}

	void SendMsg()
	{}
};

struct SampleGtkmmRootPACImpl
{
	RootPresentation presentation;
	RootAbstraction abstraction;
	RootController controller;

	SampleGtkmmRootPACImpl()
		: presentation{}, abstraction{},
		  controller{ &presentation, &abstraction }
	{}

	void load()
	{
		presentation.load();
		controller.signal_init();
	}

	void load( int& argc, char **& argv )
	{
		presentation.load( argc, argv );
		controller.signal_init();
	}

	void run()
	{
		presentation.run();
	}

	void run( int& argc, char **& argv )
	{
		presentation.run( argc, argv );
	}
};

SampleGtkmmRootPAC::SampleGtkmmRootPAC()
	: impl( new SampleGtkmmRootPACImpl )
{}

SampleGtkmmRootPAC::~SampleGtkmmRootPAC() = default;

void SampleGtkmmRootPAC::load()
{
	impl->load();
}

void SampleGtkmmRootPAC::load( int&argc, char **&argv )
{
	impl->load( argc, argv );
}

void SampleGtkmmRootPAC::run()
{
	impl->run();
}

void SampleGtkmmRootPAC::run( int& argc, char **& argv )
{
	impl->run( argc, argv );
}
