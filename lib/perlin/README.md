# C++17 Perlin noise

[![Build Status](https://travis-ci.com/jan-moeller/perlin.svg?token=LF8KGTXMvYk5dRNjQs4Q&branch=master)](https://travis-ci.com/jan-moeller/perlin)

This is a modern C++ implementation of the Perlin noise algorithm ('modern' as in 'C++17'). It can generate noise in
arbitrary dimensions, makes use of modern compile-time computation wherever possible and has a couple of customization
points.

Some examples:
- Evaluate a 2d Perlin noise function (seeded with 42) at point (0, 0):
    ```cpp
    constexpr int const Dim = 2;
    constexpr int const Smoothness = 1;
    constexpr std::uint_fast32_t const Seed = 42;
    perlin_noise_generator<Dim, Smoothness> gen(std::mt19937{Seed});
    float val = gen.at(point2d_f(0.f, 0.f));
    ```
    The second template parameter (1) denotes the order of the smoothstep function to use for gradient interpolation.
    1 corresponds to Perlin's classic implementation, 2 is his improved technique; other non-negative values are also
    possible.
    
- Layer multiple 2d perlin noise functions to generate fractal noise, then evaluate the combined function at point (0, 0).
    ```cpp
    constexpr int const Dim = 2;
    constexpr int const Smoothness = 1;
    using PerlinNoise = perlin_noise_generator<Dim, Smoothness>;
    constexpr int const Octaves = 4;
    using WeightFun = exponential_decay<float>;
    using FrequencyFun = exponential_growth<float>;
    using Gen = fractal_noise_generator<PerlinNoise, Octaves, WeightFun, FrequencyFun>;
    Gen gen(std::mt19937{42});
    float val = gen.at(point2d_f(0.f, 0.f));
    ```
    Here, 4 octaves are generated, where amplitude decays exponentially with
    <a href="https://www.codecogs.com/eqnedit.php?latex=0.6^i" target="_blank"><img src="https://latex.codecogs.com/gif.latex?0.6^i" title="0.6^i" /></a>
    and frequency grows exponentially with
    <a href="https://www.codecogs.com/eqnedit.php?latex=2^i" target="_blank"><img src="https://latex.codecogs.com/gif.latex?2^i" title="2^i" /></a>.
    Other decay/growth policies are available.
    
- A special 2d noise generator exists which turns 4d noise into seamless 2d noise:
    ```cpp
    using PerlinNoise = perlin_noise_generator<4>;
    constexpr int const Width = 2;
    constexpr int const Height = 3;
    using Gen = seamless_noise_generator_2d<PerlinNoise, Width, Height>;
    Gen gen(std::mt19937{42});
    float val = gen.at(point2d_f(0.f, 0.f));
    ``` 
    Here a default 4d perlin noise is made to loop in the range [0, 2] on the x axis and range [0, 3] on the y axis.
    Of course you can also make fractal noise tileable like this.
    