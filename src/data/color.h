#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"


#include <array>

using mat3x3 = std::array<std::array<double, 3>, 3>;

class whitepoint {
public:
    double x;
    double y;
    double z;

    whitepoint(double x, double y, double z) : x(x), y(y), z(z) {}

    static const whitepoint& A() {
        static const whitepoint a(1.09850, 1.0, 0.35585);
        return a;
    }

    static const whitepoint& B() {
        static const whitepoint b(0.99072, 1.0, 0.85223);
        return b;
    }

    static const whitepoint& C() {
        static const whitepoint c(0.98074, 1.0, 1.18232);
        return c;
    }

    static const whitepoint& D50() {
        static const whitepoint d50(0.96422, 1.0, 0.82521);
        return d50;
    }

    static const whitepoint& D55() {
        static const whitepoint d55(0.95682, 1.0, 0.92149);
        return d55;
    }

    static const whitepoint& D65() {
        static const whitepoint d65(0.95047, 1.0, 1.08883);
        return d65;
    }

    static const whitepoint& D75() {
        static const whitepoint d75(0.94972, 1.0, 1.22638);
        return d75;
    }

    static const whitepoint& E() {
        static const whitepoint e(1.00000, 1.0, 1.00000);
        return e;
    }

    static const whitepoint& F2() {
        static const whitepoint f2(0.99186, 1.0, 0.67393);
        return f2;
    }

    static const whitepoint& F7() {
        static const whitepoint f7(0.95041, 1.0, 1.08747);
        return f7;
    }

    static const whitepoint& F11() {
        static const whitepoint f11(1.00962, 1.0, 0.64350);
        return f11;
    }
};

class rgb_colorspace {
public:
    const whitepoint wp;
    double red_x;
    double red_y;
    double green_x;
    double green_y;
    double blue_x;
    double blue_y;

    rgb_colorspace(const whitepoint& wp, double rx, double ry, double gx, double gy, double bx, double by)
        : wp(wp), red_x(rx), red_y(ry), green_x(gx), green_y(gy), blue_x(bx), blue_y(by) {}

    static const rgb_colorspace& sRGB() {
        static const rgb_colorspace srgb(whitepoint(0.95047, 1.0, 1.08883), 0.640, 0.330, 0.300, 0.600, 0.150, 0.060);
        return srgb;
    }

    static const rgb_colorspace& Adobe() {
        static const rgb_colorspace adobe(whitepoint(0.95047, 1.0, 1.08883), 0.640, 0.330, 0.210, 0.710, 0.150, 0.060);
        return adobe;
    }

    static const rgb_colorspace& Rec709() {
        static const rgb_colorspace rec709(whitepoint(0.95047, 1.0, 1.08883), 0.640, 0.330, 0.300, 0.600, 0.150, 0.060);
        return rec709;
    }

    static const rgb_colorspace& Rec2020() {
        static const rgb_colorspace rec2020(whitepoint(0.95047, 1.0, 1.08883), 0.708, 0.292, 0.170, 0.797, 0.131, 0.046);
        return rec2020;
    }

    static const rgb_colorspace& P3() {
        static const rgb_colorspace p3(whitepoint(0.95047, 1.0, 1.08883), 0.680, 0.320, 0.265, 0.690, 0.150, 0.060);
        return p3;
    }
};

class color : public vec3 {
    public:
        // Inherit constructors from vec3
        using vec3::vec3;

        // Add conversion constructor from vec3
        color(const vec3& v) : vec3(v) {}
        const color from_xyz() {
            const mat3x3& m = xyz_rgb_matrix(rgb_colorspace::sRGB());
            color result = mat3x3_times(m, color(x(), y(), z()));
            return result;
        }
        const color to_xyz() {
            const mat3x3& m = rgb_xyz_matrix(rgb_colorspace::sRGB());
            color result = mat3x3_times(m, color(x(), y(), z()));
            return result;
        }
        // Add assignment operator from vec3
        color& operator=(const vec3& v) {
            vec3::operator=(v);
            return *this;
        }

        // Add conversion operator to vec3
        operator vec3() const { return vec3(x(), y(), z()); }

    private:
        const mat3x3 xyz_rgb_matrix(const rgb_colorspace& cs) {
            const mat3x3 m = rgb_xyz_matrix(cs);
            mat3x3 result = mat3x3_inverse(m);
            return result;
        }
        const mat3x3 rgb_xyz_matrix(const rgb_colorspace& cs) {
            mat3x3 m ={{     
                    {cs.red_x/cs.red_y, cs.green_x/cs.green_y, cs.blue_x/cs.blue_y},
                    {1.0, 1.0, 1.0},
                    {(1 - cs.red_x - cs.red_y)/cs.red_y, (1 - cs.green_x - cs.green_y)/cs.green_y, (1 - cs.blue_x - cs.blue_y)/cs.blue_y}
            }};
            mat3x3 wpXYZ = {{
                    {cs.wp.x, cs.wp.x, cs.wp.x},
                    {cs.wp.y, cs.wp.y, cs.wp.y},
                    {cs.wp.z, cs.wp.z, cs.wp.z}
            }};
            mat3x3 inv_m = mat3x3_inverse(m);
            mat3x3 minv_x_wp  = mat3x3_mult(inv_m, wpXYZ);
            mat3x3 minv_x_wp_t = mat3x3_transpose(minv_x_wp);
            mat3x3 result = mat3x3_times(m, minv_x_wp_t);
            return result;
        }
        inline mat3x3 mat3x3_inverse(const mat3x3& m) {

            double d = 1.0/(m[0][0]*(m[1][1]*m[2][2] - m[1][2]*m[2][1]) - m[0][1]*(m[1][0]*m[2][2] - m[1][2]*m[2][0]) + m[0][2]*(m[1][0]*m[2][1] - m[1][1]*m[2][0]));

            mat3x3 inv_m;

            inv_m[0][0] = d*(m[1][1]*m[2][2] - m[1][2]*m[2][1]);
            inv_m[0][1] = d*(m[0][2]*m[2][1] - m[0][1]*m[2][2]);
            inv_m[0][2] = d*(m[0][1]*m[1][2] - m[0][2]*m[1][1]);

            inv_m[1][0] = d*(m[1][2]*m[2][0] - m[1][0]*m[2][2]);
            inv_m[1][1] = d*(m[0][0]*m[2][2] - m[0][2]*m[2][0]);
            inv_m[1][2] = d*(m[0][2]*m[1][0] - m[0][0]*m[1][2]);

            inv_m[2][0] = d*(m[1][0]*m[2][1] - m[1][1]*m[2][0]);
            inv_m[2][1] = d*(m[0][1]*m[2][0] - m[0][0]*m[2][1]);
            inv_m[2][2] = d*(m[0][0]*m[1][1] - m[0][1]*m[1][0]);

            return inv_m;
        }
        inline mat3x3 mat3x3_mult(const mat3x3& left, const mat3x3& right) {
            mat3x3 m;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    m[i][j] = left[i][0]*right[0][j] + left[i][1]*right[1][j] + left[i][2]*right[2][j];
                }
            }
            return m;
        }
        inline color mat3x3_times(const mat3x3& left, const color& right){
            color result( left[0][0]*right.x() + left[0][1]*right.y() + left[0][2]*right.z(),
                          left[1][0]*right.x() + left[1][1]*right.y() + left[1][2]*right.z(),
                          left[2][0]*right.x() + left[2][1]*right.y() + left[2][2]*right.z());
            return result;
        }
        inline mat3x3 mat3x3_times(const mat3x3& left, const mat3x3& right){
            mat3x3 result;
            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 3; j++) {
                    result[i][j] = left[i][j]*right[i][j];
                }
            }
            return result;
        }
        inline mat3x3 mat3x3_transpose(const mat3x3& m) {
            mat3x3 mt;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    mt[i][j] = m[j][i];
                }
            }
            return mt;
        }
};


#endif