/**********************************************************
 * @file   math.h
 * @author jan
 * @date   7/17/18
 * ********************************************************
 * @brief
 * @details
 **********************************************************/
#ifndef PERLINNOISE_MATH_H
#define PERLINNOISE_MATH_H

#include <algorithm>
#include <type_traits>

namespace noise
{
/**
 * Iterate a compile-time constant number of times and call a function on each iteration
 * @tparam F       Function type
 * @tparam S       Call function with these compile-time constant integers
 * @param function Function to call
 */
template<typename F, std::size_t... S>
constexpr void static_for(F&& function, std::index_sequence<S...>)
{
    [[maybe_unused]] int unpack[] = {0, (function(std::integral_constant<std::size_t, S>{}), 0)...};
}

/**
 * Iterate a compile-time constant number of times and call a function on each iteration
 * @tparam N          Number of iterations
 * @tparam F          Function type
 * @param function    Function to call
 */
template<std::size_t N, typename F>
constexpr void static_for(F&& function)
{
    static_for(std::forward<F>(function), std::make_index_sequence<N>());
}

/**
 * Mathematical constants
 * @tparam T Arithmetic type
 */
template<typename T>
struct constants
{
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    constexpr static T const pi = 3.1415926535897932384626433832795028841971;
    constexpr static T const e = 2.71828182845904523536028747135266249775724709369995;
};

/**
 * Compute integer powers with non-negative exponent
 * @tparam T   Input and return integral type
 * @param base Base
 * @param exp  Exponent
 * @return     Result of the power. Returns 1 if both base and exponent are 0.
 */
template<typename T>
constexpr T ipow(T base, unsigned int exp) noexcept
{
    static_assert(std::is_integral_v<T>, "Can only compute integer powers");

    if (exp == 0)
        return 1;

    T temp = ipow(base, exp / 2);
    if ((exp % 2) == 0)
        return temp * temp;
    else
        return base * temp * temp;
}
static_assert(ipow(0, 0) == 1);
static_assert(ipow(1, 0) == 1);
static_assert(ipow(0, 1) == 0);
static_assert(ipow(1, 22) == 1);
static_assert(ipow(2, 2) == 4);
static_assert(ipow(-1, 1) == -1);
static_assert(ipow(-1, 2) == 1);
static_assert(ipow(-2, 2) == 4);
static_assert(ipow(-2, 1) == -2);

/**
 * Compute powers with integer exponent
 * @tparam T   Input and return arithmetic type
 * @param base Base
 * @param exp  Exponent
 * @return     Result of the power. Returns 1 if both base and exponent are 0.
 */
template<typename T>
constexpr T powi(T base, int exp) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    if (exp == 0)
        return 1;
    T result = powi(base, exp / 2);
    if ((exp % 2) == 0)
        return result * result;
    else
    {
        if (exp > 0)
            return base * result * result;
        else
            return (result * result) / base;
    }
}
static_assert(powi(0, 0) == 1);
static_assert(powi(1, 0) == 1);
static_assert(powi(0, 1) == 0);
static_assert(powi(1, 22) == 1);
static_assert(powi(2, 2) == 4);
static_assert(powi(-1, 1) == -1);
static_assert(powi(-1, 2) == 1);
static_assert(powi(-2, 2) == 4);
static_assert(powi(-2, 1) == -2);
static_assert(powi(1, -1) == 1);
static_assert(powi(2.f, -1) == 0.5f);
static_assert(powi(1, -1) == 1);
static_assert(powi(2.f, -3) == 0.125f);
static_assert(powi(2.f, 7) == 128.f);

/**
 * Compute powers with compile-time integer exponent
 * @tparam Exp Exponent
 * @tparam T   Arithmetic type
 * @param base Base
 * @return     Result of the power. Returns 1 if both base and exponent are 0.
 */
template<int Exp, typename T>
constexpr inline T static_powi(T base) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    if constexpr (Exp == 0)
    {
        return 1;
    }
    else if constexpr (Exp == 1)
    {
        return base;
    }
    else if constexpr (Exp == 2)
    {
        return base * base;
    }
    else if constexpr (Exp == 3)
    {
        return base * base * base;
    }
    else if constexpr (Exp == 4)
    {
        auto squared = base * base;
        return squared * squared;
    }
    else if constexpr (Exp == 5)
    {
        auto squared = base * base;
        return squared * squared * base;
    }
    else
    {
        T result = static_powi<Exp / 2>(base);
        if constexpr ((Exp % 2) == 0)
            return result * result;
        else
        {
            if constexpr (Exp > 0)
                return base * result * result;
            else
                return (result * result) / base;
        }
    }
}
static_assert(static_powi<0>(0) == 1);
static_assert(static_powi<0>(1) == 1);
static_assert(static_powi<1>(0) == 0);
static_assert(static_powi<22>(1) == 1);
static_assert(static_powi<2>(2) == 4);
static_assert(static_powi<1>(-1) == -1);
static_assert(static_powi<2>(-1) == 1);
static_assert(static_powi<2>(-2) == 4);
static_assert(static_powi<1>(-2) == -2);
static_assert(static_powi<-1>(1) == 1);
static_assert(static_powi<-1>(2.f) == 0.5f);
static_assert(static_powi<-1>(1) == 1);
static_assert(static_powi<-3>(2.f) == 0.125f);
static_assert(static_powi<7>(2.f) == 128.f);

/**
 * Binomial coefficient
 * @tparam T Integral type
 * @param n  Total number of elements
 * @param k  Number of elements to pick
 * @return   Number of k-element subsets from a set with n elements
 */
template<typename T>
constexpr T n_choose_k(T n, T k) noexcept
{
    static_assert(std::is_integral_v<T>, "must be an integral type");

    if (k > n)
        return 0;
    if (2 * k > n)
        k = n - k;
    if (k == 0)
        return 1;

    T val = 1;
    for (T i = 1; i <= k; ++i)
        val = val * (n - k + i) / i;
    return val;
}
static_assert(n_choose_k(0, 0) == 1);
static_assert(n_choose_k(1, 0) == 1);
static_assert(n_choose_k(0, 1) == 0);
static_assert(n_choose_k(1, 1) == 1);
static_assert(n_choose_k(0, 0) == 1);
static_assert(n_choose_k(2, 1) == 2);
static_assert(n_choose_k(5, 2) == 10);
static_assert(n_choose_k(2, 0) == 1);
static_assert(n_choose_k(3, 1) == 3);
static_assert(n_choose_k(5, 1) == 5);
static_assert(n_choose_k(4, 2) == 6);
static_assert(n_choose_k(5, 0) == 1);
static_assert(n_choose_k(3, 0) == 1);
static_assert(n_choose_k(7, 3) == 35);
static_assert(n_choose_k(4, 1) == 4);
static_assert(n_choose_k(7, 2) == 21);
static_assert(n_choose_k(6, 3) == 20);

/**
 * Smoothstep function
 *
 * @details  Maps values from input range [0,1] to output range [0,1], where 0 and 1 are mapped to
 * itself and values in-between are interpolated smoothly. Values less than 0 are mapped to 0.
 * Values greater than 1 are mapped to 1.
 * @tparam N Function order
 * @tparam T Arithmetic result type
 * @param x  Evaluation point
 * @return
 */
template<int N, typename T>
constexpr T smoothstep(T x) noexcept
{
    static_assert(N >= 0, "N must be a nonnegative integer");
    static_assert(std::is_arithmetic_v<T>, "T must be arithemtic type");

    if constexpr (N == 0)
    {
        return std::clamp<T>(x, 0, 1);
    }
    else
    {
        if (x <= 0)
            return 0;
        else if (x < 1)
        {
            if constexpr (N == 1)
            {
                return 3 * static_powi<2>(x) - 2 * static_powi<3>(x);
            }
            else if constexpr (N == 2)
            {
                return 6 * static_powi<5>(x) - 15 * static_powi<4>(x) + 10 * static_powi<3>(x);
            }
            else
            {
                T sum = 0;
                static_for<N + 1>([&sum, &x](auto n) {
                    sum += n_choose_k<int>(N + n, n) * n_choose_k<int>(2 * N + 1, N - n)
                           * static_powi<n>(-x);
                });
                return static_powi<N + 1>(x) * sum;
            }
        }
        else
            return 1;
    }
}
static_assert(smoothstep<0>(0.0) - 0 < 1e-5);
static_assert(smoothstep<0>(0.3) - 0.3 < 1e-5);
static_assert(smoothstep<0>(0.5) - 0.5 < 1e-5);
static_assert(smoothstep<0>(1.0) - 1 < 1e-5);
static_assert(smoothstep<1>(0.0) - 0 < 1e-5);
static_assert(smoothstep<1>(0.3) - 0.216 < 1e-5);
static_assert(smoothstep<1>(0.5) - 0.5 < 1e-5);
static_assert(smoothstep<1>(1.0) - 1 < 1e-5);
static_assert(smoothstep<2>(0.0) - 0 < 1e-5);
static_assert(smoothstep<2>(0.3) - 0.16308 < 1e-5);
static_assert(smoothstep<2>(0.5) - 0.5 < 1e-5);
static_assert(smoothstep<2>(1.0) - 1 < 1e-5);

/**
 * Modulo for integers
 *
 * @details  Unlike the remainder operator (%), this works correctly on negative k
 * @tparam T Arithmetic result type
 * @param k  Integer
 * @param n  Positive integer
 * @return   The result of k mod n
 */
template<typename T>
constexpr T mod(T k, T n) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    return ((k %= n) < 0) ? k + n : k;
}
static_assert(mod(0, 3) == 0);
static_assert(mod(1, 3) == 1);
static_assert(mod(2, 3) == 2);
static_assert(mod(3, 3) == 0);
static_assert(mod(5, 3) == 2);
static_assert(mod(6, 3) == 0);
static_assert(mod(7, 3) == 1);
static_assert(mod(-1, 3) == 2);
static_assert(mod(-2, 3) == 1);
static_assert(mod(-3, 3) == 0);
static_assert(mod(-5, 3) == 1);
static_assert(mod(-6, 3) == 0);
static_assert(mod(-7, 3) == 2);

} // namespace noise
#endif // PERLINNOISE_MATH_H
