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
 * Last modified: Tue Mar 04 14:02:24 KST 2014
 */

#ifndef SIGNAL_FORWARD_HPP
#define SIGNAL_FORWARD_HPP

#include "callback.hpp"
#include "signal.hpp"
#include "apply.hpp"

namespace pac {

template<class Ret, class... Args>
struct null_func
{
	Ret operator()(Args&&... args)
	{
		return Ret();
	}
};

template<class Callback, class InFunc, class OutFunc>
struct forwarded_slot;

template<class Callback,
         template<class...> class FunctionIn,
         template<class...> class FunctionOut,
         class InRet, class... InArgs,
         class OutRet, class... OutArgs>
struct forwarded_slot<
         Callback,
         FunctionIn<InRet( InArgs... )>,
         FunctionOut<OutRet( OutArgs... )>
        >
	: public slot< callback<OutRet( InArgs... )> >
{
	Callback usercb;
	FunctionIn<InRet( InArgs... )> infunc;
	FunctionOut<OutRet( OutArgs... )> outfunc;

	template<class InFunc, class OutFunc>
	forwarded_slot( Callback cb, InFunc inf, OutFunc outf )
		: slot< callback< OutRet( InArgs... ) > >( callback< OutRet( InArgs... ) >{} ),
		  usercb( cb ), infunc( inf ), outfunc( outf )
	{
		this->callback = reset_callback();
	}

	auto reset_callback()
	{
		return
			[&](InArgs... args)
		{
			return outfunc( apply( usercb, std::move( infunc( std::move( args... ) ) ) ) );
		};
	}

	forwarded_slot( forwarded_slot const& other )
		: slot< callback< OutRet( InArgs... ) > >( other )
	{
		std::cout << "hey sup\n";

		usercb = other.usercb;
		infunc = other.infunc;
		outfunc = other.outfunc;

		this->callback = reset_callback();
	}

	forwarded_slot( forwarded_slot&& ) = delete;
};

template<class Signal, class InFunc, class OutFunc>
class signal_catcher;

template<class Ret, class... Args, class InFunc, class OutFunc>
class signal_catcher< signal< Ret(Args...)>, InFunc, OutFunc>
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
	signal_catcher( signal<Ret(Args...)>& s, InFunc inf, OutFunc outf )
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

template<class OrigSignal, class NewSignalSignature>
class signal_forward;

template<class OrigRet, class... OrigArgs,
         class Ret, class... Args>
class signal_forward<
         signal<OrigRet(OrigArgs...)>,
         Ret(Args...)
        >
{
	using SignalType = signal<OrigRet(OrigArgs...)>;

	SignalType& sig;
	callback< std::tuple<Args...>( OrigArgs... ) > infunc;
	callback< OrigRet( Ret ) > outfunc;

	static auto
	default_infunc( OrigArgs... args )
	{
		return std::forward_as_tuple( args... );
	}

	static OrigRet
	default_outfunc( Ret ret )
	{
		return ret;
	}

public:
	template<class InFunc = decltype( &signal_forward::default_infunc ),
	         class OutFunc = decltype( &signal_forward::default_outfunc )>
	signal_forward( SignalType& s,
	                InFunc inf = &signal_forward::default_infunc,
	                OutFunc outf = &signal_forward::default_outfunc )
		: sig( s ), infunc(inf), outfunc(outf)
	{}

	template<class Slot>
	connection connect_slot( Slot&& slot )
	{
		return std::move( sig.connect_slot( std::forward<Slot>(slot) ) );
	}

	template<class Func>
	connection connect( Func&& func )
	{
		auto cb = make_callback( std::forward<Func>(func) );
		using cb_type = decltype( cb );

		forwarded_slot< cb_type,
		                decltype( infunc ),
		                decltype( outfunc ) >
			fwdslot( std::move( cb ), infunc, outfunc );

		return std::move( sig.connect_slot( fwdslot ) );
	}

	template<class T, class PMemFunc>
	connection connect( T *obj, PMemFunc mfunc )
	{
		auto cb = make_callback( obj, mfunc );
		using cb_type = decltype( cb );

		forwarded_slot< cb_type,
		                decltype( infunc ),
		                decltype( outfunc ) >
			fwdslot( std::move( cb ), infunc, outfunc );

		return std::move( sig.connect_slot( fwdslot) );
	}

// 	void disconnect( connection& con )
// 	{
// 		sig_forwarded.disconnect( con );
// 	}

// 	void disconnect( std::size_t con_id )
// 	{
// 		sig_forwarded.disconnect( con_id );
// 	}

};


} // namespace pac

#endif // SIGNAL_FORWARD_HPP
