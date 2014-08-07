/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014 Mats Wahlberg
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

#include <ode/ode.h>
#include "geom.hpp"
#include "log.hpp"
#include "track.hpp"
#include "internal.hpp"
#include "../simulation/event_buffers.hpp"
#include "../loaders/conf.hpp"

Geom *Geom::head = NULL;

//allocates a new geom data, returns its pointer (and uppdate its object's count),
//ads it to the component list, and ads the data to specified geom (assumed)
Geom::Geom (dGeomID geom, Object *obj): Component(obj) //pass object argument to base class constructor
{
	//increase object activity counter
	object_parent->Increase_Activity();

	//parent object
	if (obj->selected_space) dSpaceAdd (obj->selected_space, geom);
	else //add geom to global space
		dSpaceAdd (space, geom);

	//add it to the geom list
	next = Geom::head;
	Geom::head = this;
	prev = NULL;

	//one more geom after this
	if (next) next->prev = this;

	//add it to the geom
	dGeomSetData (geom, (void*)(Geom*)(this));
	geom_id = geom;

	//now lets set some default values...
	//event processing (triggering):
	colliding = false; //no collision event yet
	model = NULL; //default: don't render

	//special geom indicators
	wheel = NULL; //not a wheel (for now)
	triangle_count = 0; //no "triangles"
	material_count = 0; //no "materials"
	triangle_colliding = NULL;
	material_surfaces = NULL;

	//events:
	//for force handling (disable)
	buffer_event=false;
	force_to_body=NULL; //when true, points at wanted body
	sensor_event=false;

	//debug variables
	flipper_geom = 0;
	TMP_pillar_geom =false; //not a demo pillar geom
}
//destroys a geom, and removes it from the list
Geom::~Geom ()
{
	//remove all events
	Event_Buffer_Remove_All(this);

	//1: remove it from the list
	if (!prev) //head in list, change head pointer
		Geom::head = next;
	else //not head in list, got a previous link to update
		prev->next = next;

	if (next) //not last link in list
		next->prev = prev;

	//remove actual geom from ode
	dGeomDestroy(geom_id);

	//if wheel data, remove to
	delete wheel;

	//decrease activity and check if 0
	object_parent->Decrease_Activity();

	//clear possible collision checking and material-based-surfaces
	delete[] triangle_colliding;
	delete[] material_surfaces;
}

Surface *Geom::Find_Material_Surface(const char *name)
{
	//is possible at all?
	if (!material_count)
	{
		Log_Add(0, "WARNING: tried to use per-material surfaces for non-trimesh geom");
		return NULL;
	}

	//firtst of all, check if enabled?
	if (!material_surfaces)
	{
		Log_Add(2, "enabling per-material surfaces for trimesh geom");
		material_surfaces = new Surface[material_count];

		//set default (set to out global surface)
		for (int i=0; i<material_count; ++i)
			material_surfaces[i] = surface;
	}

	//ok 
	int i;
	for (i=0; i<material_count && strcmp(name, parent_materials[i].name); ++i);

	if (i==material_count)
	{
		Log_Add(0, "WARNING: could not find material \"%s\" for trimesh", name);
		return NULL;
	}
	else
		return &material_surfaces[i];
}

