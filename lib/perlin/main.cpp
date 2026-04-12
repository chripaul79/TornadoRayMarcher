#include "perlin/fractal_noise_generator.h"
#include "perlin/perlin_noise_generator.h"
#include "perlin/point.h"
#include "perlin/seamless_noise_generator_2d.h"
#include "perlin/vector.h"

#include <chrono>
#include <iostream>
#include <map>

/*
 * NOTE: Most of this is just boilerplate code to write pngs to disk. Look at
 *       main() at the end of the file to see where the noise generation takes
 *       place.
 */

using namespace noise;

template<typename Gen>
struct noisemap
{
    int m_width;
    int m_height;
    std::vector<typename Gen::result_t> m_values;

    noisemap(int width, int height)
        : m_width(width)
        , m_height(height)
    {
        m_values.resize(width * height);
    }

    noisemap() = default;
};

template<class Gen>
noisemap<Gen> generate_noise_map(
    Gen& gen,
    int width,
    int height,
    int cellsX,
    int cellsY,
    std::function<point<typename Gen::result_t, Gen::dimensions>(typename Gen::result_t,
                                                                 typename Gen::result_t)> mapPoint)
{
    noisemap<Gen> result{width, height};

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float xPos = (float)x / (width / (float)cellsX);
            float yPos = (float)y / (height / (float)cellsY);
            auto val = std::clamp(gen.at(mapPoint(xPos, yPos)), -1.f, 1.f);
            result.m_values[x + width * y] = val;
        }
    }

    return result;
}

template<class Gen>
int generate_png(noisemap<Gen> const& map, std::string const& filename)
{
    char const* title = filename.c_str();
    FILE* fp = nullptr;
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    auto finalize = [&]() {
        if (fp != nullptr)
            fclose(fp);
        if (info_ptr != nullptr)
            png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        if (png_ptr != nullptr)
            png_destroy_write_struct(&png_ptr, (png_infopp) nullptr);
    };

    // Open file for writing (binary mode)
    fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr)
    {
        fprintf(stderr, "Could not open file %s for writing\n", filename.c_str());
        finalize();
        return 1;
    }

    // Initialize write structure
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png_ptr == nullptr)
    {
        fprintf(stderr, "Could not allocate write struct\n");
        finalize();
        return 1;
    }

    // Initialize info structure
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr)
    {
        fprintf(stderr, "Could not allocate info struct\n");
        finalize();
        return 1;
    }

    // Setup Exception handling
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fprintf(stderr, "Error during png creation\n");
        finalize();
        return 1;
    }

    png_init_io(png_ptr, fp);

    // Write header (8 bit grayscale depth)
    png_set_IHDR(png_ptr,
                 info_ptr,
                 map.m_width,
                 map.m_height,
                 8,
                 PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    // Set title
    if (title != nullptr)
    {
        png_text title_text;
        title_text.compression = PNG_TEXT_COMPRESSION_NONE;
        title_text.key = const_cast<char*>("Title");
        title_text.text = const_cast<char*>(title);
        png_set_text(png_ptr, info_ptr, &title_text, 1);
    }

    png_write_info(png_ptr, info_ptr);

    // Allocate memory for one row (1 bytes per pixel - Grayscale)
    std::vector<png_byte> row;
    row.resize(map.m_width);

    // Write image data
    for (int y = 0; y < map.m_height; y++)
    {
        for (int x = 0; x < map.m_width; x++)
        {
            auto val = map.m_values[x + map.m_width * y];
            row[x] = png_byte((val + 1) / 2.f * 255);

            // Mark grid corners
            //            if (((std::ceil(xPos) - xPos) < 0.000001f) && ((std::ceil(yPos) - yPos) <
            //            0.000001f))
            //                row[x] = 255;
        }
        png_write_row(png_ptr, row.data());
    }

    // End write
    png_write_end(png_ptr, nullptr);
    finalize();

    return 0;
}

template<class Gen>
int generate_world_png(noisemap<Gen> const& map, std::string const& filename)
{
    char const* title = filename.c_str();
    FILE* fp = nullptr;
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    auto finalize = [&]() {
        if (fp != nullptr)
            fclose(fp);
        if (info_ptr != nullptr)
            png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        if (png_ptr != nullptr)
            png_destroy_write_struct(&png_ptr, (png_infopp) nullptr);
    };

    // Open file for writing (binary mode)
    fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr)
    {
        fprintf(stderr, "Could not open file %s for writing\n", filename.c_str());
        finalize();
        return 1;
    }

    // Initialize write structure
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png_ptr == nullptr)
    {
        fprintf(stderr, "Could not allocate write struct\n");
        finalize();
        return 1;
    }

    // Initialize info structure
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr)
    {
        fprintf(stderr, "Could not allocate info struct\n");
        finalize();
        return 1;
    }

    // Setup Exception handling
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fprintf(stderr, "Error during png creation\n");
        finalize();
        return 1;
    }

    png_init_io(png_ptr, fp);

    // Write header (8 bit rgb depth)
    png_set_IHDR(png_ptr,
                 info_ptr,
                 map.m_width,
                 map.m_height,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    // Set title
    if (title != nullptr)
    {
        png_text title_text;
        title_text.compression = PNG_TEXT_COMPRESSION_NONE;
        title_text.key = const_cast<char*>("Title");
        title_text.text = const_cast<char*>(title);
        png_set_text(png_ptr, info_ptr, &title_text, 1);
    }

    png_write_info(png_ptr, info_ptr);

    // Allocate memory for one row (3 bytes per pixel - RGB)
    std::vector<png_byte> row;
    row.resize(map.m_width * 3);

    // Color scale
    using Color = std::array<png_byte, 3>;
    std::map<float, Color> colors{
        {0.55f, {255, 255, 255}}, // snow
        {0.4f, {150, 150, 160}},  // mountains
        {0.3f, {60, 130, 30}},    // forest
        {0.25f, {120, 190, 90}},  // grass
        {0.2f, {229, 221, 0}},    // shore
        {0.15f, {80, 80, 255}},   // shallow water
        {-1.f, {0, 0, 255}},      // ocean
    };

    // Write image data
    for (int y = 0; y < map.m_height; y++)
    {
        for (int x = 0; x < map.m_width; x++)
        {
            auto val = map.m_values[x + map.m_width * y];
            auto colorIter = colors.lower_bound(val);
            if (colorIter == colors.begin())
            {
                std::cerr << "No color for " << val << std::endl;
                continue;
            }
            --colorIter;
            auto color = colorIter->second;
            row[(x * 3) + 0] = color[0];
            row[(x * 3) + 1] = color[1];
            row[(x * 3) + 2] = color[2];
        }
        png_write_row(png_ptr, row.data());
    }

    // End write
    png_write_end(png_ptr, nullptr);
    finalize();

    return 0;
}

int main()
{
    constexpr int const cellsX = 6;
    constexpr int const cellsY = 4;
    int const width = 800;
    int const height = width * (2.f / 3.f);
    std::uint_fast32_t seed = 20;
    constexpr int smoothness = 2;
    constexpr int octaves = 10;

    using perlin_gen = perlin_noise_generator<4, smoothness>;
    using fractal_gen = fractal_noise_generator<perlin_gen,
                                                octaves,
                                                exponential_decay<float>,
                                                exponential_growth<float>>;
    using seamless_gen = seamless_noise_generator_2d<fractal_gen, cellsX, cellsY>;
    noisemap<seamless_gen> map;

    auto start = std::chrono::steady_clock::now();

    seamless_gen gen{std::mt19937{seed}};
    map = generate_noise_map(gen, width, height, cellsX, cellsY, [](float x, float y) {
        return point2d_f{x, y};
    });

    auto end = std::chrono::steady_clock::now();
    std::cout << "Generating the noise map took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms"
              << std::endl;

    generate_png(map, std::to_string(seed) + "_seamless.png");
    generate_world_png(map, std::to_string(seed) + "_world.png");

    return 0;
}