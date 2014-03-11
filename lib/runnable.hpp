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
 * Last modified: Tue Mar 11 18:48:40 KST 2014
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

class runnable
{
	struct runnable_concept
	{
		virtual ~runnable_concept() {}

		virtual runnable_status operator()() const = 0;
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

		virtual runnable_status operator()() const
		{
			apply( cb, args );
			return runnable_status::CONTINUING;
		}
	};

	std::shared_ptr< runnable_concept > rcon;

public:
	template<class Callback, class... Args>
	runnable( Callback&& cb, Args&&... args )
		: rcon( std::make_shared<runnable_model<Callback, Args...>>(
			        std::forward<Callback>(cb),
			        std::forward<Args>(args)...) )
	{}

	runnable_status run() const
	{
		return (*rcon)();
	}

};

} // namespace pac

#endif // RUNNABLE_HPP
