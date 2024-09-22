#ifndef RANDOM_H 
#define RANDOM_H

class FastRandom {
private:
    uint64_t state;

    // Constants for a good Xorshift64* generator
    static constexpr uint64_t MULTIPLIER = 0x2545F4914F6CDD1DULL;

    FastRandom(uint64_t seed = 0) : state(seed ? seed : 0x853c49e6748fea9bULL) {}

public:
    static FastRandom& getInstance() {
        static FastRandom instance;
        return instance;
    }

    void seed(uint64_t seed) {
        state = seed ? seed : 0x853c49e6748fea9bULL;
    }

    inline uint64_t next() {
        uint64_t x = state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        state = x;
        return x * MULTIPLIER;
    }

    inline double random_double() {
        return (next() >> 11) * (1.0 / (1ULL << 53));
    }

    // Delete copy constructor and assignment operator
    FastRandom(const FastRandom&) = delete;
    FastRandom& operator=(const FastRandom&) = delete;
};
inline double random_double() {
    // Returns a random real in [0,1).
    return FastRandom::getInstance().random_double();
    //return std::rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) {
    // Returns a random real in [min,max).
    return min + (max-min)*random_double();
}
inline int random_int(int min, int max) {
    // Returns a random integer in [min,max].
    return int(random_double(min, max+1));
}


#endif