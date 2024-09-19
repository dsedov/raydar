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

#define CIELAB_E 0.008856
#define CIELAB_K 903.3
#define CIELAB_KE pow((CIELAB_E * CIELAB_K + 16.0)/116.0, 3.0)

struct spectral_sample {
    int wavelength;
    double value;
};

class color : public vec3 {

    public:
        enum ColorSpace {
            RGB_LIN,
            SRGB,
            XYZ,
            LAB,
            HSL,
            NO_COLORSPACE
        };
        // Inherit constructors from vec3
        using vec3::vec3;

        // Add conversion constructor from vec3
        color(const vec3& v) : vec3(v) {}
        color(float r, float g, float b, ColorSpace color_space) : vec3(r, g, b), color_space_(color_space) {}
        const color to_rgb() {
            if(color_space_ == ColorSpace::RGB_LIN) {
                return *this;

            } else if (color_space_ == ColorSpace::SRGB) {
                color result(srgb_to_linear(x()), srgb_to_linear(y()), srgb_to_linear(z()));
                result.set_color_space(ColorSpace::RGB_LIN);
                return result;

            } else if (color_space_ == ColorSpace::XYZ) {
                const mat3x3& m = xyz_rgb_matrix(rgb_colorspace::sRGB());
                color result = mat3x3_times(m, color(x(), y(), z()));
                result.set_color_space(ColorSpace::RGB_LIN);
                return result;

            } else if (color_space_ == ColorSpace::LAB) {
                color xyz_color = to_xyz();
                color result = xyz_color.to_rgb();
                result.set_color_space(ColorSpace::RGB_LIN);
                return result;

            } else if (color_space_ == ColorSpace::HSL) {
                color srgb_color = to_srgb();
                color result = srgb_color.to_rgb();
                result.set_color_space(ColorSpace::RGB_LIN);
                return result;

            } else {
                std::cerr << "Color space not supported" << std::endl;
                return color(0, 0, 0);
            }
        }
        const color to_xyz(const whitepoint& wp = whitepoint::D65()) {
            if(color_space_ == ColorSpace::RGB_LIN) {
                const mat3x3& m = rgb_xyz_matrix(rgb_colorspace::sRGB());
                color result = mat3x3_times(m, color(x(), y(), z()));
                result.set_color_space(ColorSpace::XYZ);
                return result;
            } else if (color_space_ == ColorSpace::SRGB) {
                color rgb_lin = to_rgb();
                color result = rgb_lin.to_xyz(wp);
                result.set_color_space(ColorSpace::XYZ);
                return result;
            } else if (color_space_ == ColorSpace::XYZ) {
                return *this;
            } else if (color_space_ == ColorSpace::LAB) {
                float L = x();
                float a = y();
                float b = z();
                float Y_ = (L + 16.0)/116.0;
                float X_ = a/500.0 + Y_; 
                float Z_ = Y_ - b/200.0;

                X_ = cielab_cube(X_, CIELAB_E)  * wp.x;
                Y_ = cielab_cube(Y_, CIELAB_KE) * wp.y;
                Z_ = cielab_cube(Z_, CIELAB_E)  * wp.z;
                color result(X_, Y_, Z_);
                result.set_color_space(ColorSpace::XYZ);
                return result;
            } else {
                std::cerr << "Color space not supported" << std::endl;
                return color(0, 0, 0);
            }
        }
        // From any color space to sRGB
        const color to_srgb(){
            if(color_space_ == ColorSpace::RGB_LIN) {
                color result(linear_to_srgb(x()), linear_to_srgb(y()), linear_to_srgb(z()));
                result.set_color_space(ColorSpace::SRGB);
                return result;
            } else if (color_space_ == ColorSpace::SRGB) {
                return *this;
            } else if (color_space_ == ColorSpace::XYZ) {
                color rgb_color = to_rgb();
                color result = rgb_color.to_srgb();
                result.set_color_space(ColorSpace::SRGB);
                return result;
            } else if (color_space_ == ColorSpace::LAB) {
                color rgb_color = to_rgb();
                color result = rgb_color.to_srgb();
                result.set_color_space(ColorSpace::SRGB);
                return result;
            } else if (color_space_ == ColorSpace::HSL) {
                float H = x();
                float S = y();
                float L = z();
                S /= 100.0;
                L /= 100.0;

                float C = (1 - abs(2 * L - 1)) * S;
                float X = C * (1 - std::abs(std::fmod(H / 60, 2) - 1));
                float m = L - C/2;

                float R = 0;
                float G = 0;
                float B = 0;

                if(0 <= H && H < 60) {
                    R = C;
                    G = X;
                    B = 0;
                } else if (60 <= H && H < 120) {
                    R = X;
                    G = C;
                    B = 0;
                } else if (120 <= H && H < 180) {
                    R = 0;
                    G = C;
                    B = X;
                } else if (180 <= H && H < 240) {
                    R = 0;
                    G = X;
                    B = C;
                } else if (240 <= H && H < 300) {
                    R = X;
                    G = 0;
                    B = C;
                } else if (300 <= H && H < 360) {
                    R = C;
                    G = 0;
                    B = X;
                }
                R = R + m;
                G = G + m;
                B = B + m;
                color result(R, G, B);
                result.set_color_space(ColorSpace::SRGB);
                return result;
            } else {
                std::cerr << "Color space not supported" << std::endl;
                return color(0, 0, 0);
            }
        }
        // From any color space to LAB
        const color to_lab(whitepoint wp){
            if (color_space_ == ColorSpace::RGB_LIN) {
                color color_xyz = to_xyz();
                color result = color_xyz.to_lab(wp);
                result.set_color_space(ColorSpace::LAB);
                return result;
            } else if(color_space_ == ColorSpace::SRGB) {
                color color_xyz = to_xyz();
                color result = color_xyz.to_lab(wp);
                result.set_color_space(ColorSpace::LAB);
                return result;
            } else if(color_space_ == ColorSpace::XYZ){
                float X_ = cielab_cuberoot(x()/wp.x);
                float Y_ = cielab_cuberoot(y()/wp.y);
                float Z_ = cielab_cuberoot(z()/wp.z);
                float L = 116.0 * Y_ - 16.0;
                float a = 500.0 * (X_ - Y_);
                float b = 200.0 * (Y_ - Z_);
                color result(L, a, b);  
                result.set_color_space(ColorSpace::LAB);
                return result;
            } else if (color_space_ == ColorSpace::LAB) {
                return *this;
            } else if (color_space_ == ColorSpace::HSL) {
                color color_xyz = to_xyz();
                color result = color_xyz.to_lab(wp);
                result.set_color_space(ColorSpace::LAB);
                return result;
            } else {
                std::cerr << "Color space not supported" << std::endl;
                return color(0, 0, 0);
            }
        }
        // From any to HSL
        const color to_hsl() {
            if (color_space_ == ColorSpace::RGB_LIN) {
                color srgb_color = to_srgb();
                color result = srgb_color.to_hsl();
                result.set_color_space(ColorSpace::HSL);
                return result;
            } else if (color_space_ == ColorSpace::SRGB) {

                float R = x();
                float G = y();
                float B = z();

                float cmin = std::min(R, std::min(G, B));
                float cmax = std::max(R, std::max(G, B));
                float delta = cmax - cmin;

                float H = 0;
                float S = 0;
                float L = 0;

                if (delta == 0) {
                    H = 0;
                } else if (cmax == R) {
                    H = std::fmod((G - B) / delta, 6.0);
                } else if (cmax == G) {
                    H = (B - R) / delta + 2;
                } else {
                    H = (R - G) / delta + 4;
                }

                H = round(H * 60);
                if (H < 0) {
                    H += 360;
                }
                L = (cmax + cmin) / 2.0;
                S = (delta == 0) ? 0 : delta / (1 - std::abs(2 * L - 1));
                S = +(S * 100);
                L = +(L * 100);
                color result(H, S, L);
                result.set_color_space(ColorSpace::HSL);
                return result;
            } else if (color_space_ == ColorSpace::XYZ) {
                color rgb_color = to_rgb();
                color result = rgb_color.to_hsl();
                result.set_color_space(ColorSpace::HSL);
                return result;
            } else if (color_space_ == ColorSpace::LAB) {
                color rgb_color = to_rgb();
                color result = rgb_color.to_hsl();
                result.set_color_space(ColorSpace::HSL);
                return result;
            } else {
                std::cerr << "Color space not supported" << std::endl;
                return color(0, 0, 0);
            }
        }
        // Add assignment operator from vec3
        color& operator=(const vec3& v) {
            vec3::operator=(v);
            return *this;
        }
        std::vector<float> toVec() {
            return {static_cast<float>(x()), static_cast<float>(y()), static_cast<float>(z())};
        }

        // Add conversion operator to vec3
        operator vec3() const { return vec3(x(), y(), z()); }

        void set_color_space(ColorSpace color_space) {
            color_space_ = color_space;
        }
        ColorSpace get_color_space() const {
            return color_space_;
        }
        void clamp() {
            e[0] = std::max(0.0, std::min(e[0], 1.0));
            e[1] = std::max(0.0, std::min(e[1], 1.0));
            e[2] = std::max(0.0, std::min(e[2], 1.0));
        }

    private:
        ColorSpace color_space_ = ColorSpace::RGB_LIN;
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
        inline float linear_to_srgb(float c){
            if(c <= 0.0031308) {
                return 12.92*c;
            } else {
                return 1.055*pow(c, 1/2.4) - 0.055;
            }
        }
        inline float srgb_to_linear(float c){
            if(c <= 0.04045) {
                return c/12.92;
            } else {
                return pow((c + 0.055)/1.055, 2.4);
            }
        }
        inline float cielab_cube(float c, float threshold){
            if(c * c * c <= threshold)
                return (116.0 * c - 16.0) / CIELAB_K;
            else
                return c * c * c;
        }
        inline float cielab_cuberoot(float c){
            if (c <= CIELAB_E)
                return (CIELAB_K * c + 16.0)/116.0;
            else
                return pow(c, 1.0/3.0);
        }
        
};


#endif