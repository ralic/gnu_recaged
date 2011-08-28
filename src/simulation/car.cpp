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

#include "../shared/car.hpp"
#include "../shared/track.hpp"
#include "../shared/internal.hpp"

void Car::Physics_Step(dReal step)
{
	int i;
	Car *carp = head;
	bool downforce;
	while (carp != NULL)
	{
		//first flipover detection (+ antigrav forces)

		//both sensors are triggered, not flipping, only downforce
		if (carp->sensor1->colliding && carp->sensor2->colliding)
			downforce = true;
		//only one sensor, flipping+downforce
		else if (carp->sensor1->colliding)
		{
			downforce = true;
			carp->dir = 1.0;
		}
		//same
		else if (carp->sensor2->colliding)
		{
			downforce = true;
			carp->dir = -1.0;
		}
		//no sensor active, no flipping, no downforce
		else
			downforce = false;

		//rotation speed of wheels
		dReal rotv[4];
		for (i=0; i<4; ++i)
			rotv[i] = dJointGetHinge2Angle2Rate (carp->joint[i]);

		//get car velocity:
		//TODO/IMPORTANT: need to find a reliable solution to this...

		//calculated from the average wheel rotation?
		//(wheels will often spin, resulting in too high value...)
		//carp->velocity = carp->dir*carp->wheel->radius*(rotv[0]+rotv[1]+rotv[2]+rotv[3])/4;

		//calculate from absolute velocity of body?
		//(doesn't take account of non-static surfaces (platforms)...)
		const dReal *vel = dBodyGetLinearVel (carp->bodyid);
		const dReal *rot = dBodyGetRotation  (carp->bodyid);
		carp->velocity = (rot[1]*vel[0] + rot[5]*vel[1] + rot[9]*vel[2]);

		//calculate velocity relative to surface in contact with side sensor?
		//(not implemented yet. will need bitfield to mask away non-ground gepms)
		//

		//downforce (debug implementation)
		if (downforce && carp->downforce)
		{
			//downforce (by thrusters to make space tracks possible)
			//for air-based downforce, this would be: conf*air_density*v*v
			//but since using thrusters, I'd guess: conf*v*v or just: conf*v

			//here: debug based on air dynamics:
			//TODO/NOTE: should, but does not, care about wind speed/direction!
			dReal force = carp->downforce*track.density*carp->velocity*carp->velocity;

			//avoid too big value:
			if (force > carp->maxdownforce)
				force=carp->maxdownforce;

			dBodyAddForce (carp->bodyid,0,0, carp->distdownforce*force);
			dBodyAddRelForce (carp->bodyid,0,0, -(1.0-carp->distdownforce)*force*carp->dir);
		}

		//calculate turning:
		dReal A[4];

		//length from turning axis to front and rear wheels
		dReal L1 = (carp->wy*2.0)*carp->dsteer;
		dReal L2 = (carp->wy*2.0)*(carp->dsteer-1.0);

		//turning radius (done by _assuming_ turning only with front wheels - but works for all situations)
		dReal maxsteer = carp->max_steer*(M_PI/180.0); //can not steer more than this
		//steering not allowed to decrease more than this
		dReal minsteer = carp->min_steer*(M_PI/180.0); //don't decrease more than this

		//should not steer more than this (-turning radius = v²*conf- and then calculated into turning angle)
		dReal limitsteer = atan( (carp->wy*2.0) /(carp->steerdecr * fabs(carp->velocity*carp->velocity)) );
		//and add minimum steering to limited steering (vel=inf -> limitsteer=minsteer+0)
		limitsteer += minsteer;

		//if limit is within what the actual turning can handle, use the limited as the max:
		if (limitsteer < maxsteer)
			maxsteer = limitsteer;

		//smooth transition from old max steer to new max steer
		//(if enabled)
		dReal speed = carp->limit_speed*(M_PI/180.0)*step;
		dReal old = carp->oldsteerlimit;
		//if not 0, not infinity, and not enough to move in this step
		if (speed && speed != dInfinity && fabs(maxsteer-old) > speed)
		{
			if (old < maxsteer)
				maxsteer = old+speed;
			else
				maxsteer = old-speed;
		}

		//store max steer
		carp->oldsteerlimit = maxsteer;

		//calculate turning radius
		dReal R = (carp->wy*2.0)/tan(maxsteer*carp->steering);

		//turning radius plus/minus distance to wheel hinge/axe/joint (similar to L1 and L2)
		dReal R1 = R-carp->jx;
		dReal R2 = R+carp->jx;

		if (carp->adapt_steer)
		{
			//turning angle of each wheel:
			A[0] = atan(L1/(R1));
			A[1] = atan(L2/(R1));
			A[2] = atan(L2/(R2));
			A[3] = atan(L1/(R2));
		}
		else //dumb
		{
			//different distribution and direction of fron and rear wheels
			dReal front = maxsteer*carp->dsteer*carp->steering;
			dReal rear = -maxsteer*(1.0-carp->dsteer)*carp->steering;
			A[0] = front;
			A[1] = rear;
			A[2] = rear;
			A[3] = front;
		}

		//apply steering
		if (carp->turn)
		{
			dJointSetHinge2Param (carp->joint[0],dParamLoStop,A[0]-carp->fwtoe);
			dJointSetHinge2Param (carp->joint[0],dParamHiStop,A[0]-carp->fwtoe);
			dJointSetHinge2Param (carp->joint[1],dParamLoStop,A[1]-carp->rwtoe);
			dJointSetHinge2Param (carp->joint[1],dParamHiStop,A[1]-carp->rwtoe);
			dJointSetHinge2Param (carp->joint[2],dParamLoStop,A[2]+carp->rwtoe);
			dJointSetHinge2Param (carp->joint[2],dParamHiStop,A[2]+carp->rwtoe);
			dJointSetHinge2Param (carp->joint[3],dParamLoStop,A[3]+carp->fwtoe);
			dJointSetHinge2Param (carp->joint[3],dParamHiStop,A[3]+carp->fwtoe);
		}
		//

		//breaking/accelerating:

		//the torque we want to apply
		dReal torque[4] = {0,0,0,0};


		//useful values:
		dReal kinertiatensor = carp->wheel->inertia; //moment of inertia tensor for wheel rotation
		dReal kpower = carp->power*carp->throttle; //motor power
		dReal krbreak = (1.0-carp->dbreak)*carp->max_break*carp->throttle/2.0; //breaking power for rear wheels
		dReal kfbreak = carp->dbreak*carp->max_break*carp->throttle/2.0; //breaking power for front wheels
		dReal kbreak[4] = {kfbreak, krbreak, krbreak, kfbreak}; //break power for each wheel (to make things easier)

		//check if using the built-in hinge2 motor for handbreaking, and release it
		if (carp->hinge2_dbreaks)
		{
			dJointSetHinge2Param (carp->joint[1],dParamFMax2, 0.0);
			dJointSetHinge2Param (carp->joint[2],dParamFMax2, 0.0);
		}

		//no fancy motor/break solution, lock rear wheels to handbrake turn (oversteer)
		if (carp->drift_breaks)
		{
			if (carp->hinge2_dbreaks) //use super-break
			{
				//request ode to apply as much force as needed to completely lock wheels
				dJointSetHinge2Param (carp->joint[1],dParamVel2, 0.0);
				dJointSetHinge2Param (carp->joint[1],dParamFMax2, dInfinity);
				dJointSetHinge2Param (carp->joint[2],dParamVel2, 0.0);
				dJointSetHinge2Param (carp->joint[2],dParamFMax2, dInfinity);
			}
			else
			{
				//apply enough on rear wheels to theoretically "lock" them
				//note: based on moment of inertia by rotation relative to car body...
				//not the most reliable solution but should suffice
				torque[1] = -rotv[1]*kinertiatensor/step;
				torque[2] = -rotv[2]*kinertiatensor/step;
			}
		}
		else
		{
			if (carp->throttle) //if driver is throttling (breake/accelerate)
			{
				//motor torque based on gearbox output rotation:
				//check if using good torque calculation or bad one:
				if (carp->diff) //even distribution
				{
					dReal rotation;
					if (carp->fwd&&carp->rwd) //4wd
					{
						rotation = fabs(rotv[0]+rotv[1]+rotv[2]+rotv[3])/4.0;

						//same torque for all wheels
						torque[0]=torque[1]=torque[2]=torque[3]=kpower/4.0;
					}
					else if (carp->rwd) //rwd
					{
						rotation = fabs(rotv[1]+rotv[2])/2.0;

						//(wheel 0 and 3 = 0 -> torque=0)
						torque[1]=torque[2]=kpower/2.0;
					}
					else //fwd
					{
						rotation = fabs(rotv[0]+rotv[3])/2.0;
						torque[0]=torque[3]=kpower/2.0;
					}

					//if less than optimal rotation (for gearbox), set to this level
					if (rotation < carp->gear_limit)
						rotation = carp->gear_limit;

					//apply
					for (i=0; i<4; ++i)
						torque[i]/=rotation;
				}
				else //uneven: one motor+gearbox for each wheel....
				{
					if (carp->fwd&&carp->rwd)
						torque[0]=torque[1]=torque[2]=torque[3]=kpower/4.0;
					else if (carp->rwd)
						torque[1]=torque[2]=kpower/2.0;
					else
						torque[0]=torque[3]=kpower/2.0;

					for (i=0; i<4; ++i)
					{
						if (fabs(rotv[i]) < carp->gear_limit)
							torque[i]/=carp->gear_limit;
						else
							torque[i]/=fabs(rotv[i]);
					}
				}

				//check if wanting to break
				dReal needed;
				for (i=0; i<4; ++i)
				{
					//if rotating in the oposite way of wanted, use breaks
					if (rotv[i]*kpower < 0.0) //(different signs makes negative)
					{
						//this much torque (in this direction) is needed to break wheel
						//not too reliable, since ignores the mass of the car body, and the fact
						//that the breaking will have to slow down the car movement too...
						needed = -rotv[i]*kinertiatensor/step;

						//to make transition between breaking and acceleration smooth
						//use different ways of calculate breaking torque:
						if ( needed/kbreak[i] < 1.0) //more breaking than needed
							torque[i] += needed; //break as needed + keep possible motor
						else //not enough break to stop... full break
							torque[i] += kbreak[i]; //full break + possible motor
					}
				}
			}

			//redistribute torque if enabled (nonzero force + enabled somewhere)
			if (carp->redist_force && (carp->fwredist || carp->rwredist) )
			{
				dReal radius[4] = {1,1,1,1}; //default, sane, turning radius for all wheels

				//adaptive redistribution and steering:
				//the wheels are wanted to rotate differently when turning.
				//this done by calculating the turning radius of the wheels.
				//otherwise they'll all be set to the same (1m)
				if (carp->adapt_redist && carp->steering)
				{
					//calculate proper turning radius for all wheels
					radius[0] =  sqrt(R1*R1+L1*L1); //right front
					radius[1] =  sqrt(R1*R1+L2*L2); //right rear
					radius[2] =  sqrt(R2*R2+L2*L2); //left rear
					radius[3] =  sqrt(R2*R2+L1*L1); //left front

					//and since wheels might not turn around their center, remove "offset"
					dReal offset = carp->wx-carp->jx; //difference between joint and wheel x
					if (R > 0.0) //turning right
					{
						radius[0] -= offset;
						radius[1] -= offset;
						radius[2] += offset;
						radius[3] += offset;
					}
					else //left
					{
						radius[0] += offset;
						radius[1] += offset;
						radius[2] -= offset;
						radius[3] -= offset;
					}

					//when reversing, the car will turn reversed, which the player
					//understands. but when reversing and driver turnings while
					//pressing forward, the driver usually expects the turning to
					//be normal, like driving forwards (at least I...).
					if (carp->velocity < 0.0 && carp->throttle*carp->dir > 0.0)
					{
						//ok, so how solve this? swapping the wheels'turning
						//radius (left/right) will reverse the redistribution.
						dReal tmp;
						tmp = radius[0]; radius[0]=radius[3]; radius[3]=tmp;
						tmp = radius[1]; radius[1]=radius[2]; radius[2]=tmp;
					}
				}

				dReal average; //average rotation ("per meters of turning radius")
				if (carp->fwredist && carp->rwredist)
				{
					average = (rotv[0]/radius[0]+rotv[1]/radius[1]
							+rotv[2]/radius[2]+rotv[3]/radius[3])/4.0;

					torque[0] -= (rotv[0]-radius[0]*average)*carp->redist_force;
					torque[1] -= (rotv[1]-radius[1]*average)*carp->redist_force;
					torque[2] -= (rotv[2]-radius[2]*average)*carp->redist_force;
					torque[3] -= (rotv[3]-radius[3]*average)*carp->redist_force;
				}
				else if (carp->rwredist)
				{
					average = (rotv[1]/radius[1]+rotv[2]/radius[2])/2.0;
					torque[1] -= (rotv[1]-radius[1]*average)*carp->redist_force;
					torque[2] -= (rotv[2]-radius[2]*average)*carp->redist_force;
				}
				else
				{
					average = (rotv[0]/radius[0]+rotv[3]/radius[3])/2.0;
					torque[0] -= (rotv[0]-radius[0]*average)*carp->redist_force;
					torque[3] -= (rotv[3]-radius[3]*average)*carp->redist_force;
				}
			}
		}
	
		//if a wheel is in air, lets limit the torques
		dReal airtorque = carp->airtorque;
		for (i=0; i<4; ++i)
		{
			//is in air
			if (!carp->wheel_geom_data[i]->colliding)
			{
				//above limit...
				if (torque[i] > airtorque)
					torque[i] = airtorque;
				else if (torque[i] < -airtorque)
					torque[i] = -airtorque;
			}
		}

		//apply torques
		dJointAddHinge2Torques (carp->joint[0],0, torque[0]);
		dJointAddHinge2Torques (carp->joint[1],0, torque[1]);
		dJointAddHinge2Torques (carp->joint[2],0, torque[2]);
		dJointAddHinge2Torques (carp->joint[3],0, torque[3]);

		//

		//done, next car...
		carp=carp->next;
	}
}
