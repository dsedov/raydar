#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
  public:
    ray() {}

    ray(const point3& origin, const vec3& direction, const int depth) : orig(origin), dir(direction), depth(depth) {}

    const point3& origin() const  { return orig; }
    const vec3& direction() const { return dir; }

    point3 at(double t) const {
        return orig + t*dir;
    }
    const int get_depth() const {
        return depth;
    }
    
  private:
  int depth = 0;
    point3 orig;
    vec3 dir;
    
};

#endif