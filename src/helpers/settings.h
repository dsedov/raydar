#ifndef SETTINGS_H
#define SETTINGS_H

#include "cxxopts.hpp"
#include <iostream>

class settings {
  public:
    int image_width = 1024;
    int image_height = 768;
    int samples = 4;
    int max_depth = 10;
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
            ("h,help", "Print usage");

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
        

        // SAMPLES
        if (result.count("samples")) samples = result["samples"].as<int>();

        std::cout << "Samples: " << samples << std::endl;
    }

};

#endif