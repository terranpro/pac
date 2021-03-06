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
 * callback.hpp
 *
 * Author: Brian Fransioli
 * Created: Sun Feb 09 20:15:18 KST 2014
 * Last modified: Tue Sep 16 14:02:35 KST 2014
 */

#ifndef CALLBACK_HPP
#define CALLBACK_HPP

#include <functional>
#include <utility>
#include <memory>

namespace pac {

template<class Signature>
class callback;

template<class Ret, class... Args>
class callback< Ret(Args...) >
{
	struct concept
	{
		virtual ~concept() {}
		virtual Ret operator()(Args... args) = 0;
	};

	template<class Func>
	struct model : concept
	{
		Func func;

		Ret operator()(Args... args)
		{
			return (func)( std::forward<Args>(args)... );
		}

		model(Func f)
			: func(f)
		{}
	};

	template<class PMemFunc, class T>
	struct model_memfunc : concept
	{
		T obj;
		PMemFunc mfunc;

		template<class M, class U>
		model_memfunc( M&& m, U&& o )
			: obj( std::forward<U>(o) ), mfunc( std::forward<M>(m) )
		{}

		Ret operator()(Args... args)
		{
			// Use (*obj).*mfunc for smart pointers (e.g. unique_ptr)
			return ( (*obj).*mfunc )( std::forward<Args>(args)... );
		}
	};

	std::shared_ptr<concept> con;

public:
	~callback() = default;

	callback()
		: con( nullptr )
	{}

	template<class Func>
	callback(Func func)
		: con( std::make_shared<model< Func >>(func) )
	{}

	template<class PMemFunc, class T>
	callback(PMemFunc memfunc, T&& obj)
		: con( std::make_shared<model_memfunc<PMemFunc, T>>( memfunc, std::forward<T>(obj) ) )
	{}

	callback(callback&&) = default;
	callback& operator=(callback&&) = default;

	callback(callback const&) = default;
	callback& operator=(callback const&) = default;

	Ret operator()(Args... args)
	{
		if ( con )
			return (*con)( std::forward<Args>(args)... );
		return Ret();
	}
};

template<class T, class Ret, class... Args>
auto make_callback( Ret (T::*mfunc)(Args...), T *obj )
	-> callback<Ret( Args... )>
{
	callback<Ret( Args... )> cb{ mfunc, obj };
	return cb;
}

template<class Ret, class... Args>
auto make_callback( Ret (*func)(Args...) )
	-> callback<Ret( Args... )>
{
	callback<Ret( Args... )> cb{func};
	return cb;
}

} // namespace pac

#endif // CALLBACK_HPP
