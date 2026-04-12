//
// Created by jan on 07.07.18.
//

#ifndef PERLINNOISE_POINT_H
#define PERLINNOISE_POINT_H

#include <algorithm>
#include <array>
#include <cmath>
#include <ostream>
#include <type_traits>

namespace noise
{

template<typename T, int Dim>
class point
{
  public:
    static_assert(std::is_arithmetic_v<T>, "Must use an arithmetic type");
    static_assert(Dim > 0, "Dimensionality must be greater than 0");

    using const_iterator = typename std::array<T, Dim>::const_iterator;
    using iterator = typename std::array<T, Dim>::iterator;

    constexpr point() noexcept = default;
    constexpr point(point const&) noexcept = default;
    constexpr point(point&&) noexcept = default;

    template<typename... U>
    constexpr explicit point(U&&... u)
        : m_elems{{std::forward<U>(u)...}}
    {
    }

    ~point() noexcept = default;

    constexpr point& operator=(point const&) noexcept = default;
    constexpr point& operator=(point&&) noexcept = default;

    constexpr bool operator==(point const& other) noexcept { return m_elems == other.m_elems; }
    constexpr bool operator!=(point const& other) noexcept { return !(*this == other); }

    constexpr T const& operator[](std::size_t i) const noexcept { return m_elems[i]; }
    constexpr T& operator[](std::size_t i) noexcept { return m_elems[i]; }

    constexpr const_iterator begin() const noexcept { return m_elems.begin(); }
    constexpr iterator begin() noexcept { return m_elems.begin(); }
    constexpr const_iterator end() const noexcept { return m_elems.end(); }
    constexpr iterator end() noexcept { return m_elems.end(); }

    template<typename U>
    constexpr point<U, Dim> convert_to() const noexcept
    {
        static_assert(std::is_arithmetic_v<U>, "Can only cast to points of other arithmetic types");
        point<U, Dim> result;
        std::transform(m_elems.begin(), m_elems.end(), result.begin(), [](T e) -> U { return e; });
        return result;
    }

    template<typename U = T>
    constexpr point<U, Dim> ceil() const noexcept
    {
        static_assert(std::is_arithmetic_v<U>, "Can only cast to points of other arithmetic types");
        point<U, Dim> result;
        std::transform(
            m_elems.begin(), m_elems.end(), result.begin(), [](T e) -> U { return std::ceil(e); });
        return result;
    }

    template<typename U = T>
    constexpr point<U, Dim> floor() const noexcept
    {
        static_assert(std::is_arithmetic_v<U>, "Can only cast to points of other arithmetic types");
        point<U, Dim> result;
        std::transform(
            m_elems.begin(), m_elems.end(), result.begin(), [](T e) -> U { return std::floor(e); });
        return result;
    }

  private:
    std::array<T, Dim> m_elems{};
};

template<typename T, int Dim>
constexpr std::ostream& operator<<(std::ostream& os, point<T, Dim> const& v) noexcept
{
    os << "( " << v[0]; // every point has at least one element
    for (int i = 1; i < Dim; ++i)
        os << ", " << v[i];
    os << " )";

    return os;
}

template<typename T>
using point2d = point<T, 2>;
using point2d_i = point2d<int>;
using point2d_f = point2d<float>;

template<typename T>
using point3d = point<T, 3>;
using point3d_i = point3d<int>;
using point3d_f = point3d<float>;

template<typename T>
using point4d = point<T, 4>;
using point4d_i = point4d<int>;
using point4d_f = point4d<float>;

} // namespace noise

#endif // PERLINNOISE_POINT_H
