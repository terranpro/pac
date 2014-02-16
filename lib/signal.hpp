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
 * Last modified: Mon Feb 17 04:53:25 KST 2014
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
};

struct connection
{
	struct signal_concept
	{
		void *sig;
		std::size_t id;
		bool blocked;
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
			blocked = false;
			slot_ = sl;
		}
		~signal_reference()
		{
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
			blocked = false;
			slot_ = nullptr;
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

template<class Ret, class... Args>
class signal<Ret(Args...)>
{
public:
	using callback_type = callback<Ret, Args...>;
	using slot_type = slot<callback_type>;

private:
	//	std::unordered_map< std::size_t, callback_type > callbacks;
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

	template<class Func>
	connection connect(Func func)
	{
		callback<Ret, Args...> cb{ func };
		std::shared_ptr<slot_type> slot{ new slot_type(cb) };
		connection con( this, next_id, slot.get() );

		slots.insert( std::make_pair(next_id, slot) );
		++next_id;
		return con;
	}

	template<class T, class PMemFunc>
	connection connect( T *obj, PMemFunc mfunc )
	{
		auto cb = make_callback( obj, mfunc );
		std::shared_ptr<slot_type> slot{ new slot_type(cb) };
		connection con( this, next_id, slot.get() );

		slots.insert( std::make_pair(next_id, slot) );
		++next_id;
		return con;
	}

	void disconnect( connection& con )
	{
		con.disconnect();
	}

	void disconnect( std::size_t con_id )
	{
		auto it = slots.find( con_id );
		if ( it != std::end( slots ) ) {
			std::cout << "ERASED!\n";

			if ( dispatch_depth > 0 )
				it->second->delete_requested;
			else
				slots.erase( it );
		}
		BARK;
		BARK_THIS;
	}

	void dispatch(Args... args)
	{
		debug();

		++dispatch_depth;

		auto it = slots.begin();
		auto end = slots.end();

		for ( ; it != slots.end(); ++it ) {
			if ( it->second->blocked )
				continue;
			BARK;
			it->second->callback( std::forward<Args>(args)... );

			if ( it->second->delete_requested )
				it = slots.erase( it );
		}

		--dispatch_depth;
	}

	void emit(Args... args)
	{
		dispatch( std::forward<Args>(args)... );
	}

	void debug()
	{
		std::cout << "slots size = " << slots.size() << "\n";
	}
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
