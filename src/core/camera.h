#ifndef RD_CORE_CAMERA_H
#define RD_CORE_CAMERA_H

namespace rd::core {
    class camera {
        public:
            camera() : fov(60), center(point3(0, 0, 0)), look_at(point3(0, 0, -1)), look_up(vec3(0, 1, 0)) {}
            camera(double fov, point3 center, point3 look_at, vec3 look_up) : fov(fov), center(center), look_at(look_at), look_up(look_up) {}

            double fov;
            point3 center; 
            point3 look_at;
            vec3   look_up;
    };
}

#endif