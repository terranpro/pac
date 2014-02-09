/*
 * This file is part of SampleGtkmm
 *
 * SampleGtkmm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SampleGtkmm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SampleGtkmm.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * SampleGtkmmRootPAC.hpp
 *
 * Author: Brian Fransioli
 * Created: Sun Jan 19 15:15:18 KST 2014
 * Last modified: Mon Feb 10 02:22:17 KST 2014
 */

#ifndef SAMPLEGTKMMROOTPAC_HPP
#define SAMPLEGTKMMROOTPAC_HPP

#include <memory>

struct SampleGtkmmRootPACImpl;

struct SampleGtkmmRootPAC
{
	std::unique_ptr<SampleGtkmmRootPACImpl> impl;

	SampleGtkmmRootPAC();
	~SampleGtkmmRootPAC();

	void load();
	void load( int&argc, char **&argv );
	void run();
	void run( int& argc, char **& argv );
};

#endif // SAMPLEGTKMMROOTPAC_HPP
