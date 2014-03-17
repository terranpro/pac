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
 * Last modified: Mon Mar 17 13:49:55 KST 2014
 */

#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <functional>
#include <vector>
#include <unordered_map>
#include <utility>
#include <memory>

#include <iostream>

#include "callback.hpp"

//#define DEBUG_BARK 1

#ifdef DEBUG_BARK
#define BARK std::cout << __PRETTY_FUNCTION__ << ":" << __LINE__ << "\n"
#define BARK_THIS std::cout << "this = " << this << "\n"
#else
#define BARK
#define BARK_THIS
#endif // DEBUG_BARK

namespace pac {

template< class Signature >
class signal;

template<class Callback>
struct slot
{
	bool blocked = false;
	bool delete_requested = false;
	Callback callback;

	slot(Callback cb)
		: callback(cb)
	{}
	virtual ~slot() = default;

	slot( slot const& ) = default;
	slot( slot&& ) = default;
};

struct connection
{
	struct signal_concept
	{
		void *sig;
		std::size_t id;
		bool detached;
		void *slot_;

		virtual ~signal_concept()
		{
			BARK;
		}
		virtual void disconnect()
		{}
		virtual void block()
		{}
		virtual void unblock()
		{}
	};

	template<class Signal>
	struct signal_reference : signal_concept
	{
		using slot_type = typename Signal::slot_type;
		signal_reference(Signal *s, std::size_t i, slot_type *sl)
		{
			sig = s;
			id = i;
			detached = false;
			slot_ = sl;
		}
		~signal_reference()
		{
			BARK;
			if (!detached)
				disconnect();
		}
		void disconnect();
		void block();
		void unblock();
	};

	struct null_signal : signal_concept
	{
		null_signal()
		{
			sig = nullptr;
			id = -1;
			slot_ = nullptr;
			detached = false;
		}

		~null_signal()
		{}
	};

	std::shared_ptr<signal_concept> concept;

	template<class Signal>
	connection(Signal *sig, std::size_t i, typename Signal::slot_type *slot_)
		: concept( new signal_reference<Signal>(sig, i, slot_) )
	{ BARK; }

	connection()
		: concept( new null_signal )
	{ BARK; }

	~connection()
	{
		BARK;
	}

	bool operator==(const connection& other) const
	{
		return ( concept->id == other.concept->id &&
		         concept.get() == other.concept.get() );
	}

	void disconnect()
	{
		std::shared_ptr<signal_concept> empty;
		std::swap( concept, empty );
	}

	void detach()
	{
		concept->detached = true;
	}
};

}

namespace std {
template<>
struct hash<pac::connection>
{
	std::size_t operator()(pac::connection const& con) const
	{
		return std::hash<std::size_t>()( con.concept->id );
	}
};

}

namespace pac {

template<class Signature>
struct invoker;

template<class Ret, class... Args>
struct invoker<Ret(Args...)>
{
	using return_type = Ret;
	using results_type = std::vector<return_type>;

	template<class SlotIt>
	results_type dispatch(SlotIt beg, SlotIt end,
	                      Args... args)
	{
		results_type results;
		auto it = beg;

		for ( ; it != end; ++it ) {
			if ( it->second->blocked )
				continue;
			BARK;

			results.push_back(
				std::move( it->second->callback( std::forward<Args>(args)... ) ) );
		}

		return std::move( results );
	}
};

template<class... Args>
struct invoker<void(Args...)>
{
	using return_type = void;
	using results_type = void;

	using callback_type = callback<void( Args... )>;
	using slot_type = slot<callback_type>;

	template<class SlotIt>
	return_type dispatch(SlotIt beg, SlotIt end,
	                     Args... args)
	{
		auto it = beg;

		for ( ; it != end; ++it ) {
			if ( it->second->blocked )
				continue;
			BARK;

			it->second->callback( std::forward<Args>(args)... );
		}

	}

};

template<class Ret, class... Args>
class signal<Ret(Args...)>
{
public:
	using results_type = typename invoker<Ret(Args...)>::results_type;
	using callback_type = callback<Ret( Args... )>;
	using slot_type = slot<callback_type>;

	friend class invoker<Ret(Args...)>;

private:
	std::unordered_map< std::size_t, std::shared_ptr<slot_type> > slots;
	std::size_t next_id = 0;
	std::size_t dispatch_depth = 0;

public:
	signal() = default;

	~signal()
	{}

	// signal(signal const&) = default;
	// signal& operator=(signal const&) = default;

	signal(signal&&) = default;
	signal& operator=(signal&&) = default;

	template<class SlotType>
	connection connect_slot( SlotType const& slot )
	{
		BARK;

		slot_type *slotcopy = new SlotType( slot );
		auto slotptr = std::shared_ptr<slot_type>( slotcopy );
		connection con( this, next_id, slotptr.get() );

		slots.insert( std::make_pair(next_id, slotptr) );
		++next_id;
		return std::move( con );
	}

	template<class Func>
	connection connect(Func func)
	{
		callback<Ret( Args... )> cb{ func };
		return connect_slot( slot_type( cb ) );
	}

	template<class T, class PMemFunc>
	connection connect( PMemFunc mfunc, T&& obj )
	{
		auto cb = make_callback( mfunc, std::forward<T>(obj) );
		return connect_slot( slot_type( cb ) );
	}

	void disconnect( connection& con )
	{
		con.disconnect();
	}

	void disconnect( std::size_t con_id )
	{
		auto it = slots.find( con_id );
		if ( it == std::end( slots ) )
			return;

		if ( dispatch_depth > 0 )
			it->second->delete_requested = true;
		else
			slots.erase( it );

		BARK;
		BARK_THIS;
	}

	results_type emit(Args... args)
	{
		invoker<Ret(Args...)> inv;

		scoped_dec<std::size_t> dec( ++dispatch_depth );
		scoped_cleanup<decltype(*this)> cleanup_deleted_slots( *this );

		auto it = slots.begin();
		auto end = slots.end();

		return inv.dispatch( it, end, std::forward<Args>(args)... );
	}

private:
	template<class T>
	struct scoped_dec
	{
		T& obj;

		scoped_dec(T& obj_)
			: obj(obj_)
		{}

		~scoped_dec()
		{
			--obj;
		}
	};

	template<class Signal>
	struct scoped_cleanup
	{
		Signal& sig;
		scoped_cleanup(Signal& sig_)
			: sig(sig_)
		{}

		~scoped_cleanup()
		{
			auto it = sig.slots.begin();
			auto end = sig.slots.end();

			for( ; it != end; ++it ) {
				if ( it->second->delete_requested )
					it = sig.slots.erase( it );
			}
		}
	};

	friend struct scoped_cleanup< signal<Ret(Args...)> >;
};

template<class Signal>
void connection::signal_reference<Signal>::disconnect()
{
	BARK;
	BARK_THIS;

	Signal *real_sig = static_cast<Signal *>(sig);

	if (!real_sig) {
		BARK;
		return;
	}

	real_sig->disconnect( id );
	BARK;
}

template<class Signal>
void connection::signal_reference<Signal>::block()
{
	BARK;
	BARK_THIS;

	using slot_type = typename Signal::slot_type;

	slot_type *real_slot = static_cast<slot_type *>(slot_);

	if (!real_slot) {
		BARK;
		return;
	}

	real_slot->blocked = true;
	BARK;
}

template<class Signal>
void connection::signal_reference<Signal>::unblock()
{
	BARK;
	BARK_THIS;
	using slot_type = typename Signal::slot_type;

	slot_type *real_slot = static_cast<slot_type *>(slot_);

	if (!real_slot) {
		BARK;
		return;
	}

	real_slot->blocked = false;
	BARK;
}

struct connection_block
{
	connection& con;

	connection_block(connection& c)
		: con(c)
	{
		con.concept->block();
	}

	~connection_block()
	{
		con.concept->unblock();
	}
};

} // namespace pac

#endif // SIGNAL_HPP
