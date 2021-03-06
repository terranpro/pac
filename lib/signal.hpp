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
 * Last modified: Tue Sep 16 13:23:14 KST 2014
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
		: concept( std::make_shared<signal_reference<Signal>>(sig, i, slot_) )
	{}

	connection()
		: concept( std::make_shared<null_signal>() )
	{}

	~connection()
	{
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

	template<class SlotIt, class... A>
	results_type dispatch(SlotIt beg, SlotIt end, A&&... args)
	{
		results_type results;
		auto it = beg;

		for ( ; it != end; ++it ) {
			if ( it->second->blocked )
				continue;

			results.push_back(
				std::move( it->second->callback( std::forward<A>(args)... ) ) );
		}

		return results;
	}
};

template<class... Args>
struct invoker<void(Args...)>
{
	using return_type = void;
	using results_type = void;

	using callback_type = callback<void( Args... )>;
	using slot_type = slot<callback_type>;

	template<class SlotIt, class... A>
	return_type dispatch(SlotIt beg, SlotIt end, A&&... args)
	{
		auto it = beg;

		for ( ; it != end; ++it ) {
			if ( it->second->blocked )
				continue;

			it->second->callback( std::forward<A>(args)... );
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

	friend struct invoker<Ret(Args...)>;

private:
	std::unordered_map< std::size_t, std::shared_ptr<slot_type> > slots;
	std::size_t next_id = 0;
	std::size_t dispatch_depth = 0;

public:
	signal() = default;

	~signal()
	{}

	signal(signal const&) = delete;
	signal& operator=(signal const&) = delete;

	signal(signal&&) = default;
	signal& operator=(signal&&) = default;

	template<class SlotType>
	connection connect_slot( SlotType const& slot )
	{

		auto slotptr = std::make_shared<SlotType>( slot );
		connection con( this, next_id, slotptr.get() );

		slots.insert( std::make_pair(next_id, slotptr) );
		++next_id;
		return con;
	}

	template<class Signature>
	connection connect( pac::callback<Signature> cb )
	{
		return connect_slot( slot_type(cb) );
	}

	template<class Func>
	connection connect(Func func)
	{
		callback_type cb{ func };
		return connect_slot( slot_type( cb ) );
	}

	template<class T, class PMemFunc>
	connection connect( PMemFunc mfunc, T&& obj )
	{
		callback_type cb{ mfunc, obj };
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

	}

	template<class... A>
	results_type emit(A&&... args)
	{
		invoker<Ret(Args...)> inv;

		scoped_dec<std::size_t> dec( ++dispatch_depth );
		scoped_cleanup<decltype(*this)> cleanup_deleted_slots( *this );

		auto it = slots.begin();
		auto end = slots.end();

		return inv.dispatch( it, end, std::forward<A>(args)... );
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

	Signal *real_sig = static_cast<Signal *>(sig);

	if (!real_sig) {
		return;
	}

	real_sig->disconnect( id );
}

template<class Signal>
void connection::signal_reference<Signal>::block()
{

	using slot_type = typename Signal::slot_type;

	slot_type *real_slot = static_cast<slot_type *>(slot_);

	if (!real_slot) {
		return;
	}

	real_slot->blocked = true;
}

template<class Signal>
void connection::signal_reference<Signal>::unblock()
{
	using slot_type = typename Signal::slot_type;

	slot_type *real_slot = static_cast<slot_type *>(slot_);

	if (!real_slot) {
		return;
	}

	real_slot->blocked = false;
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
