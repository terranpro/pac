#include "signal.hpp"

#include <iostream>

using namespace pac;

struct Server
{
	int even = 0;

	pac::signal<bool(int)> sigconnect;
	pac::signal<bool(float)> sigdisconnect;

	void Update()
	{
		++even;
		if ( even % 2 == 0 && even < 10 )
			sigconnect.emit( even );
	}

	void Disconnect()
	{
		sigdisconnect.emit( 3.1415 );
	}
};

struct Client
{
	Server& server;
	connection con_con;
	connection con_dis;

	Client(Server& s)
		: server(s)
	{
		con_con =
			server.sigconnect.connect(
				this, &Client::OnConnect);
		BARK;
		con_dis =
			server.sigdisconnect.connect(
				this, &Client::OnDisconnect);
	}

	bool OnConnect(int)
	{
		std::cout << "Connected!\n";

		{
			connection_block block( con_con );
			server.Update();
			std::cout << "BERP!\n";
			server.Update();
			BARK;
		}

		// BARK;
		std::cout << "DIS the CON\n";
		server.sigconnect.disconnect(con_con);

		// BARK;
		// server.Update();
		// server.Update();

		return true;
	}

	bool OnDisconnect(float)
	{
		std::cout << "Disconnected!\n";
		return false;
	}
};

int main(int argc, char *argv[])
{
	Server s;
	Client *c = new Client{s};

	BARK;
	BARK;
	s.Update();

	s.Update();
	//s.sigconnect.disconnect(c->con_con);

	s.Disconnect();

	{
		connection_block block( c->con_dis );
		s.Disconnect();
	}

	delete c;

	BARK;
	s.Update();
	BARK;

	s.Disconnect();

	return 0;
}
