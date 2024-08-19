#ifndef MATERIAL_H
#define MATERIAL_H

#include "../raydar.h"
#include "../data/ray.h"
#include "../data/hittable.h"
#include "../data/pdf.h"
#include "../data/onb.h"
class hit_record;

namespace rd::core {
    class material {
    public:
        virtual ~material() = default;

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered,
            double& pdf
        ) const {
            return false;
        }
        virtual double scattering_pdf(
            const ray& r_in, const hit_record& rec, const ray& scattered
        ) const {
            return 0;
        }
        virtual color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const {
            return color(0,0,0);
        }
        virtual bool is_visible() const {
            return visible;
        }
        virtual void set_visible(bool v) {
            visible = v;
        }
        private:
            bool visible = true;
    };
    class lambertian : public material {
    public:
        lambertian(const color& albedo) : albedo(albedo) {}

        bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf)
        const override {

            onb uvw(rec.normal);
            auto scatter_direction = uvw.transform(random_cosine_direction());
            // Catch degenerate scatter direction
            if (scatter_direction.near_zero())
                scatter_direction = rec.normal;

            scattered = ray(rec.p, scatter_direction, r_in.get_depth() + 1);
            attenuation = albedo;
            pdf = dot(uvw.w(), scattered.direction()) / pi;
            return true;
        }
        double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const override {
            auto cosine = dot(rec.normal, unit_vector(scattered.direction()));
            return cosine < 0 ? 0 : cosine / pi;
        }
        

    private:
        color albedo;
    };
    class constant : public material {
    public:
        constant(const color& c) : albedo(c) {}

        bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf) 
        const override {
            // Constant materials don't scatter, so we always return false
            return false;
        }

        color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override {
            // Instead of scattering, we emit the constant color
            return albedo;
        }

    private:
        color albedo;
    };
    class light : public material {
    public:
        light(const color& light_color, double light_intensity) : light_color(light_color), light_intensity(light_intensity) {
            set_visible(true);
        }

        bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf)
        const override {
            return false;
        }

        color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override {
            if (!rec.front_face)
                return color(0,0,0);
            return light_color * light_intensity;
        }

    private:
        color light_color;
        double light_intensity;

    };
    class metal : public material {
    public:
        metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

        bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf)
        const override {
            vec3 reflected = reflect(r_in.direction(), rec.normal);
            reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
            scattered = ray(rec.p, reflected, r_in.get_depth() + 1);
            attenuation = albedo;
            pdf = 1.0;
            return (dot(scattered.direction(), rec.normal) > 0);
        }
        double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const override {
            // For perfect reflection, pdf is infinite at the reflection angle, 0 elsewhere
            // In practice, we return 1 for numerical stability
            return 1.0;
        }

    private:
        color albedo;
        double fuzz;
    };
    class dielectric : public material {
    public:
        dielectric(double refraction_index, double fuzz = 0.0) : refraction_index(refraction_index), fuzz(fuzz < 1 ? fuzz : 1) {}

        bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf)
        const override {
            attenuation = color(1.0, 1.0, 0.9);
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

            scattered = ray(rec.p, direction, r_in.get_depth() + 1);
            pdf = 1.0;
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
            double base_weight, const color& base_color, double base_metalness,
            double specular_weight, const color& specular_color, double specular_roughness, double specular_ior,
            double transmission_weight, const color& transmission_color,
            double emission_luminance, const color& emission_color
        ) : base_weight(base_weight), base_color(base_color), base_metalness(base_metalness),
            specular_weight(specular_weight), specular_color(specular_color), 
            specular_roughness(std::clamp(specular_roughness, 0.0, 1.0)), specular_ior(specular_ior),
            transmission_weight(transmission_weight), transmission_color(transmission_color),
            emission_luminance(emission_luminance), emission_color(emission_color) {}

        bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf) const override {
            vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
            vec3 refracted = refract(unit_vector(r_in.direction()), rec.normal, 1.0 / specular_ior);

            color weighted_base_color = base_color * base_weight;
            
            // Compute fresnel term (Schlick approximation)
            double cosTheta = std::fmin(dot(-unit_vector(r_in.direction()), rec.normal), 1.0);
            vec3 F0 = vec3(0.04, 0.04, 0.04);
            F0 = vec3(F0.x() * (1.0 - base_metalness) + weighted_base_color.x() * base_metalness,
                    F0.y() * (1.0 - base_metalness) + weighted_base_color.y() * base_metalness,
                    F0.z() * (1.0 - base_metalness) + weighted_base_color.z() * base_metalness);
            vec3 F = F0 + (vec3(1.0, 1.0, 1.0) - F0) * std::pow(1.0 - cosTheta, 5.0);

            // Compute roughness influence
            vec3 scatter_direction = reflected + specular_roughness * random_in_unit_sphere();

            // Handle scatter direction more accurately
            if (scatter_direction.near_zero()) {
                // If the scatter direction is too close to zero, use a random direction in the hemisphere
                scatter_direction = random_on_hemisphere(rec.normal);
            } else if (dot(scatter_direction, rec.normal) < 0) {
                // If the scatter direction is below the surface, reflect it about the normal
                scatter_direction = scatter_direction - 2 * dot(scatter_direction, rec.normal) * rec.normal;
            }
            // Normalize the scatter direction
            scatter_direction = unit_vector(scatter_direction);

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
                scattered = ray(rec.p, uvw.transform(random_in_unit_sphere()), r_in.get_depth() + 1);
                attenuation = base_color * (1.0 - base_metalness) * norm_base_weight;
                pdf = dot(uvw.w(), scattered.direction()) / pi;
                pdf = 1.0;
            } else if (p < norm_base_weight + norm_specular_weight) {
                // Specular reflection
                scattered = ray(rec.p, scatter_direction, r_in.get_depth() + 1);
                pdf = 1.0;
                attenuation = specular_color * F * norm_specular_weight;
            } else {
                // Transmission
                scattered = ray(rec.p, refracted, r_in.get_depth() + 1);
                pdf = 1.0;
                attenuation = transmission_color * norm_transmission_weight;
            }

            // Add contributions from all components
            attenuation += base_color * (1.0 - base_metalness) * norm_base_weight;
            attenuation += specular_color * F * norm_specular_weight;
            attenuation += transmission_color * norm_transmission_weight;

            // Apply gamma correction
            if (false && r_in.get_depth() == 0) {
            attenuation = color(
                pow(attenuation.x(), gamma) + offset,
                pow(attenuation.y(), gamma) + offset,
                pow(attenuation.z(), gamma) + offset
            );
            // Apply tint
            attenuation = attenuation * tint;
            }
            
            

            return true;
        }
        double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const override {
            return 1.0;
            auto cosine = dot(rec.normal, unit_vector(scattered.direction()));
            return cosine < 0 ? 0 : cosine / pi;
        }
        color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override {
            return emission_luminance * emission_color;
        }

        double gamma = 1.0;
        double offset = 0.0;
        color tint = color(1.0, 1.0, 1.0);

    private:
        double base_weight;
        color base_color;
        double base_metalness;
        double specular_weight;
        color specular_color;
        double specular_roughness;
        double specular_ior;
        double transmission_weight;
        color transmission_color;
        double emission_luminance;
        color emission_color;

        
    };
}
#endif