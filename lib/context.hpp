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
 * Last modified: Wed Mar 19 16:51:50 KST 2014
 */

#ifndef PAC_CONTEXT_HPP
#define PAC_CONTEXT_HPP

#include "runnable.hpp"
#include "signal.hpp"

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
	context()
		: runnables{}, cid{}, tid{}
	{}

	static context_ptr create()
	{
		return std::make_shared<context>();
	}

	runnable_ptr next_runnable()
	{
		if ( runnables.empty() )
			return {};

		auto run = runnables.front();
		runnables.pop_front();

		return run;
	}

	std::size_t runnable_count()
	{
		return runnables.size();
	}

	void add_runnable( runnable_ptr run )
	{
		runnables.push_back( run );
	}

	template<class Callback, class... Args>
	void add_callback( Callback callback, Args&&... args )
	{
		runnable_ptr run =
			std::make_shared<runnable>( callback, std::forward<Args>(args)... );

		run->set_once();
		add_runnable( run );
	}

	void reset()
	{
		runnables.clear();
	}

	void set_thread_id( thread_id id )
	{
		tid = id;
	}

	void set_thread_id()
	{
		tid = std::this_thread::get_id();
	}

	thread_id get_thread_id() const
	{
		return tid;
	}

	static thread_id current_thread_id()
	{
		return std::this_thread::get_id();
	}
};

struct context_invoker
{
	using context_ptr = context::context_ptr;

	context_ptr ctxt;

	context_invoker( context_ptr c )
		: ctxt(c)
	{
		ctxt->set_thread_id();
	}

	bool iterate(std::mutex& mutex)
	{
		std::unique_lock<std::mutex> lock( mutex );
		auto nextrun = ctxt->next_runnable();
		lock.unlock();

		if (!nextrun)
			return false;

		auto res = nextrun->run();

		if ( res == runnable_status::CONTINUING ) {
			lock.lock();
			ctxt->add_runnable( nextrun );
			lock.unlock();
		}

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

		for ( bool finished = false; !finished; ) {

			if ( pauseme )
				handle_pause();

			if ( quitme )
				break;

			auto res = inv.iterate(mutex);
			if ( !res ) {
				idle( [&](){ return !quitme && ctxt->runnable_count() == 0; } );
			}

		}

		handle_quit();
	}

	bool is_toe_context()
	{
		return ctxt->get_thread_id() == context::current_thread_id();
	}

	void idle()
	{
		std::unique_lock<std::mutex> lock( mutex );
		cond.wait_for( lock, std::chrono::milliseconds(10) );
	}

	template<class Condition>
	void idle( Condition cond )
	{
		while( cond() )
			idle();
	}

	void wake()
	{
		std::lock_guard<std::mutex> lock( mutex );
		cond.notify_all();
	}

	void handle_pause()
	{
		if ( !is_toe_context() )
			return;

		while( pauseme == true ) {
			idle();
		}
	}

	void handle_resume()
	{
		wake();
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
		ctxt{ context::create() }, mutex{}, pauseme{false},
		quitme{false}, cond{}, thr{}
	{}

	toe( context_ptr c )
		: ctxt{ c }, mutex{}, pauseme{false}, quitme{false},
		  cond{}, thr{}
	{}

	~toe()
	{
		quit();
		join();
	}

	void set_context( context_ptr c )
	{
		using std::swap;
		swap( ctxt, c );
	}

	void launch( launch_type t = sync )
	{
		if ( t == sync )
			run();
		else if ( t == async )
			thr = std::thread( &toe::run, this );
	}

	void pause()
	{ pauseme = true; handle_pause(); }

	void resume()
	{ pauseme = false; handle_resume(); }

	void quit()
	{ quitme = true; resume(); }

	void join()
	{
		if ( joinable() )
			thr.join();
	}

	bool joinable()
	{
		return thr.joinable();
	}

	void sleep_for( long long time_us )
	{
		if ( is_toe_context() )
			std::this_thread::sleep_for( std::chrono::microseconds( time_us ) );
	}

	template<class Callback, class... Args>
	void add_callback( Callback callback, Args&&... args )
	{
		{
			std::lock_guard<std::mutex> lock( mutex );
			ctxt->add_callback( callback, std::forward<Args>(args)... );
		}
		wake();
	}
};

template<class Ret, class... Args, class RetGenerator = Ret>
auto toe_callback( toe& toe, pac::callback<Ret(Args...)> cb )
{
	auto stubfunc = [&toe, cb]( Args... args )
		{
			toe.add_callback( cb, std::forward<Args>(args)... );
			return RetGenerator();
		};

	pac::callback<Ret(Args...)> stubcb( stubfunc );
	return stubcb;
}

} // namespace pac

#endif // PAC_CONTEXT_HPP
