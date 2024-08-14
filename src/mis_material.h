#ifndef MIS_MATERIAL_H
#define MIS_MATERIAL_H

#include "raydar.h"
#include "data/ray.h"
//#include "data/pdf.h"
#include "data/hittable.h"

class mis_material {
public:
    virtual ~mis_material() = default;

    virtual bool scatter(
        const ray& r_in, const mis_hit_record& rec, color& attenuation, ray& scattered, double& pdf_val
    ) const = 0;

    virtual double pdf(const vec3& wo, const vec3& wi, const vec3& n) const {
        return 0.0;
    }

    virtual color brdf(const vec3& wo, const vec3& wi, const vec3& n) const {
        return color(0,0,0);
    }

    virtual color emitted(double u, double v, const point3& p) const {
        return color(0,0,0);
    }

    virtual bool can_scatter() const {
        return true;
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
class emissive_material : public mis_material {
public:
    emissive_material(const color& emit) : emission(emit) {}

    bool scatter(const ray& r_in, const mis_hit_record& rec, color& attenuation, ray& scattered, double& pdf_val) const override {
        return false;  // Emissive materials don't scatter light
    }

    color emitted(double u, double v, const point3& p) const override {
        return emission;
    }

    bool can_scatter() const override {
        return false;
    }

private:
    color emission;
};
class pbr_material : public mis_material {
public:
    pbr_material(
        double base_weight, const color& base_color, double base_metalness,
        double specular_weight, const color& specular_color, double specular_roughness, double specular_ior,
        double transmission_weight, const color& transmission_color,
        double emission_luminance, const color& emission_color
    ) : base_weight(base_weight), base_color(base_color), base_metalness(base_metalness),
        specular_weight(specular_weight), specular_color(specular_color), 
        specular_roughness(std::clamp(specular_roughness, 0.0, 1.0)), specular_ior(specular_ior),
        transmission_weight(transmission_weight), transmission_color(transmission_color),
        emission_luminance(emission_luminance), emission_color(emission_color) {
        
        total_weight = base_weight + specular_weight + transmission_weight;
        norm_base_weight = base_weight / total_weight;
        norm_specular_weight = specular_weight / total_weight;
        norm_transmission_weight = transmission_weight / total_weight;
    }

    bool scatter(const ray& r_in, const mis_hit_record& rec, color& attenuation, ray& scattered, double& pdf_val) const override {
        vec3 wo = -unit_vector(r_in.direction());
        vec3 n = rec.normal;

        // Generate a random number to choose between BRDF components
        double p = random_double();
        
        if (p < norm_base_weight) {
            // Diffuse scattering
            vec3 wi = random_cosine_direction();
            scattered = ray(rec.p, wi, r_in.get_depth() + 1);
            pdf_val = pdf(wo, wi, n);
        } else if (p < norm_base_weight + norm_specular_weight) {
            // Specular reflection
            vec3 reflected = reflect(unit_vector(r_in.direction()), n);
            vec3 wi = reflected + specular_roughness * random_in_unit_sphere();
            wi = unit_vector(wi);
            scattered = ray(rec.p, wi, r_in.get_depth() + 1);
            pdf_val = pdf(wo, wi, n);
        } else {
            // Transmission
            vec3 refracted = refract(unit_vector(r_in.direction()), n, 1.0 / specular_ior);
            scattered = ray(rec.p, refracted, r_in.get_depth() + 1);
            pdf_val = norm_transmission_weight; // Simplified PDF for transmission
        }

        attenuation = brdf(wo, scattered.direction(), n);
        return true;
    }

    color brdf(const vec3& wo, const vec3& wi, const vec3& n) const override {
        vec3 h = unit_vector(wo + wi);
        double cos_theta = std::fmax(dot(n, wi), 0.0);

        // Compute fresnel term (Schlick approximation)
        vec3 F0 = vec3(0.04, 0.04, 0.04);
        F0 = vec3(F0.x() * (1.0 - base_metalness) + base_color.x() * base_metalness,
                  F0.y() * (1.0 - base_metalness) + base_color.y() * base_metalness,
                  F0.z() * (1.0 - base_metalness) + base_color.z() * base_metalness);
        vec3 F = F0 + (vec3(1.0, 1.0, 1.0) - F0) * std::pow(1.0 - std::fmax(dot(h, wo), 0.0), 5.0);

        // Diffuse component
        color diffuse = (base_color / pi) * (1.0 - base_metalness) * norm_base_weight;

        // Specular component (simplified microfacet model)
        double D = ggx_distribution(n, h, specular_roughness);
        double G = geometry_smith(n, wo, wi, specular_roughness);
        color specular = (F * D * G) / (4 * std::fmax(dot(n, wo), 0.0) * cos_theta) * norm_specular_weight;

        // Transmission component (simplified)
        color transmission = transmission_color * norm_transmission_weight;

        return diffuse + specular + transmission;
    }

    double pdf(const vec3& wo, const vec3& wi, const vec3& n) const override {
        double cos_theta = std::fmax(dot(n, wi), 0.0);

        // Diffuse PDF
        double pdf_diffuse = cos_theta / pi * norm_base_weight;

        // Specular PDF (simplified)
        vec3 reflected = reflect(-wo, n);
        double pdf_specular = std::pow(std::fmax(dot(reflected, wi), 0.0), specular_roughness * 1000) * norm_specular_weight;

        // Transmission PDF (simplified)
        double pdf_transmission = norm_transmission_weight;

        return pdf_diffuse + pdf_specular + pdf_transmission;
    }

    color emitted(double u, double v, const point3& p) const override {
        return emission_luminance * emission_color;
    }

    bool can_scatter() const override {
        return true; // This material can scatter light
    }

    double gamma = 1.0;
    double offset = 0.0;
    color tint = color(1.0, 1.0, 1.0);

private:
    double base_weight, base_metalness;
    color base_color;
    double specular_weight, specular_roughness, specular_ior;
    color specular_color;
    double transmission_weight;
    color transmission_color;
    double emission_luminance;
    color emission_color;

    double total_weight;
    double norm_base_weight, norm_specular_weight, norm_transmission_weight;

    // Helper functions for BRDF calculation
    double ggx_distribution(const vec3& n, const vec3& h, double roughness) const {
        double a2 = roughness * roughness;
        double nh = std::fmax(dot(n, h), 0.0);
        double denom = (nh * nh * (a2 - 1.0) + 1.0);
        return a2 / (pi * denom * denom);
    }

    double geometry_schlick_ggx(double ndotv, double roughness) const {
        double k = roughness * roughness / 2.0;
        return ndotv / (ndotv * (1.0 - k) + k);
    }

    double geometry_smith(const vec3& n, const vec3& v, const vec3& l, double roughness) const {
        double ndotv = std::fmax(dot(n, v), 0.0);
        double ndotl = std::fmax(dot(n, l), 0.0);
        return geometry_schlick_ggx(ndotl, roughness) * geometry_schlick_ggx(ndotv, roughness);
    }
};
class constant_color_material : public mis_material {
public:
    constant_color_material(const color& c) : albedo(c) {}

    bool scatter(const ray& r_in, const mis_hit_record& rec, color& attenuation, ray& scattered, double& pdf_val) const override {
        return false; // This material doesn't scatter light
    }

    color brdf(const vec3& wo, const vec3& wi, const vec3& n) const override {
        return albedo / pi; // Return the constant color divided by pi for energy conservation
    }

    double pdf(const vec3& wo, const vec3& wi, const vec3& n) const override {
        return 0.0; // This material doesn't have a probability distribution function
    }

    color emitted(double u, double v, const point3& p) const override {
        return albedo; // Emit the constant color
    }

    bool can_scatter() const override {
        return false; // This material doesn't scatter light
    }

private:
    color albedo;
};
#endif