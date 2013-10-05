/*
  CSprite.h
  Header for the simple SDL sprite class.
*/

#ifndef __CSPRITE_H__
#define __CSPRITE_H__

#include <SDL/SDL.h>

#include "CSpriteBase.h"

class CSprite
{
  public:
  CSprite() {}
  ~CSprite() {};
  int init(CSpriteBase *base, SDL_Surface *screen);
  void draw();
  void clearBG();
  void updateBG();

  void setFrame(int nr) { mFrame = nr; }
  int getFrame() { return mFrame; }

  void setSpeed(float nr) { mSpeed = nr; }
  float getSpeed() { return mSpeed; }

  void toggleAnim() { mAnimating = !mAnimating; }
  void startAnim() { mAnimating = 1; }
  void stopAnim() { mAnimating = 0; }
  void rewind() { mFrame = 0; }

  void xadd(int nr) { mX+=nr; }
  void yadd(int nr) { mY+=nr; }
  void xset(int nr) { mX=nr; }
  void yset(int nr) { mY=nr; }
  void set(int xx, int yy) { mX=xx; mY=yy; }

  int getX() {return mX;}
  int getY() {return mY;}

  int getw() {return mSpriteBase->mAnim[0].image->w;}
  int geth() {return mSpriteBase->mAnim[0].image->h;}

  private:
  int mFrame;
  int mX, mY, mOldX, mOldY;
  int mAnimating;
  int mDrawn;
  float mSpeed;
  long mLastupdate;
  CSpriteBase *mSpriteBase;
  SDL_Surface *mBackreplacement;
  SDL_Surface *mScreen;
};

#endif
