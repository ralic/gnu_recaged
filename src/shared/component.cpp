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

#include "component.hpp"
#include "object.hpp"
#include "log.hpp"

Component::Component(Object *obj)
{
	//rather simple: just add it to the top of obj list of components
	next = obj->components;
	prev = NULL;
	obj->components = this;

	if (next) next->prev = this;

	//keep track of owning object
	object_parent = obj;
}

Component::~Component()
{
	//just unlink...
	if (prev) prev->next = next;
	else object_parent->components = next;

	if (next) next->prev = prev;
}

