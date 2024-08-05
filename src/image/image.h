#pragma once
#include <iostream>

class Image {
public:
    Image(int width, int height);
    virtual ~Image() {}

    virtual void save(const char * filename) = 0;


protected:
    int width_;
    int height_;
};
