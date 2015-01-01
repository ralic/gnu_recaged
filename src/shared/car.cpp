/*
 * RCX - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg
 *
 * This file is part of RCX.
 *
 * RCX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RCX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RCX.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#include "car.hpp"
#include "log.hpp"
Car_Module::Car_Module(const char *name) :Racetime_Data(name)
{
	conf = car_conf_defaults; //set conf values to default
}



Car *Car::head = NULL;

//allocates car, add to list...
Car::Car(void)
{
	Log_Add(2, "configuring Car class");

	//default values
	dir = 1; //initiate to 1 for default

	//control values
	drift_brakes = true; //if the user does nothing, lock wheels
	throttle = 0;
	steering = 0;
	oldsteerlimit = 0;
	velocity = 0;

	//linking
	next=head;
	head=this;

	if (next)
		next->prev=this;

	prev=NULL;
}

//run _before_ starting full erase of object/component lists (at race end)
Car::~Car()
{
	Log_Add(2, "clearing Car class");

	//remove from list
	if (!prev) //head
		head = next;
	else //not head
		prev->next = next;

	if (next) //not last
		next->prev = prev;
}


