/*
  CSprite.h
  Simple SDL sprite class.
*/

#include "CSprite.h"

int CSprite::init(CSpriteBase *base, SDL_Surface *screen)
{
  mSpriteBase = base;
  if(mSpriteBase->mBuilt)
  {
    if(mSpriteBase->mNumframes>1) mAnimating=1;
    mBackreplacement = SDL_DisplayFormat(mSpriteBase->mAnim[0].image);
  }
  mScreen = screen;
  return 0;
}

void CSprite::clearBG()
{
  if(mDrawn==1)
  {
    SDL_Rect dest;
    dest.x = (int)mOldX;
    dest.y = (int)mOldY;
    dest.w = mSpriteBase->mW;
    dest.h = mSpriteBase->mH;
    SDL_BlitSurface(mBackreplacement, NULL, mScreen, &dest);
  }
}

void CSprite::updateBG()
{
  SDL_Rect srcrect;
  srcrect.x = (int)mX;
  srcrect.y = (int)mY;
  srcrect.w = mSpriteBase->mW;
  srcrect.h = mSpriteBase->mH;
  mOldX=mX;mOldY=mY;
  SDL_BlitSurface(mScreen, &srcrect, mBackreplacement, NULL);
}

void CSprite::draw()
{
  if(mAnimating == 1)
  {
    if(mLastupdate+mSpriteBase->mAnim[mFrame].pause*mSpeed<SDL_GetTicks())
    {
      mFrame++;
      if(mFrame>mSpriteBase->mNumframes-1) mFrame=0;
      mLastupdate = SDL_GetTicks();
    }
  }

  if(mDrawn==0) mDrawn=1;

  SDL_Rect dest;
  dest.x = (int)mX; dest.y = (int)mY;
  SDL_BlitSurface(mSpriteBase->mAnim[mFrame].image, NULL, mScreen, &dest);
}
