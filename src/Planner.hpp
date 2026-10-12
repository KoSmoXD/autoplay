#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
namespace autojump {
struct Rect { float left=0,bottom=0,right=0,top=0; };
struct Obstacle { Rect bounds; bool hazard=true; };
struct Sense {
 float gap=10000.f,clusterEnd=0.f,dangerAbove=10000.f,dangerBelow=10000.f;
 bool obstacle=false,cluster=false;
};
class Planner {
public:
 void clear(){objects.clear();sorted=true;lastX=-100000.f;}
 void add(Obstacle o){if(o.bounds.right<=o.bounds.left||o.bounds.top<=o.bounds.bottom)return;objects.push_back(o);sorted=false;}
 void newAttempt(){lastX=-100000.f;}
 Sense sense(Rect p,bool inverted,bool solid,float ahead=270.f){
  sort();Sense s;
  auto it=std::lower_bound(objects.begin(),objects.end(),p.left-65.f,[](Obstacle const&o,float x){return o.bounds.left<x;});
  for(;it!=objects.end()&&it->bounds.left<p.right+ahead;++it){
   auto const& b=it->bounds;
   if(!it->hazard&&!solid)continue;
   if(!it->hazard&&(b.right-b.left>48.f||b.top-b.bottom>48.f))continue;
   float gap=b.left-p.right;if(gap< -20.f)continue;
   bool path=!inverted?(b.top>=p.bottom-3.f&&b.bottom<=p.bottom+34.f):(b.bottom<=p.top+3.f&&b.top>=p.top-34.f);
   if(path&&gap<s.gap){s.gap=gap;s.clusterEnd=b.right;s.obstacle=true;}
   if(s.obstacle&&path&&gap<=s.gap+105.f&&b.left<=s.clusterEnd+36.f)s.clusterEnd=std::max(s.clusterEnd,b.right);
   if(it->hazard&&b.right>p.left-12.f&&b.left<p.right+95.f){
    if(b.bottom>=p.top-4.f)s.dangerAbove=std::min(s.dangerAbove,b.bottom-p.top);
    if(b.top<=p.bottom+4.f)s.dangerBelow=std::min(s.dangerBelow,p.bottom-b.top);
   }
  }
  // Second pass: group successive ground hazards even when spacing is imperfect.
  // A three-spike row is typically ~90 world units long.
  if(s.obstacle){
   float end=s.clusterEnd;
   for(auto const& o:objects){
    auto const& b=o.bounds;
    if(b.left>end+42.f)break;
    if(b.right<=end||b.left<p.right+s.gap)continue;
    bool sameLane=!inverted?(b.top>=p.bottom-3.f&&b.bottom<=p.bottom+34.f):(b.bottom<=p.top+3.f&&b.top>=p.top-34.f);
    if(sameLane&&(o.hazard||solid))end=std::max(end,b.right);
   }
   s.clusterEnd=end;
  }
  s.cluster=s.obstacle&&s.clusterEnd-(p.right+s.gap)>44.f;return s;
 }
 bool shouldJump(Rect p,bool ground,bool inverted,float base,bool solids,float bias=0.f){
  if(!ground)return false;
  auto s=sense(p,inverted,solids,320.f);if(!s.obstacle)return false;
  float width=s.clusterEnd-(p.right+s.gap);
  // Longer spike clusters need launch earlier to cover the whole row.
  float extra=s.cluster?std::clamp((width-30.f)*1.05f+18.f,0.f,115.f):0.f;
  float distance=std::clamp(base+bias+extra,8.f,220.f);
  if(s.gap>distance||s.gap< -3.f)return false;
  float key=p.right+s.gap;if(std::fabs(key-lastX)<2.f)return false;
  lastX=key;return true;
 }
private:
 void sort(){if(sorted)return;std::stable_sort(objects.begin(),objects.end(),[](Obstacle const&a,Obstacle const&b){return a.bounds.left<b.bounds.left;});sorted=true;}
 std::vector<Obstacle> objects;bool sorted=true;float lastX=-100000.f;
};
class RetryLearner {
public:
 void clear(){bias=0;lastDeath=-100000;deaths=0;}
 void onDeath(float x){if(std::isfinite(x)){bias=std::clamp(bias+(std::fabs(x-lastDeath)<130.f?12.f:3.f),-20.f,65.f);lastDeath=x;++deaths;}}
 float getBias()const{return bias;}
 unsigned getDeaths()const{return deaths;}
private:float bias=0,lastDeath=-100000;unsigned deaths=0;
};
}