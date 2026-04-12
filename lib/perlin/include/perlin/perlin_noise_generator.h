//
// Created by jan on 07.07.18.
//

#ifndef PERLINNOISE_PERLIN_NOISE_GENERATOR_H
#define PERLINNOISE_PERLIN_NOISE_GENERATOR_H

#include "perlin/math.h"
#include "perlin/point.h"
#include "perlin/vector.h"

#include <algorithm>
#include <bitset>
#include <numeric>
#include <random>

namespace noise
{
/**
 * Generates perlin noise in arbitrary dimensions.
 *
 * @details Perlin noise is a type of coherent noise. As such it has three properties:
 *          1. Evaluating it at the same point will always give the same result
 *          2. A small change in input will yield a small change in output
 *          3. A large change in input results in a random change in output
 *
 *          Noise is generated in the [-1,1] domain.
 *
 * @tparam Dim          Dimensionality of the noise function
 * @tparam Smoothness   Order of smoothstep function to use for interpolation
 * @tparam Result       Arithmetic result type
 * @tparam NumGradients Amount of random gradients to use. A larger number results in more
 * randomness, but longer computation times.
 * @tparam GridCoord    Integral grid coordinate type
 */
template<int Dim,
         int Smoothness = 2,
         typename Result = float,
         int NumGradients = 256,
         typename GridCoord = int>
class perlin_noise_generator
{
  public:
    static_assert(Dim > 0, "Must have at least one dimension");
    static_assert(Smoothness >= 0, "Smoothness must be positive");
    static_assert(NumGradients > 0, "Must allow at least one pre-computed gradient");
    static_assert(std::is_arithmetic_v<Result>, "Result must be an arithmetic type");
    static_assert(std::is_integral_v<GridCoord>, "GridCoord must be an integral type");

    using result_t = Result;
    using grid_coord_t = GridCoord;

    static constexpr const int dimensions = Dim;
    static constexpr const int smoothness = Smoothness;

    /**
     * @param seed Random seed for noise generation
     */
    template<class RndEngine = std::default_random_engine>
    explicit perlin_noise_generator(RndEngine&& rnd) noexcept
    {
        std::iota(m_permutations.begin(), m_permutations.end(), result_t{0});
        std::shuffle(m_permutations.begin(), m_permutations.end(), std::forward<RndEngine>(rnd));
        std::generate(m_gradients.begin(), m_gradients.end(), [&]() {
            return vector<result_t, Dim>::make_rand_unit_vec(std::forward<RndEngine>(rnd));
        });
    }

    /**
     * Evaluate the noise function at a given point.
     *
     * @param p Point of evaluation
     * @return  Noise function value at the specified point
     */
    result_t at(point<result_t, Dim> const& p) const noexcept
    {
        // Compute neighboring grid points
        constexpr int const NumNeighbors = ipow(2, Dim);
        auto baseGridPoint = p.template floor<grid_coord_t>();
        std::array<point<grid_coord_t, Dim>, NumNeighbors> m_nodes{};
        std::generate(m_nodes.begin(), m_nodes.end(), [n = 0u, &baseGridPoint]() mutable {
            std::bitset<Dim> bits(n++);
            auto gridPoint = baseGridPoint;
            for (unsigned d = 0; d < Dim; ++d)
            {
                if (bits.test(d))
                    gridPoint[d]++;
            }
            return gridPoint;
        });

        // Vectors from neighboring grid nodes to point
        std::array<vector<result_t, Dim>, NumNeighbors> m_nodeVecs;
        std::transform(m_nodes.begin(),
                       m_nodes.end(),
                       m_nodeVecs.begin(),
                       [&p](point<grid_coord_t, Dim> const& n) {
                           return vector<result_t, Dim>::from_point(p)
                                  - vector<result_t, Dim>::from_point(
                                      n.template convert_to<result_t>());
                       });

        // Compute dot products between node vectors and gradients
        std::array<result_t, NumNeighbors> dot_products{};
        std::transform(m_nodes.begin(),
                       m_nodes.end(),
                       m_nodeVecs.begin(),
                       dot_products.begin(),
                       [this](point<grid_coord_t, Dim> const& n, vector<result_t, Dim> const& nv) {
                           return dot(gradient_at(n), nv);
                       });

        // Interpolate dot products
        int s = NumNeighbors;
        for (int d = 0; d < Dim; ++d)
        {
            // Iterate neighbors
            for (int i = 0; i < s; i += 2)
            {
                dot_products[i / 2] = dot_products[i]
                                      + smoothstep<Smoothness>(p[d] - baseGridPoint[d])
                                            * (dot_products[i + 1] - dot_products[i]);
            }
            s /= 2;
        }

        // Account for rounding errors
        return std::clamp(dot_products[0], static_cast<result_t>(-1), static_cast<result_t>(1));
    }

  private:
    std::array<int, NumGradients> m_permutations{};
    std::array<vector<result_t, Dim>, NumGradients> m_gradients{};

    constexpr vector<result_t, Dim> const& gradient_at(point<grid_coord_t, Dim> const& point) const
        noexcept
    {
        grid_coord_t idx = mod(point[Dim - 1], NumGradients);
        for (int i = Dim - 2; i >= 0; --i)
            idx = mod((point[i] + m_permutations[idx]), NumGradients);

        return m_gradients[idx];
    }
};

} // namespace noise

#endif // PERLINNOISE_PERLIN_NOISE_GENERATOR_H
