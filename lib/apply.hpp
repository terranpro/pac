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
 * apply.hpp
 *
 * Author: Brian Fransioli
 * Created: Sun Mar 02 02:25:50 KST 2014
 * Last modified: Tue Sep 16 10:31:13 KST 2014
 */

#ifndef APPLY_HPP
#define APPLY_HPP

#include "sequence.hpp"

#include <tuple>

namespace {

template<class Func, class Tuple, std::size_t... Sequence>
auto apply( Func&& func, Tuple&& tup, pac::index_sequence<Sequence...> )
{
	return std::forward<Func>(func)(
		std::get<Sequence>( std::forward<Tuple>(tup) )... );
}

} // implementation details

namespace pac {

template<class Func, class Tuple>
auto apply( Func&& func, Tuple&& tup )
{
	return ::apply( std::forward<Func>(func),
	                std::forward<Tuple>(tup),
	                typename make_sequence<std::tuple_size<
	                typename std::decay<Tuple>::type>::value>::type()
	              );
}

} // namespace pac

#endif // APPLY_HPP
