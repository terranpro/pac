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
 * signal.hpp
 *
 * Author: Brian Fransioli
 * Created: Sun Feb 09 20:18:04 KST 2014
 * Last modified: Sun Feb 09 22:59:50 KST 2014
 */

#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <functional>
#include <vector>
#include <unordered_map>
#include <utility>
#include <memory>

//#include <iostream>

#include "callback.hpp"

namespace pac {

template<class T, class Ret, class... Args>
auto make_callback( T *obj, Ret (T::*mfunc)(Args...) )
	-> callback<Ret, Args...>
{
	callback<Ret, Args...> cb{obj, mfunc};
	return cb;
}

template<class Ret, class... Args>
auto make_callback( Ret (*func)(Args...) )
	-> callback<Ret, Args...>
{
	callback<Ret, Args...> cb{func};
	return cb;
}

using connection = std::size_t;

template< class Signature >
class signal;

template<class Ret, class... Args>
class signal<Ret(Args...)>
{
	using callback_type = callback<Ret, Args...>;

	std::unordered_map< connection, callback_type > callbacks;
	connection next_id = 0;

public:
	signal() = default;

	~signal()
	{}

	// signal(signal const&) = default;
	// signal& operator=(signal const&) = default;

	signal(signal&&) = default;
	signal& operator=(signal&&) = default;

	template<class Func>
	connection connect(Func func)
	{
		//auto con = make_callback( func );
		callback<Ret, Args...> cb{ func };
		callbacks.insert( std::make_pair(++next_id, cb) );
		return next_id;
	}

	template<class T, class PMemFunc>
	connection connect( T *obj, PMemFunc mfunc )
	{
		auto cb = make_callback( obj, mfunc );
		callbacks.insert( std::make_pair(++next_id, cb) );
		return next_id;
	}

	void disconnect( connection con )
	{
		auto it = callbacks.find( con );
		if ( it != std::end( callbacks ) )
			callbacks.erase( it );
	}

	void dispatch(Args&&... args)
	{
		for ( auto& ccb : callbacks )
			ccb.second( std::forward<Args>(args)... );
	}

	void emit(Args&&... args)
	{
		dispatch( std::forward<Args>(args)... );
	}
};

} // namespace pac

#endif // SIGNAL_HPP
