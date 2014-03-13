/*
 * This file is part of PAC
 *
 * PAC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PAC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PAC.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * context.hpp
 *
 * Author: Brian Fransioli
 * Created: Mon Mar 10 17:30:53 KST 2014
 * Last modified: Thu Mar 13 23:56:23 KST 2014
 */

#ifndef PAC_CONTEXT_HPP
#define PAC_CONTEXT_HPP

#include "runnable.hpp"

#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <atomic>

namespace pac {

class context
{
public:
	using runnable_ptr = std::shared_ptr< runnable >;
	using runnable_cont = std::list< runnable_ptr >;
	using runnable_iter = typename runnable_cont::iterator;

	using context_id = std::size_t;
	using context_ptr = std::shared_ptr< context >;

	using thread_id = std::thread::id;

private:
	runnable_cont runnables;
	context_id cid;
	thread_id tid;

public:
	static context_ptr create()
	{
		return std::make_shared< context >();
	}

	runnable_ptr next_runnable()
	{
		if ( runnables.empty() )
			return {};

		auto run = runnables.front();
		runnables.pop_front();

		return run;
	}

	void add_runnable( runnable_ptr run )
	{
		runnables.push_back( run );
	}

	void reset()
	{
		runnables.clear();
	}
};


struct context_invoker
{
	using context_ptr = context::context_ptr;

	context_ptr ctxt;

	context_invoker( context_ptr c )
		: ctxt(c)
	{}

	bool iterate()
	{
		auto nextrun = ctxt->next_runnable();

		if (!nextrun)
			return false;

		auto res = nextrun->run();

		if ( res == runnable_status::CONTINUING )
			ctxt->add_runnable( nextrun );

		return true;
	}

};

class toe
{
	using context_ptr = context::context_ptr;

	context_ptr ctxt;
	std::mutex mutex;
	std::atomic<bool> pauseme;
	std::atomic<bool> quitme;
	std::condition_variable cond;
	std::thread thr;

	void run()
	{
		context_invoker inv( ctxt );

		for ( bool finished = false;
		        !finished;
		    ) {

			if ( pauseme )
				handle_pause();

			if ( quitme )
				break;

			finished = !inv.iterate();
		}

		handle_quit();
	}

	void handle_pause()
	{
		std::unique_lock<std::mutex> lock( mutex );
		while( pauseme == true )
			cond.wait( lock );
	}

	void handle_resume()
	{
		cond.notify_all();
	}

	void handle_quit()
	{}

public:
	enum launch_type {
		sync,
		async
	};

public:
	toe() :
		ctxt{}, mutex{}, pauseme{false}, quitme{false},
		cond{}, thr{}
	{}

	~toe()
	{
		quit();
		join();
	}

	void launch( context_ptr c, launch_type t = sync )
	{
		using std::swap;
		swap( ctxt, c );

		if ( t == sync )
			run();
		else if ( t == async )
			thr = std::thread( &toe::run, this );
	}

	void pause()
	{ pauseme = true; }

	void resume()
	{ pauseme = false; handle_resume(); }

	void quit()
	{ quitme = true; }

	void join()
	{
		if ( joinable() )
			thr.join();
	}

	bool joinable()
	{
		return thr.joinable();
	}
};

} // namespace pac

#endif // PAC_CONTEXT_HPP
