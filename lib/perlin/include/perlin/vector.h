//
// Created by jan on 08.07.18.
//

#ifndef PERLINNOISE_VECTOR_H
#define PERLINNOISE_VECTOR_H

#include "perlin/point.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <random>
#include <type_traits>

namespace noise
{

template<typename T, int Dim>
class vector;

template<typename T, int Dim>
constexpr T magnitude(vector<T, Dim> const& v) noexcept;

template<typename T, int Dim>
class vector : private point<T, Dim>
{
  public:
    static_assert(std::is_floating_point_v<T>, "Must use a floating point type");

    using typename point<T, Dim>::const_iterator;
    using typename point<T, Dim>::iterator;

    using point<T, Dim>::point;
    using point<T, Dim>::operator=;

    template<typename RandEngine>
    constexpr static vector<T, Dim> make_rand_unit_vec(RandEngine&& engine) noexcept
    {
        vector<T, Dim> vec;
        T mag{0};
        std::uniform_real_distribution<T> dist(T{-1}, T{1});
        do
        {
            std::generate(vec.begin(), vec.end(), [&dist, &engine]() { return dist(engine); });
            mag = magnitude(vec);
        } while (mag > T{1});
        vec /= mag;
        return vec;
    }

    constexpr static vector<T, Dim> from_point(point<T, Dim> const& p) noexcept
    {
        vector<T, Dim> vec;
        std::copy(p.begin(), p.end(), vec.begin());
        return vec;
    }

    constexpr static vector<T, Dim> from_value(T val) noexcept
    {
        vector<T, Dim> vec;
        std::fill(vec.begin(), vec.end(), val);
        return vec;
    }

    constexpr bool operator==(vector const& other) noexcept
    {
        return point<T, Dim>::operator==(other);
    }
    constexpr bool operator!=(vector const& other) noexcept { return !(*this == other); }

    using point<T, Dim>::operator[];

    using point<T, Dim>::begin;
    using point<T, Dim>::end;

    constexpr vector& operator+=(vector const& other) noexcept
    {
        std::transform(begin(), end(), other.begin(), begin(), std::plus<T>{});
        return *this;
    }
    constexpr friend vector operator+(vector const& v1, vector const& v2) noexcept
    {
        vector result = v1;
        result += v2;
        return result;
    }

    constexpr vector& operator-=(vector const& other) noexcept
    {
        std::transform(begin(), end(), other.begin(), begin(), std::minus<T>{});
        return *this;
    }
    constexpr friend vector operator-(vector const& v1, vector const& v2) noexcept
    {
        vector result = v1;
        result -= v2;
        return result;
    }
    constexpr vector operator-() const noexcept
    {
        vector result = *this;
        std::transform(begin(), end(), result.begin(), [](T const& e) { return -e; });
        return result;
    }

    constexpr vector& operator*=(T const& scalar) noexcept
    {
        std::transform(begin(), end(), begin(), [&scalar](T& e) { return e * scalar; });
        return *this;
    }
    constexpr friend vector operator*(vector const& v, T const& scalar) noexcept
    {
        vector result = v;
        result *= scalar;
        return result;
    }

    constexpr vector& operator*=(vector const& vec) noexcept
    {
        std::transform(begin(), end(), vec.begin(), begin(), std::multiplies<>());
        return *this;
    }
    constexpr friend vector operator*(vector const& v1, vector const& v2) noexcept
    {
        vector result = v1;
        result *= v2;
        return result;
    }

    constexpr vector& operator/=(T const& scalar) noexcept
    {
        std::transform(begin(), end(), begin(), [&scalar](T& e) { return e / scalar; });
        return *this;
    }
    constexpr friend vector operator/(vector const& v, T const& scalar) noexcept
    {
        vector result = v;
        result /= scalar;
        return result;
    }

    constexpr vector& operator/=(vector const& vec) noexcept
    {
        std::transform(begin(), end(), vec.begin(), begin(), std::divides<>());
        return *this;
    }
    constexpr friend vector operator/(vector const& v1, vector const& v2) noexcept
    {
        vector result = v1;
        result /= v2;
        return result;
    }
};

template<typename T, int Dim>
constexpr T dot(vector<T, Dim> const& v1, vector<T, Dim> const& v2) noexcept
{
    return std::inner_product(v1.begin(), v1.end(), v2.begin(), T{0});
}

template<typename T, int Dim>
constexpr T sq_magnitude(vector<T, Dim> const& v) noexcept
{
    return std::accumulate(
        v.begin(), v.end(), T{0}, [](T const& e1, T const e2) { return e1 + (e2 * e2); });
}

template<typename T, int Dim>
constexpr T magnitude(vector<T, Dim> const& v) noexcept
{
    return std::sqrt(sq_magnitude(v));
}

template<typename T, int Dim>
constexpr vector<T, Dim> normalized(vector<T, Dim> const& v) noexcept
{
    auto const mag = magnitude(v);
    vector result = v;
    std::transform(
        result.begin(), result.end(), result.begin(), [mag](T const& e) { return e / mag; });
    return result;
}

template<typename T, int Dim>
constexpr std::ostream& operator<<(std::ostream& os, vector<T, Dim> const& v) noexcept
{
    os << "[ " << v[0]; // every vector has at least one element
    for (int i = 1; i < Dim; ++i)
        os << ", " << v[i];
    os << " ]";

    return os;
}

template<typename T>
using vec2d = vector<T, 2>;
using vec2d_f = vec2d<float>;

template<typename T>
using vec3d = vector<T, 3>;
using vec3d_f = vec3d<float>;

template<typename T>
using vec4d = vector<T, 4>;
using vec4d_f = vec4d<float>;

} // namespace noise

#endif // PERLINNOISE_VECTOR_H
