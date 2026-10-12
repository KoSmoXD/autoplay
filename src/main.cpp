#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include "Planner.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
using namespace geode::prelude;
namespace {
constexpr int kJump=1;
autojump::Rect rect(cocos2d::CCRect const&r){return {r.getMinX(),r.getMinY(),r.getMaxX(),r.getMaxY()};}
enum class Mode{Cube,Ship,Wave,UFO,Ball,Robot,Spider,Swing};
Mode mode(PlayerObject*p){
 if(p->m_isShip)return Mode::Ship;if(p->m_isDart)return Mode::Wave;
 if(p->m_isBird)return Mode::UFO;if(p->m_isBall)return Mode::Ball;
 if(p->m_isRobot)return Mode::Robot;if(p->m_isSpider)return Mode::Spider;
 if(p->m_isSwing)return Mode::Swing;return Mode::Cube;
}
}
class $modify(AutoJumpPlayLayer,PlayLayer) {
 struct Fields {
  autojump::Planner planner;
  autojump::RetryLearner learner;
  bool pressed=false,wasDead=false;
  unsigned framesHeld=0,cooldown=0;
  cocos2d::CCDrawNode* trajectory=nullptr;
  std::vector<float> savedJumps;
  std::string loadedKey;
  bool pending=false, airborne=false;
  float pendingX=0.f, pendingEnd=0.f;
 };
 std::string levelKey(){
  if(!m_level)return "";
  return "successful-jumps-"+std::to_string(m_level->m_levelID);
 }
 void loadJumps(){
  auto key=levelKey();
  if(key==m_fields->loadedKey)return;
  m_fields->loadedKey=key;
  m_fields->savedJumps.clear();
  m_fields->pending=false;
  if(key.empty())return;
  std::istringstream input(Mod::get()->getSavedValue<std::string>(key,""));
  float x=0.f;
  while(input>>x && m_fields->savedJumps.size()<512)
   if(std::isfinite(x))m_fields->savedJumps.push_back(x);
  std::sort(m_fields->savedJumps.begin(),m_fields->savedJumps.end());
 }
 void saveJump(float x){
  if(m_fields->loadedKey.empty()||!std::isfinite(x))return;
  for(float v:m_fields->savedJumps)if(std::fabs(x-v)<12.f)return;
  if(m_fields->savedJumps.size()>=512)return;
  m_fields->savedJumps.push_back(x);
  std::sort(m_fields->savedJumps.begin(),m_fields->savedJumps.end());
  std::ostringstream out;
  for(float v:m_fields->savedJumps)out<<v<<' ';
  Mod::get()->setSavedValue(m_fields->loadedKey,out.str());
 }
 void button(bool down){
  if(m_fields->pressed==down)return;
  m_fields->pressed=down;this->handleButton(down,kJump,true);
 }
 void addObject(GameObject* obj){
  PlayLayer::addObject(obj);if(!obj)return;
  const auto t=obj->m_objectType;
  bool hazard=t==GameObjectType::Hazard||t==GameObjectType::AnimatedHazard;
  bool solid=t==GameObjectType::Solid;
  if(hazard||solid)m_fields->planner.add({rect(obj->getObjectRect()),hazard});
 }
 void drawTrajectory(autojump::Rect p,Mode m,float speed,bool hold){
  if(!m_fields->trajectory){
   m_fields->trajectory=cocos2d::CCDrawNode::create();
   if(m_fields->trajectory)this->addChild(m_fields->trajectory,999);
  }
  auto*d=m_fields->trajectory;if(!d)return;d->clear();
  float x=(p.left+p.right)*.5f,y=(p.top+p.bottom)*.5f;
  cocos2d::ccColor4F tint{.2f,1.f,.55f,.75f};
  cocos2d::CCPoint prev=ccp(x,y);
  for(int i=1;i<=20;++i){
   float t=i/20.f,dx=t*std::clamp(speed*26.f,100.f,230.f),dy=0.f;
   if(m==Mode::Cube||m==Mode::Robot)dy=hold?100.f*4.f*t*(1.f-t):0.f;
   else if(m==Mode::Wave)dy=(hold?1.f:-1.f)*dx*.8f;
   else if(m==Mode::Ship||m==Mode::Swing)dy=(hold?1.f:-1.f)*dx*.25f;
   else if(m==Mode::UFO)dy=hold?48.f*std::sin(t*3.1415926f):-45.f*t;
   cocos2d::CCPoint next=ccp(x+dx,y+dy);d->drawSegment(prev,next,1.f,tint);prev=next;
  }
 }
 void postUpdate(float dt){
  PlayLayer::postUpdate(dt);
  auto* p=m_player1;if(!p)return;
  loadJumps();
  if(p->m_isDead&&!m_fields->wasDead)m_fields->learner.onDeath(p->getPositionX());
  m_fields->wasDead=p->m_isDead;
  if(p->m_isDead)m_fields->pending=false;
  if(m_fields->pending&&!p->m_isOnGround)m_fields->airborne=true;
  if(m_fields->pending&&!p->m_isDead&&m_fields->airborne&&
     p->m_isOnGround&&p->getPositionX()>m_fields->pendingEnd+12.f){
   saveJump(m_fields->pendingX);
   m_fields->pending=false;
  }
  auto*mod=Mod::get();
  if(!mod->getSettingValue<bool>("enabled")||m_isPaused||m_hasCompletedLevel||
   p->m_isDead||p->m_controlsDisabled||p->m_inputsLocked||p->m_isPlatformer||p->m_isGoingLeft){
   button(false);if(m_fields->trajectory)m_fields->trajectory->clear();return;
  }
  Mode m=mode(p);auto position=rect(p->getObjectRect());bool upside=p->m_isUpsideDown;
  float base=static_cast<float>(mod->getSettingValue<double>("lookahead"));
  bool solids=mod->getSettingValue<bool>("jump-solids");
  auto sense=m_fields->planner.sense(position,upside,solids);
  bool learning=mod->getSettingValue<bool>("learning");
  float bias=learning?m_fields->learner.getBias():0.f;
  if(m_fields->cooldown)--m_fields->cooldown;
  if(m==Mode::Ship||m==Mode::Wave||m==Mode::Swing){
   bool hold=m_fields->pressed;
   if(sense.dangerBelow<75.f)hold=!upside;
   else if(sense.dangerAbove<75.f)hold=upside;
   else if(m==Mode::Wave&&m_fields->framesHeld>=12)hold=!m_fields->pressed;
   button(hold);
   if(m_fields->pressed)++m_fields->framesHeld;else m_fields->framesHeld=0;
  }else{
   bool trigger=false;
   if(m==Mode::Cube||m==Mode::Robot){
    bool nearbySaved=false;
    for(float x:m_fields->savedJumps){
     if(std::fabs(x-p->getPositionX())<35.f){
      nearbySaved=true;
      if(p->m_isOnGround&&!m_fields->pending&&
       p->getPositionX()>=x-6.f&&p->getPositionX()<=x+8.f)
       trigger=true;
      break;
     }
    }
    if(!nearbySaved&&!m_fields->pending)
     trigger=m_fields->planner.shouldJump(position,p->m_isOnGround,upside,base,solids,bias);
    if(trigger&&!m_fields->pending){
     auto target=m_fields->planner.sense(position,upside,solids,320.f);
     if(target.obstacle){
      m_fields->pending=true;
      m_fields->airborne=false;
      m_fields->pendingX=p->getPositionX();
      m_fields->pendingEnd=target.clusterEnd;
     }
    }
   }
   else if(m_fields->cooldown==0&&sense.obstacle&&sense.gap<=base+bias+15.f&&sense.gap>=-3.f){
    trigger=true;m_fields->cooldown=m==Mode::UFO?10:14;
   }
   button(trigger);
  }
  if(mod->getSettingValue<bool>("trajectory"))drawTrajectory(position,m,std::max(1.f,p->m_playerSpeed),m_fields->pressed);
  else if(m_fields->trajectory)m_fields->trajectory->clear();
 }
 void resetLevel(){
  button(false);m_fields->planner.newAttempt();m_fields->framesHeld=0;
  m_fields->cooldown=0;m_fields->wasDead=false;
  m_fields->pending=false;m_fields->airborne=false;PlayLayer::resetLevel();
 }
 void onExit(){
  button(false);m_fields->planner.clear();m_fields->learner.clear();
  m_fields->savedJumps.clear();m_fields->loadedKey.clear();
  PlayLayer::onExit();
 }
};