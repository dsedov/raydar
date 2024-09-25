#ifndef SETTINGS_H
#define SETTINGS_H

#include "cxxopts.hpp"
#include "strings.h"
#include <iostream>

class settings {
  public:
    int image_width = 1024; 
    int image_height = 768;
    float gamma = 1.0;
    float gain = 1.0;
    int samples = 4;
    int max_depth = 3;
    bool show_ui = false;
    std::string usd_file;
    std::string image_file = "output.png";

    int error = 0;

    settings(int argc, char* argv[]) {
        cxxopts::Options options("Raydar", "Spectral USD Renderer");
        // Define options for file, resolution, and samples
        options.add_options()
            ("f,file", "USD file name", cxxopts::value<std::string>())
            ("i,image", "Output file name", cxxopts::value<std::string>())
            ("r,resolution", "Resolution (two integers)", cxxopts::value<std::vector<int>>()->default_value("1024,768"))
            ("s,samples", "Number of samples", cxxopts::value<int>()->default_value("4"))
            ("d,depth", "Max depth", cxxopts::value<int>()->default_value("10"))
            ("h,help", "Print usage")
            ("gm,gamma", "Gamma", cxxopts::value<float>()->default_value("2.2"))
            ("gn,gain", "Gain", cxxopts::value<float>()->default_value("2.7"))
            ("ui", "Show UI", cxxopts::value<bool>()->default_value("false"));

        auto result = options.parse(argc, argv);    
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            error = 1; 
            return;
        }

        // USD FILE
        if (result.count("file")) {
            usd_file = result["file"].as<std::string>();
            std::cout << "File: " << usd_file << std::endl;
        } else {
            std::cerr << "Error: File name must be provided." << std::endl;
            error = 1; 
            return;
        }

        // IMAGE FILE
        if (result.count("image")) image_file = result["image"].as<std::string>();
        std::cout << "Image: " << image_file << std::endl;

        // RESOLUTION
        if (result.count("resolution")) {
            std::vector<int> resolution = result["resolution"].as<std::vector<int>>();
            if (resolution.size() != 2) {
                std::cerr << "Error: Resolution must consist of exactly two integers." << std::endl;
                error = 1; 
                return;
            }
            std::cout << "Resolution: " << resolution[0] << "x" << resolution[1] << std::endl;
            image_width = resolution[0];
            image_height = resolution[1];
        } 
        std::cout << "Resolution: " << image_width << "x" << image_height << std::endl;

        // MAX DEPTH
        if (result.count("depth")) max_depth = result["depth"].as<int>();
        std::cout << "Max depth: " << max_depth << std::endl;

        // SAMPLES
        if (result.count("samples")) samples = result["samples"].as<int>();

        std::cout << "Samples: " << samples << std::endl;

        // UI
        if (result.count("ui")) show_ui = result["ui"].as<bool>();
        std::cout << "Show UI: " << show_ui << std::endl;

        // GAMMA
        if (result.count("gamma")) gamma = result["gamma"].as<float>();
        std::cout << "Gamma: " << gamma << std::endl;

        // GAIN
        if (result.count("gain")) gain = result["gain"].as<float>();
        std::cout << "Gain: " << gain << std::endl;
    }

    std::string get_file_name(int width, int height, int samples, int seconds, bool with_extension = true) const{
        std::string file_name = image_file;
        size_t dot_pos = file_name.find_last_of(".");
        std::string name = file_name.substr(0, dot_pos);
        std::string extension = file_name.substr(dot_pos);

        // Create the new file name with resolution and samples
        std::stringstream new_file_name;
  
        new_file_name << name << "_" << width << "x" << height << "_" << samples << "spp" << "_" << seconds << "s" << "_" << rd::strings::get_uuid();
        if (with_extension) new_file_name << extension;
        return new_file_name.str();
    }

};

#endif