#ifndef MATERIAL_H
#define MATERIAL_H

#include "raydar.h"
#include "data/ray.h"
#include "data/hittable.h"

class hit_record;

class material {
  public:
    virtual ~material() = default;

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const {
        return false;
    }

    virtual color emitted(double u, double v, const point3& p) const {
        return color(0,0,0);
    }
};
class lambertian : public material {
  public:
    lambertian(const color& albedo) : albedo(albedo) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        auto scatter_direction = rec.normal + random_unit_vector();
        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        return true;
    }

  private:
    color albedo;
};
class metal : public material {
  public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

  private:
    color albedo;
    double fuzz;
};
class dielectric : public material {
  public:
    dielectric(double refraction_index, double fuzz = 0.0) : refraction_index(refraction_index), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
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

        scattered = ray(rec.p, direction);
        return true;
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

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        vec3 refracted = refract(unit_vector(r_in.direction()), rec.normal, 1.0 / specular_ior);
        
        // Compute fresnel term (Schlick approximation)
        double cosTheta = std::fmin(dot(-unit_vector(r_in.direction()), rec.normal), 1.0);
        vec3 F0 = vec3(0.04, 0.04, 0.04);
        F0 = vec3(F0.x() * (1.0 - base_metalness) + base_color.x() * base_metalness,
                  F0.y() * (1.0 - base_metalness) + base_color.y() * base_metalness,
                  F0.z() * (1.0 - base_metalness) + base_color.z() * base_metalness);
        vec3 F = F0 + (vec3(1.0, 1.0, 1.0) - F0) * std::pow(1.0 - cosTheta, 5.0);

        // Compute roughness influence
        vec3 scatter_direction = reflected + specular_roughness * random_in_unit_sphere();

        // Ensure the scatter direction is above the surface
        if (scatter_direction.near_zero() || dot(scatter_direction, rec.normal) < 0)
            scatter_direction = rec.normal;

        // Probabilistically choose between reflection, refraction, and diffuse scattering
        double p = random_double();
        if (p < base_weight) {
            // Diffuse scattering
            scattered = ray(rec.p, rec.normal + random_unit_vector());
            attenuation = base_color * (1.0 - base_metalness);
        } else if (p < base_weight + specular_weight) {
            // Specular reflection
            scattered = ray(rec.p, scatter_direction);
            attenuation = specular_color * F;
        } else {
            // Transmission
            scattered = ray(rec.p, refracted);
            attenuation = transmission_color;
        }

        return true;
    }

    color emitted(double u, double v, const point3& p) const override {
        return emission_luminance * emission_color;
    }

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
#endif