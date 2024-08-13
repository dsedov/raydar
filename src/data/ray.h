#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
  public:
    ray() {}

    ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction) {}

    const point3& origin() const  { return orig; }
    const vec3& direction() const { return dir; }

    point3 at(double t) const {
        return orig + t*dir;
    }
    void set_max_depth(int max_depth) {
        this->max_depth = max_depth;
    }
    void set_depth(int depth) {
        this->depth = depth;
    }
    int max_depth = 0;
    int depth = 0;
  private:
    point3 orig;
    vec3 dir;
    
};

#endif