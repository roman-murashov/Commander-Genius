/*
 * CAmpton.cpp
 *
 *  Created on: 29 Dez 2012
 *      Author: Gerstrong
 */


#include "CAmpton.h"
#include "engine/galaxy/common/ai/CPlayerBase.h"
#include <engine/galaxy/common/ai/CPlayerLevel.h>
#include "misc.h"


namespace galaxy {  
  
  
enum AMPTONACTIONS
{
A_AMPTON_WALK = 0,
A_AMPTON_TURN = 4,
A_AMPTON_START_POLE = 5,
A_AMPTON_POLE_SLIDE = 7,
A_AMPTON_STOP_POLE = 8,
A_AMPTON_FLIP_SWITCH = 10,
A_AMPTON_STUNNED = 15
};


const int CSF_DISTANCE_TO_FOLLOW = 6<<CSF;

const int WALK_SPEED = 25;
const int SLIDE_SPEED = 25;

const int UMOUNT_TIME = 30;

  
CAmpton::CAmpton(CMap *pmap, const Uint16 foeID, const Uint32 x, const Uint32 y) :
CStunnable(pmap, foeID, x, y),
mTimer(0)
{
  	mActionMap[A_AMPTON_WALK] = (void (CStunnable::*)()) &CAmpton::processWalking;
  	mActionMap[A_AMPTON_TURN] = (void (CStunnable::*)()) &CAmpton::processTurn;
  	mActionMap[A_AMPTON_START_POLE] = (void (CStunnable::*)()) &CAmpton::processStartPole;
  	mActionMap[A_AMPTON_POLE_SLIDE] = (void (CStunnable::*)()) &CAmpton::processPoleSlide;
  	mActionMap[A_AMPTON_STOP_POLE] = (void (CStunnable::*)()) &CAmpton::processStopPole;
  	mActionMap[A_AMPTON_FLIP_SWITCH] = (void (CStunnable::*)()) &CAmpton::processFlipSwitch;
	mActionMap[A_AMPTON_STUNNED] = &CStunnable::processGettingStunned;
  
	// Adapt this AI
	setupGalaxyObjectOnMap(0x21DC, A_AMPTON_WALK);
	
	xDirection = LEFT;
}


void CAmpton::processWalking()
{
  //play tic toc sound
  if(getActionStatus(A_AMPTON_WALK))    
    playSound(SOUND_AMPTONWALK0);
  else if(getActionStatus(A_AMPTON_WALK+1))    
    playSound(SOUND_AMPTONWALK1);
  
  
  int l_x_l = getXLeftPos();
  int l_x_r = getXRightPos();
  int l_w = getXRightPos() - getXLeftPos();
  int l_h = getYDownPos() - getYUpPos();
  int l_y = getYMidPos();
  int l_x_mid = getXMidPos();    
  
  if ( (l_x_mid & 0x1FF) <= WALK_SPEED) 
  {
    if(hitdetectWithTilePropertyRectRO(31, l_x_mid, l_y, l_w, l_h, 1<<CSF))
    {
      setAction(A_AMPTON_FLIP_SWITCH);
    }
    
    if(hitdetectWithTilePropertyRectRO(1, l_x_mid, l_y, l_w, l_h, 1<<CSF))
    {
      if( getProbability(600) )
      //if (rand() < 0xC4)
      {
	bool polebelow = hitdetectWithTilePropertyHor(1, l_x_l, l_x_r, getYDownPos(), 1<<CSF);
	bool poleabove = hitdetectWithTilePropertyHor(1, l_x_l, l_x_r, getYUpPos(), 1<<CSF); 
	
	if( getProbability(400) )
	//if (rand() < 0x80) 
	  poleabove = false;
	else 
	  polebelow = false;
	
	//climb up
	if (poleabove) 
	{
	  setAction(A_AMPTON_START_POLE);
	  yDirection = UP;
	  return;
	}	  
	else if (polebelow) 
	{
	  setAction(A_AMPTON_START_POLE);
	  yDirection = DOWN;
	  return;
	}
      }
    }
  }
  
  // Move normally in the direction
  if( xDirection == RIGHT )
  {
    moveRight( WALK_SPEED );
  }
  else
  {
    moveLeft( WALK_SPEED );
  }   
}


void CAmpton::processTurn()
{
  if(getActionStatus(A_AMPTON_WALK))
  {
    setAction(A_AMPTON_WALK);
  }
}


void CAmpton::processStartPole()
{
  solid = false;
  setAction(A_AMPTON_POLE_SLIDE);
}


void CAmpton::processPoleSlide()
{
    
  int l_x_l = getXLeftPos();
  int l_x = getXMidPos();
  int l_x_r = getXRightPos();
  int l_y_mid = getYMidPos();
  int l_y_down = getYDownPos();	
  
  // Move normally in the direction
  if( yDirection == UP )
  {    
	// Check for the upper side and don't let him move if the pole ends
	if( hitdetectWithTileProperty(1, l_x_l, l_y_mid) ||
	    hitdetectWithTileProperty(1, l_x, l_y_mid) ||
	    hitdetectWithTileProperty(1, l_x_r, l_y_mid) )
	{
		moveUp( SLIDE_SPEED );
	}
	else
	{
		yDirection = DOWN;
	}	
  }
  else // Down
  {
	// Check for the upper side and don't let him move if the pole ends
	if( hitdetectWithTileProperty(1, l_x_l, l_y_down) ||
	    hitdetectWithTileProperty(1, l_x, l_y_down) ||
	    hitdetectWithTileProperty(1, l_x_r, l_y_down) )
	{
		moveDown( SLIDE_SPEED );
	}
	else
	{
		yDirection = UP;
	}    
  }
  
  mTimer++;
  if(mTimer < UMOUNT_TIME)
    return;
  
  mTimer = 0;
  
  // Check for Floor here!        
  const int fall1 = mp_Map->getPlaneDataAt(1, l_x, l_y_down+(1<<CSF));
  //const int fall1 = mp_Map->getPlaneDataAt(1, l_x, l_y_down);
  const CTileProperties &TileProp1 = g_pBehaviorEngine->getTileProperties(1)[fall1];
  const bool leavePole = (TileProp1.bup != 0);
  
  if(leavePole)
  {
    setAction(A_AMPTON_STOP_POLE);
    moveXDir(2*xDirection*WALK_SPEED);
    moveUp(1<<CSF);
    solid = true;
    blockedd = true;
  }
}


void CAmpton::processStopPole()
{
  if(getActionStatus(A_AMPTON_WALK))
  {
    setAction(A_AMPTON_WALK);
  }   
}


void CAmpton::processFlipSwitch()
{
  if(getActionStatus(A_AMPTON_WALK))
  {
    setAction(A_AMPTON_WALK);
  }   
}


bool CAmpton::isNearby(CSpriteObject &theObject)
{
	if( !getProbability(10) )
		return false;

	return true;
}

void CAmpton::getTouchedBy(CSpriteObject &theObject)
{
	if(dead || theObject.dead)
		return;

	CStunnable::getTouchedBy(theObject);

	// Was it a bullet? Than make it stunned.
	if( dynamic_cast<CBullet*>(&theObject) )
	{
		playSound(SOUND_ROBO_STUN);
		setAction(A_AMPTON_STUNNED);
		solid = true;
		dead = true;
		theObject.dead = true;
	}

	if( CPlayerBase *player = dynamic_cast<CPlayerBase*>(&theObject) )
	{
		//player->kill();
		player->push(*this);
	}
}


int CAmpton::checkSolidD( int x1, int x2, int y2, const bool push_mode )
{	
  if(getActionNumber(A_AMPTON_WALK))
  {
	if(turnAroundOnCliff( x1, x2, y2 ))
	  setAction(A_AMPTON_TURN);
  }
	
  return CGalaxySpriteObject::checkSolidD(x1, x2, y2, push_mode);
}


void CAmpton::process()
{
	performCollisions();
	
	if(!getActionNumber(A_AMPTON_POLE_SLIDE))
	  performGravityMid();

	if( blockedl )
	{
  	  if(xDirection == LEFT)
	    setAction(A_AMPTON_TURN);

	  xDirection = RIGHT;
	}
	else if(blockedr)
	{
  	  if(xDirection == RIGHT)
	    setAction(A_AMPTON_TURN);
	  
	  xDirection = LEFT;
	}

	if(!processActionRoutine())
	    exists = false;
	
	(this->*mp_processState)();
}

}
