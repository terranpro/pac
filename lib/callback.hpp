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
 * Last modified: Mon Mar 03 00:37:37 KST 2014
 */

#ifndef CALLBACK_HPP
#define CALLBACK_HPP

#include <functional>
#include <utility>
#include <memory>

namespace pac {

template<class Ret, class... Args>
class callback
{
  struct concept
  {
    virtual ~concept()
    {}
    virtual Ret operator()(Args&&... args) = 0;
  };

  template<class Func>
  struct model : concept
  {
    Func func;

    Ret operator()(Args&&... args)
    {
	    return func( std::forward<Args>(args)... );
    }

    model(Func f)
	    : func( std::forward<Func>(f) )
    {}
  };

  template<class T, class PMemFunc>
  struct model_memfunc : concept
  {
    T *obj;
    PMemFunc mfunc;

    model_memfunc(T *o, PMemFunc m )
      : obj(o), mfunc(m)
    {}

    Ret operator()(Args&&... args)
	  {
	    return (obj->*mfunc)( std::forward<Args>(args)... );
    }
  };

  std::shared_ptr<concept> con;

public:
	using return_type = Ret;

public:
  ~callback()
  {}

  callback()
    : con( nullptr )
  {}

  template<class Func>
  callback(Func func)
	  : con( new model< Func >(func) )
	{}

  template<class T, class PMemFunc>
  callback(T *obj, PMemFunc memfunc)
    : con( new model_memfunc<T, PMemFunc>(obj, memfunc) )
	{}

	callback(callback&&) = default;
	callback& operator=(callback&&) = default;

	callback(callback const&) = default;
	callback& operator=(callback const&) = default;

  Ret operator()(Args&&... args)
	{
    if ( con )
	    return (*con)( std::forward<Args>(args)... );
    return Ret();
  }
};

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

} // namespace pac

#endif // CALLBACK_HPP
