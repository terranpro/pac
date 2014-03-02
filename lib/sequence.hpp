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
 * sequence.hpp
 *
 * Author: Brian Fransioli
 * Created: Sun Mar 02 02:25:28 KST 2014
 * Last modified: Sun Mar 02 02:56:50 KST 2014
 */

#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP

namespace pac {

template<std::size_t... indices>
struct index_sequence
{};

template<std::size_t N, std::size_t... indices>
struct make_sequence : make_sequence< N-1, N-1, indices... >
{};

template<std::size_t... indices>
struct make_sequence< 0, indices... >
{
	using type = index_sequence<indices...>;
};

} // namespace pac

#endif // SEQUENCE_HPP
