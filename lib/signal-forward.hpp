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
 * Last modified: Sun Mar 30 03:19:25 KST 2014
 */

#ifndef SIGNAL_FORWARD_HPP
#define SIGNAL_FORWARD_HPP

#include "callback.hpp"
#include "signal.hpp"
#include "apply.hpp"

namespace pac {

template<class InFuncRet,
         class... UserFuncRet>
struct forward_invoker
{
	template<class InFunc, class UserFunc, class OutFunc,
	         class... InArgs>
		auto operator()( InFunc&& infunc,
		                 UserFunc&& userfunc,
		                 OutFunc&& outfunc,
		                 InArgs&&... inargs )
	{
		auto newargs = infunc( std::forward<InArgs>( inargs )... );
		return std::forward<OutFunc>(outfunc)(
			apply( std::forward<UserFunc>(userfunc),
			       std::move( newargs ) ) );
	}

	template<class... InArgs>
	static InFuncRet default_infunc( InArgs&&... inargs )
	{
		return std::make_tuple( std::forward<InArgs>( inargs )... );
	}

	template<class OR, class R>
	static OR default_outfunc( R ret )
	{
		return ret;
	}
};

template<class UserFuncRet>
struct forward_invoker<void, UserFuncRet>
{
	template<class InFunc, class UserFunc, class OutFunc,
	         class... InArgs>
		auto operator()( InFunc&& infunc,
		                 UserFunc&& userfunc,
		                 OutFunc&& outfunc,
		                 InArgs&&... inargs )
	{
		// infunc returns void - no newargs
		infunc( std::forward<InArgs>( inargs )... );

		// userfunc returns non-void - forward to outfunc which returns anything
		return std::forward<OutFunc>(outfunc)( std::forward<UserFunc>(userfunc) );
	}

	template<class... InArgs>
	static void default_infunc( InArgs&&... inargs )
	{
		return;
	}

	// TODO: Alternate signature that also seemed to work?!
	//static UserFuncRet default_outfunc( UserFuncRet uret )

	template<class OR, class R>
	static OR default_outfunc( R uret )
	{
		return uret;
	}

};

template<>
struct forward_invoker<void>
{
	template<class InFunc, class UserFunc, class OutFunc,
	         class... InArgs>
		auto operator()( InFunc&& infunc,
		                 UserFunc&& userfunc,
		                 OutFunc&& outfunc,
		                 InArgs&&... inargs )
	{
		// infunc returns void - no newargs
		infunc( std::forward<InArgs>( inargs )... );

		// userfunc returns void and takes no args
		std::forward<UserFunc>(userfunc)();

		// outfunc takes no args returns anything
		return std::forward<OutFunc>(outfunc)();
	}

	static void default_infunc()
	{
		return;
	}

	template<class OutFuncRet>
	static OutFuncRet default_outfunc()
	{
		return OutFuncRet();
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
		pac::callback< OutRet( InArgs... ) > cb =
			[&](InArgs... args)
			{
				forward_invoker< InRet, OutArgs... > invoker;

				return invoker( infunc, usercb, outfunc, args... );
			};
		return cb;
	}

	template<class CB, class FIn, class FOut>
	static auto generate_callback(CB cb,
	                              FIn fin,
	                              FOut fout)
	{
		pac::callback< OutRet( InArgs... ) > fwdcb =
			[cb, fin, fout](InArgs... args)
			{
				auto lcb = CB(cb);
				auto lfin = FIn(fin);
				auto lfout = FOut(fout);

				forward_invoker< InRet, OutArgs... > invoker;

				return invoker( lfin, lcb, lfout, args... );
			};

		return fwdcb;
	}

	forwarded_slot( forwarded_slot const& other )
		: slot< callback< OutRet( InArgs... ) > >( other )
	{
		usercb = other.usercb;
		infunc = other.infunc;
		outfunc = other.outfunc;

		this->callback = reset_callback();
	}

	forwarded_slot( forwarded_slot&& ) = delete;
};

template<class CB>
struct callback_gen;

template<class Ret, class... Args>
struct callback_gen<Ret(Args...)>
{
	using type = pac::callback<Ret(Args...)>;
};

template< template<class...> class Func,
          class Ret, class... Args >
struct callback_gen< Func<Ret(Args...)> >
	: callback_gen<Ret(Args...)>
{};

template<class CB1, class CB2>
struct fwdcallback_gen;

template<class R1, class... A1,
         class R2, class... A2>
struct fwdcallback_gen<R1(A1...), R2(A2...)>
{
	using type = pac::callback< R2(A1...) >;
};

template<class OrigSignal, class NewSignalSignature>
class signal_forward_base;

template<class OrigRet, class... OrigArgs,
         class Ret, class... Args>
class signal_forward_base<
         signal<OrigRet(OrigArgs...)>,
         Ret(Args...)
        >
{
protected:
	using SignalType = signal<OrigRet(OrigArgs...)>;
	using CallbackType = pac::callback<Ret( Args... )>;
	using InvokerType = forward_invoker<std::tuple<Args...>,Ret>;

	SignalType& sig;
	callback< std::tuple<Args...>( OrigArgs... ) > infunc;
	callback< OrigRet( Ret ) > outfunc;

	signal_forward_base( SignalType& s )
		: sig( s ),
		  infunc( &InvokerType::template default_infunc<OrigArgs...>),
		  outfunc( &InvokerType::template default_outfunc<OrigRet, Ret> )
	{}

	template<class InFunc, class OutFunc>
	signal_forward_base( SignalType& s, InFunc inf, OutFunc outf )
		: sig( s ), infunc( inf ), outfunc( outf )
	{}
};

template<class OrigRet, class... OrigArgs,
         class... Args>
class signal_forward_base<
         signal<OrigRet(OrigArgs...)>,
         void(Args...)
        >
{
protected:
	using SignalType = signal<OrigRet(OrigArgs...)>;
	using CallbackType = pac::callback<void( Args... )> ;

	SignalType& sig;
	callback< void( OrigArgs... ) > infunc;
	callback< OrigRet() > outfunc;

	signal_forward_base( SignalType& s )
		: sig( s ),
		  infunc( &forward_invoker<void>::default_infunc ),
		  outfunc( &forward_invoker<void>::default_outfunc<OrigRet> )
	{}

	template<class InFunc, class OutFunc>
	signal_forward_base( SignalType& s, InFunc inf, OutFunc outf )
		: sig( s ), infunc( inf ), outfunc( outf )
	{}
};

template<class OrigSignal, class NewSignalSignature>
class signal_forward
	: public signal_forward_base<
         OrigSignal,
         NewSignalSignature
        >
{
	using ParentType = signal_forward_base< OrigSignal,
	                                        NewSignalSignature
	                                      >;

	using SignalType = typename ParentType::SignalType;

public:
	signal_forward( SignalType& s )
		: ParentType( s )
	{}

	template<class InFunc,
	         class OutFunc>
	signal_forward( SignalType& s,
	                InFunc inf,
	                OutFunc outf )
		: ParentType( s, inf, outf )
	{}

	// template<class Slot>
	// connection connect_slot( Slot&& slot )
	// {
	// 	return std::move( this->sig.connect_slot( std::forward<Slot>(slot) ) );
	// }

	template<class Signature>
	connection connect( pac::callback<Signature> cb )
	{
		using cb_type = decltype( cb );

		// forwarded_slot< cb_type,
		//                 decltype( this->infunc ),
		//                 decltype( this->outfunc ) >
		// 	fwdslot( std::move( cb ), this->infunc, this->outfunc );

		auto fwdcb =
			forwarded_slot< cb_type,
			                decltype( this->infunc ),
			decltype( this->outfunc ) >
			::generate_callback( std::move(cb), this->infunc, this->outfunc );

		return this->sig.connect( fwdcb );
	}

	template<class Func>
	connection connect( Func&& func )
	{
		typename ParentType::CallbackType cb = func;
		using cb_type = decltype( cb );

		// forwarded_slot< cb_type,
		//                 decltype( this->infunc ),
		//                 decltype( this->outfunc ) >
		// 	fwdslot( std::move( cb ), this->infunc, this->outfunc );

		// return this->sig.connect_slot( fwdslot );

		auto fwdcb =
			forwarded_slot< cb_type,
			                decltype( this->infunc ),
			decltype( this->outfunc ) >
			::generate_callback( std::move(cb), this->infunc, this->outfunc );

		return this->sig.connect( fwdcb );
	}

	template<class PMemFunc, class T>
	connection connect( PMemFunc mfunc, T *obj )
	{
		auto cb = make_callback( obj, mfunc );
		using cb_type = decltype( cb );

		// forwarded_slot< cb_type,
		//                 decltype( this->infunc ),
		//                 decltype( this->outfunc ) >
		// 	fwdslot( std::move( cb ), this->infunc, this->outfunc );

		// return std::move( this->sig.connect_slot( fwdslot ) );

		auto fwdcb =
			forwarded_slot< cb_type,
			                decltype( this->infunc ),
			decltype( this->outfunc ) >
			::generate_callback( std::move(cb), this->infunc, this->outfunc );

		return this->sig.connect( fwdcb );
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
