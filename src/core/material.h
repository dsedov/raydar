#ifndef MATERIAL_H
#define MATERIAL_H
#include <iostream>
#include "../data/ray.h"
#include "../data/hittable.h"
#include "../data/pdf.h"
#include "../data/onb.h"
#include "../data/spectrum.h"
#include "../image/image_spd.h"
class hit_record;
class scatter_record {
  public:
    spectrum attenuation;
    pdf * pdf_ptr;
    bool skip_pdf;
    ray skip_pdf_ray;
};
namespace rd::core {
    class material {
    public:
        virtual ~material() = default;

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, scatter_record& srec
        ) const {
            return false;
        }
        virtual double scattering_pdf(
            const ray& r_in, const hit_record& rec, const ray& scattered
        ) const {
            return 0;
        }
        virtual spectrum emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const {
            return spectrum(color(0,0,0));
        }
        virtual bool is_visible() const {
            return visible;
        }
        virtual void set_visible(bool v) {
            visible = v;
        }
        virtual bool is_cast_shadow() const {
            return cast_shadow;
        }
        virtual void set_cast_shadow(bool v) {
            cast_shadow = v;
        }
        virtual spectrum fast_ray_color(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const {
            return spectrum(color(0.5,0.5,0.5));
        }
        virtual void set_fast_light_color(spectrum c){
            fast_light_color = c;
        }
        virtual spectrum get_fast_light_color() const {
            return fast_light_color;
        }
        private:
            bool visible = true;
            bool cast_shadow = true;
            spectrum fast_light_color = spectrum::d65();
    };

    class constant : public material {
    public:
        constant(const spectrum& c) : albedo(c) {}

        bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) 
        const override {
            // Constant materials don't scatter, so we always return false
            return false;
        }

        spectrum emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override {
            // Instead of scattering, we emit the constant color
            return albedo;
        }

    private:
        spectrum albedo;
    };
    class light : public material {
    public:
        light(const spectrum& light_color, double light_intensity, ImageSPD * texture = nullptr, double light_spread = 1.0) 
        : light_color(light_color), light_intensity(light_intensity), texture(texture), light_spread(light_spread) {
            set_visible(true), set_cast_shadow(false);
        }

        bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec)
        const override {
            return false;
        }

        spectrum emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override {
            if (!rec.front_face)
                return spectrum(color(0,0,0));
            if (texture) {
                // Calculate the angle between the light normal and the ray direction
                double cos_angle = dot(unit_vector(rec.normal), unit_vector(-r_in.direction()));
                
                // Calculate the intensity based on the light spread
                double intensity_factor = 1.0;
                if (light_spread < 1.0) {
                    // Use a more appropriate falloff function
                    intensity_factor = std::pow(std::max(0.0, cos_angle), 1.0 / light_spread);
                }
                
                // Apply the intensity factor to the light intensity
                double adjusted_intensity = light_intensity * intensity_factor;
                return light_color * texture->uv_value(u, v) * adjusted_intensity * mult;
            }
            return light_color * light_intensity * mult;
        }
        void set_emission(const spectrum& c){
            light_color = c;
        }

    private:
        spectrum light_color;
        double light_intensity;
        ImageSPD * texture;
        double light_spread = 1.0;
        float mult = 0.1;
    };
    class metal : public material {
    public:
        metal(const spectrum& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

        bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec)
        const override {
            vec3 reflected = reflect(r_in.direction(), rec.normal);
            reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
            srec.skip_pdf = true;
            srec.skip_pdf_ray = ray(rec.p, reflected, r_in.get_depth() + 1);
            srec.attenuation = albedo;
            return (dot(srec.skip_pdf_ray.direction(), rec.normal) > 0);
        }
        double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const override {
            // For perfect reflection, pdf is infinite at the reflection angle, 0 elsewhere
            // In practice, we return 1 for numerical stability
            return 1.0;
        }

    private:
        spectrum albedo;
        double fuzz;
    };
    class dielectric : public material {
    public:
        dielectric(double refraction_index, double fuzz = 0.0) : refraction_index(refraction_index), fuzz(fuzz < 1 ? fuzz : 1) {}

        bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec)
        const override {
            srec.attenuation = spectrum(color(1.0, 1.0, 0.9));
            double ri = rec.front_face ? (1.0/refraction_index) : refraction_index;

            vec3 unit_direction = unit_vector(r_in.direction());
            double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
            double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

            bool cannot_refract = ri * sin_theta > 1.0;
            vec3 direction;

            if (cannot_refract || reflectance(cos_theta, ri) > random_double())
                direction = reflect(unit_direction, rec.normal);// + (fuzz * random_unit_vector());
            else
                direction = refract(unit_direction, rec.normal, ri) + (fuzz * random_unit_vector());

            srec.skip_pdf = true;
            srec.skip_pdf_ray = ray(rec.p, direction, r_in.get_depth() + 1);
            srec.pdf_ptr = nullptr;
            return true;
        }
        double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const override {
            return 1.0;
        }

    private:
        // Refractive index in vacuum or air, or the ratio of the material's refractive index over
        // the refractive index of the enclosing media
        double refraction_index;
        double fuzz;
        static double reflectance(double cosine, double refraction_index) {
            // Use Schlick's approximation for reflectance.
            auto r0 = (1 - refraction_index) / (1 + refraction_index);
            r0 = r0*r0;
            return r0 + (1-r0)*std::pow((1 - cosine),5);
        }
    };

    class advanced_pbr_material : public material {
    public:
        advanced_pbr_material(
            double base_weight, const spectrum& base_color, double base_metalness,
            double specular_weight, const spectrum& specular_color, double specular_roughness, double specular_ior,
            double transmission_weight, const spectrum& transmission_color,
            double emission_luminance, const spectrum& emission_color
        ) : base_weight(base_weight), base_color(base_color), base_metalness(base_metalness),
            specular_weight(specular_weight), specular_color(specular_color), 
            specular_roughness(std::clamp(specular_roughness, 0.0, 1.0)), specular_ior(specular_ior),
            transmission_weight(transmission_weight), transmission_color(transmission_color),
            emission_luminance(emission_luminance), emission_color(emission_color) {

                std::cout << "base_color: " << base_color << std::endl;
                std::cout << "specular_color: " << specular_color << std::endl;
                std::cout << "transmission_color: " << transmission_color << std::endl;
                std::cout << "emission_color: " << emission_color << std::endl;
            }
        spectrum fast_ray_color(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override {
            // Calculate the dot product of the normal and the up vector
            double up_factor = std::max(0.0, dot(rec.normal, vec3(0.2, 0.7, 0.3)));
            
            // Create a simple lighting factor based on the up_factor
            double lighting_factor = 0.3 + 0.7 * up_factor;
            
            // Apply the lighting factor to the base color
            return base_color * base_weight * lighting_factor * get_fast_light_color() / 25.0;
        }
        bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
            vec3 unit_direction = unit_vector(r_in.direction());
            vec3 reflected = reflect(unit_direction, rec.normal);
            spectrum weighted_base_color = base_color * base_weight;

            // Calculate the total weight
            double total_weight = base_weight + specular_weight + transmission_weight;

            // Normalize weights
            double norm_base_weight = base_weight / total_weight;
            double norm_specular_weight = specular_weight / total_weight;
            double norm_transmission_weight = transmission_weight / total_weight;

            // Choose between reflection, refraction, and diffuse scattering
            double p = random_double();
            if (p < norm_base_weight) {
                // Diffuse scattering
                onb uvw;
                uvw.build_from_w(rec.normal);
                vec3 scatter_direction = uvw.transform(random_cosine_direction());
                srec.skip_pdf_ray = ray(rec.p, scatter_direction, r_in.get_depth() + 1);
                srec.attenuation = base_color * (1.0 - base_metalness) * norm_base_weight;
                srec.skip_pdf = true;

            } else {
                double computed_ior = specular_ior + (r_in.wavelength - 550.0) / 300.0 / 8.0;
                double refraction_ratio = rec.front_face ? (1.0 / computed_ior) : computed_ior;
                double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
                double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
                bool cannot_refract = refraction_ratio * sin_theta > 1.0;

                if (cannot_refract || reflectance(cos_theta, refraction_ratio) > p){
                    // Specular reflection
                    vec3 scatter_direction = reflected + specular_roughness * random_in_unit_sphere();
                    scatter_direction = unit_vector(scatter_direction);
                    srec.skip_pdf_ray = ray(rec.p, scatter_direction, r_in.get_depth() + 1);
                    srec.attenuation = specular_color * specular_weight;
                    srec.skip_pdf = true;

                } else {
                    // Transmission (refraction)
                    
                    vec3 direction = refract(unit_direction, rec.normal, refraction_ratio);

                    srec.skip_pdf_ray = ray(rec.p, direction, r_in.get_depth() + 1);
                    srec.attenuation = transmission_color * transmission_weight;
                    srec.skip_pdf = true;
                }
            }

            return true;
        }
        double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
            if (transmission_weight > 0) {
                return 1.0;  // Perfect transmission
            }
            auto cosine = dot(rec.normal, unit_vector(scattered.direction()));
            return cosine < 0 ? 0 : cosine / pi;
        }
        spectrum emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override {
            return emission_color * emission_luminance;
        }

        double gamma = 1.0;
        double offset = 0.0;
        spectrum tint = spectrum(color(1.0, 1.0, 1.0));

    private:
        double base_weight;
        spectrum base_color;
        double base_metalness;
        double specular_weight;
        spectrum specular_color;
        double specular_roughness;
        double specular_ior;
        double transmission_weight;
        spectrum transmission_color;
        double emission_luminance;
        spectrum emission_color;
        static double reflectance(double cosine, double ref_idx) {
            // Use Schlick's approximation for reflectance.
            auto r0 = (1 - ref_idx) / (1 + ref_idx);
            r0 = r0 * r0;
            return r0 + (1 - r0) * pow((1 - cosine), 5);
        }
        
    };
}
#endif