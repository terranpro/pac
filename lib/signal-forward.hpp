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
 * signal-forward.hpp
 *
 * Author: Brian Fransioli
 * Created: Mon Feb 24 19:51:40 KST 2014
 * Last modified: Sat Mar 01 19:50:24 KST 2014
 */

#ifndef SIGNAL_FORWARD_HPP
#define SIGNAL_FORWARD_HPP

#include "callback.hpp"
#include "signal.hpp"

namespace pac {

template<class Signal, class InFunc, class OutFunc>
class signal_catcher;

template<class Ret, class... Args, class InFunc, class OutFunc>
class signal_catcher< signal< Ret(Args...)>, InFunc, OutFunc >
{
	connection con;
	InFunc infunc;
	OutFunc outfunc;

	auto operator()(Args&&... args)
		-> decltype( outfunc( infunc( std::forward<Args>(args)... ) ) )
	{
		std::cout << __PRETTY_FUNCTION__ << "\n";
		return outfunc( infunc( std::forward<Args>(args)... ) );
	}

public:
	signal_catcher(signal<Ret(Args...)>& s, InFunc inf, OutFunc outf)
		: infunc(inf), outfunc(outf)
	{
		con = s.connect( this, &signal_catcher::operator() );
	}

};

template<class... InArgs, class TFunc, class... OutArgs>
std::tuple<OutArgs...> transform( std::tuple<InArgs...> in_args,
                                  TFunc tfunc )
{
	return tfunc( in_args );
}

template<class Signal, class InFunc, class OutFunc>
class signal_forward;

template<class Signal, class InRet, class... InArgs, class OutRet, class... OutArgs>
class signal_forward<Signal, InRet(InArgs...), OutRet(OutArgs...) >
{
	using UserSignalType = signal< InRet(InArgs...) >;
	using UserSignalResultsType = typename UserSignalType::results_type;

	UserSignalType sig_forwarded;
	signal_catcher< Signal, std::function<InRet(InArgs...)>, std::function<OutRet(OutArgs...)> > catcher;

	signal_forward( Signal& sig )
	{}

	template<class InFunc, class OutFunc>
	signal_forward( Signal& sig, InFunc inf, OutFunc outf )
		: catcher( sig, inf, outf )
	{
	}

	template<class Func>
	connection connect( Func func )
	{
		return sig_forwarded.connect( func );
	}

	template<class T, class PMemFunc>
	connection connect( T *obj, PMemFunc mfunc )
	{
		return sig_forwarded.connect( obj, mfunc );
	}

	void disconnect( connection& con )
	{
		sig_forwarded.disconnect( con );
	}

	void disconnect( std::size_t con_id )
	{
		sig_forwarded.disconnect( con_id );
	}

	template<class... Args>
	UserSignalResultsType emit(Args... args)
	{
		return sig_forwarded.emit( args... );
	}
};


} // namespace pac

#endif // SIGNAL_FORWARD_HPP
