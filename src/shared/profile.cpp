/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg
 *
 * This file is part of ReCaged.
 *
 * ReCaged is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReCaged is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#include "profile.hpp"

Profile *profile_head = NULL;

void Profile_Remove (Profile *target)
{
	printlog(2, "removing profile");

	//remove from list
	if (!target->prev) //head
		profile_head = target->next;
	else //not head
		target->prev->next = target->next;

	if (target->next) //not last
		target->next->prev = target->prev;

	//remove profile
	free (target);
}

void Profile_Remove_All()
{
	while (profile_head)
		Profile_Remove(profile_head);
}

