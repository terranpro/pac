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
 * runnable.hpp
 *
 * Author: Brian Fransioli
 * Created: Tue Mar 11 17:40:26 KST 2014
 * Last modified: Sun Mar 16 19:22:49 KST 2014
 */

#ifndef RUNNABLE_HPP
#define RUNNABLE_HPP

#include <memory>
#include <tuple>
#include <string>
#include <map>

#include "callback.hpp"
#include "apply.hpp"

namespace pac {

enum class runnable_status
{
	INVALID,
	CONTINUING,
	FINISHED,
	ABORT,
	INTERRUPTED
};

std::string as_string( runnable_status const& r )
{
	static std::map< runnable_status,
	                 std::string > runstrmap =
		{
			{ runnable_status::INVALID, "INVALID" },
			{ runnable_status::CONTINUING, "CONTINUING" },
			{ runnable_status::FINISHED, "FINISHED" },
			{ runnable_status::ABORT, "ABORT" },
			{ runnable_status::INTERRUPTED, "INTERRUPTED" },
		};

	auto x = runstrmap.find( r );
	if ( x == std::end( runstrmap ) )
		return "";

	return x->second;
}

struct runnable_context
{
	bool once;
	runnable_status status;

	runnable_context()
		: once( false )
		, status( runnable_status::CONTINUING )
	{}

	runnable_context( bool once_ )
		: once( once_ )
		, status( once ? runnable_status::FINISHED : runnable_status::CONTINUING )
	{}
};

class runnable
{
	struct runnable_concept
	{
		virtual ~runnable_concept() {}

		virtual void operator()() = 0;
	};

	template<class Callback, class... Args>
	struct runnable_model : runnable_concept
	{
		Callback cb;
		std::tuple<Args...> args;

		template<class C, class... A>
		runnable_model( C&& c, A&&... a )
			: cb( std::forward<C>(c) )
			, args( std::forward<A>(a)... )
		{}

		virtual void operator()()
		{
			apply( cb, args );
		}
	};

	std::shared_ptr< runnable_concept > rcon;
	runnable_context rctxt;

public:
	runnable()
		: rcon{}
	{}

	// Take Callback by value as its a handle (pac::callback<>)
	// Otherwise, type deduction will resolve to a lvalue ref, and a ref
	// to a local stack callback could be kept dangling and invoked
	// later
	template<class Callback, class... Args>
	runnable( Callback cb, Args&&... args )
		: rcon( std::make_shared<runnable_model<Callback, Args...>>(
			        cb, std::forward<Args>(args)...) ),
		  rctxt()
	{}

	void set_once()
	{
		rctxt.once = true;
		rctxt.status = runnable_status::FINISHED;
	}

	runnable_status run()
	{
		if ( !rcon )
			return runnable_status::INVALID;

		(*rcon)();

		return rctxt.status;
	}

};

} // namespace pac

#endif // RUNNABLE_HPP
