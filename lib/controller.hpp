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
 * controller.hpp
 *
 * Author: Brian Fransioli
 * Created: Sun Feb 09 23:12:32 KST 2014
 * Last modified: Mon Feb 10 02:13:00 KST 2014
 */

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

struct base_controller
{

};

struct controller : base_controller
{
	void init(){}
	void reset(){}

	void launch(){}
	void pause(){}
	void resume(){}
	void quit(){}

	void add_child(){}

	void OnLaunch(){}

};

struct unit
{
	controller *con;
	void add_child( unit *u ){}

};

#endif // CONTROLLER_HPP
