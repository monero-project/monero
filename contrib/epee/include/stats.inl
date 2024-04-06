#include <math.h>
#include <limits>
#include <algorithm>
#include "misc_language.h"
#include "stats.h"

enum
{
  bit_min = 0,
  bit_max,
  bit_median,
  bit_mean,
  bit_standard_deviation,
  bit_standard_error,
  bit_variance,
  bit_kurtosis,
};

static inline double square(double x)
{
  return x * x;
}

template<typename T>
static inline double interpolate(T v, T v0, double i0, T v1, double i1)
{
  return i0 + (i1 - i0) * (v - v0) / (v1 - v0);
}

template<typename T, typename Tpod>
inline bool Stats<T, Tpod>::is_cached(int bit) const
{
  return cached & (1<<bit);
}

template<typename T, typename Tpod>
inline void Stats<T, Tpod>::set_cached(int bit) const
{
  cached |= 1<<bit;
}

template<typename T, typename Tpod>
size_t Stats<T, Tpod>::get_size() const
{
  return values.size();
}

template<typename T, typename Tpod>
Tpod Stats<T, Tpod>::get_min() const
{
  if (!is_cached(bit_min))
  {
    min = std::numeric_limits<Tpod>::max();
    for (const T &v: values)
      min = std::min<Tpod>(min, v);
    set_cached(bit_min);
  }
  return min;
}

template<typename T, typename Tpod>
Tpod Stats<T, Tpod>::get_max() const
{
  if (!is_cached(bit_max))
  {
    max = std::numeric_limits<Tpod>::min();
    for (const T &v: values)
      max = std::max<Tpod>(max, v);
    set_cached(bit_max);
  }
  return max;
}

template<typename T, typename Tpod>
Tpod Stats<T, Tpod>::get_median() const
{
  if (!is_cached(bit_median))
  {
    std::vector<Tpod> sorted;
    sorted.reserve(values.size());
    for (const T &v: values)
      sorted.push_back(v);
    std::sort(sorted.begin(), sorted.end());
    if (sorted.size() & 1)
    {
      median = sorted[sorted.size() / 2];
    }
    else
    {
      median = epee::misc_utils::get_mid(sorted[(sorted.size() - 1) / 2], sorted[sorted.size() / 2]);
    }
    set_cached(bit_median);
  }
  return median;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_mean() const
{
  if (values.empty())
    return 0.0;
  if (!is_cached(bit_mean))
  {
    mean = 0.0;
    for (const T &v: values)
      mean += v;
    mean /= values.size();
    set_cached(bit_mean);
  }
  return mean;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_cdf95(size_t df) const
{
  static const double p[101] = {
    -1, 12.706, 4.3027, 3.1824, 2.7765, 2.5706, 2.4469, 2.3646, 2.3060, 2.2622, 2.2281, 2.2010, 2.1788, 2.1604, 2.1448, 2.1315,
    2.1199, 2.1098, 2.1009, 2.0930, 2.0860, 2.0796, 2.0739, 2.0687, 2.0639, 2.0595, 2.0555, 2.0518, 2.0484, 2.0452, 2.0423, 2.0395,
    2.0369, 2.0345, 2.0322, 2.0301, 2.0281, 2.0262, 2.0244, 2.0227, 2.0211, 2.0195, 2.0181, 2.0167, 2.0154, 2.0141, 2.0129, 2.0117,
    2.0106, 2.0096, 2.0086, 2.0076, 2.0066, 2.0057, 2.0049, 2.0040, 2.0032, 2.0025, 2.0017, 2.0010, 2.0003, 1.9996, 1.9990, 1.9983,
    1.9977, 1.9971, 1.9966, 1.9960, 1.9955, 1.9949, 1.9944, 1.9939, 1.9935, 1.9930, 1.9925, 1.9921, 1.9917, 1.9913, 1.9908, 1.9905,
    1.9901, 1.9897, 1.9893, 1.9890, 1.9886, 1.9883, 1.9879, 1.9876, 1.9873, 1.9870, 1.9867, 1.9864, 1.9861, 1.9858, 1.9855, 1.9852,
    1.9850, 1.9847, 1.9845, 1.9842, 1.9840,
  };
  if (df <= 100)
    return p[df];
  if (df <= 120)
    return interpolate<size_t>(df, 100, 1.9840, 120, 1.98);
  return 1.96;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_cdf95(const Stats<T> &other) const
{
  return get_cdf95(get_size() + other.get_size() - 2);
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_cdf99(size_t df) const
{
  static const double p[101] = {
    -1, 9.9250, 5.8408, 4.6041, 4.0321, 3.7074, 3.4995, 3.3554, 3.2498, 3.1693, 3.1058, 3.0545, 3.0123, 2.9768, 2.9467, 2.9208, 2.8982,
     2.8784, 2.8609, 2.8453, 2.8314, 2.8188, 2.8073, 2.7970, 2.7874, 2.7787, 2.7707, 2.7633, 2.7564, 2.7500, 2.7440, 2.7385, 2.7333,
     2.7284, 2.7238, 2.7195, 2.7154, 2.7116, 2.7079, 2.7045, 2.7012, 2.6981, 2.6951, 2.6923, 2.6896, 2.6870, 2.6846, 2.6822, 2.6800,
     2.6778, 2.6757, 2.6737, 2.6718, 2.6700, 2.6682, 2.6665, 2.6649, 2.6633, 2.6618, 2.6603, 2.6589, 2.6575, 2.6561, 2.6549, 2.6536,
     2.6524, 2.6512, 2.6501, 2.6490, 2.6479, 2.6469, 2.6458, 2.6449, 2.6439, 2.6430, 2.6421, 2.6412, 2.6403, 2.6395, 2.6387, 2.6379,
     2.6371, 2.6364, 2.6356, 2.6349, 2.6342, 2.6335, 2.6329, 2.6322, 2.6316, 2.6309, 2.6303, 2.6297, 2.6291, 2.6286, 2.6280, 2.6275,
     2.6269, 2.6264, 2.6259,
  };
  if (df <= 100)
    return p[df];
  if (df <= 120)
    return interpolate<size_t>(df, 100, 2.6529, 120, 2.617);
  return 2.576;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_cdf99(const Stats<T> &other) const
{
  return get_cdf99(get_size() + other.get_size() - 2);
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_confidence_interval_95() const
{
  const size_t df = get_size() - 1;
  return get_standard_error() * get_cdf95(df);
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_confidence_interval_99() const
{
  const size_t df = get_size() - 1;
  return get_standard_error() * get_cdf99(df);
}

template<typename T, typename Tpod>
bool Stats<T, Tpod>::is_same_distribution_95(size_t npoints, double mean, double stddev) const
{
  return fabs(get_t_test(npoints, mean, stddev)) < get_cdf95(get_size() + npoints - 2);
}

template<typename T, typename Tpod>
bool Stats<T, Tpod>::is_same_distribution_95(const Stats<T> &other) const
{
  return fabs(get_t_test(other)) < get_cdf95(other);
}

template<typename T, typename Tpod>
bool Stats<T, Tpod>::is_same_distribution_99(size_t npoints, double mean, double stddev) const
{
  return fabs(get_t_test(npoints, mean, stddev)) < get_cdf99(get_size() + npoints - 2);
}

template<typename T, typename Tpod>
bool Stats<T, Tpod>::is_same_distribution_99(const Stats<T> &other) const
{
  return fabs(get_t_test(other)) < get_cdf99(other);
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_standard_deviation() const
{
  if (values.size() <= 1)
    return 0.0;
  if (!is_cached(bit_standard_deviation))
  {
    Tpod m = get_mean(), t = 0;
    for (const T &v: values)
      t += ((T)v - m) * ((T)v - m);
    standard_deviation = sqrt(t / ((double)values.size() - 1));
    set_cached(bit_standard_deviation);
  }
  return standard_deviation;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_standard_error() const
{
  if (!is_cached(bit_standard_error))
  {
    standard_error = get_standard_deviation() / sqrt(get_size());
    set_cached(bit_standard_error);
  }
  return standard_error;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_variance() const
{
  if (!is_cached(bit_variance))
  {
    double stddev = get_standard_deviation();
    variance = stddev * stddev;
    set_cached(bit_variance);
  }
  return variance;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_kurtosis() const
{
  if (values.empty())
    return 0.0;
  if (!is_cached(bit_kurtosis))
  {
    double m = get_mean();
    double n = 0, d = 0;
    for (const T &v: values)
    {
      T p2 = (v - m) * (v - m);
      T p4 = p2 * p2;
      n += p4;
      d += p2;
    }
    n /= values.size();
    d /= values.size();
    d *= d;
    kurtosis = n / d;
    set_cached(bit_kurtosis);
  }
  return kurtosis;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_non_parametric_skew() const
{
  return (get_mean() - get_median()) / get_standard_deviation();
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_t_test(T t) const
{
  const double n = get_mean() - t;
  const double d = get_standard_deviation() / sqrt(get_size());
  return n / d;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_t_test(size_t npoints, double mean, double stddev) const
{
  const double n = get_mean() - mean;
  const double d = sqrt(get_variance() / get_size() + square(stddev) / npoints);
  return n / d;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_t_test(const Stats<T> &other) const
{
  const double n = get_mean() - other.get_mean();
  const double d = sqrt(get_variance() / get_size() + other.get_variance() / other.get_size());
  return n / d;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_z_test(const Stats<T> &other) const
{
  const double m0 = get_mean();
  const double m1 = other.get_mean();
  const double sd0 = get_standard_deviation();
  const double sd1 = other.get_standard_deviation();
  const size_t s0 = get_size();
  const size_t s1 = other.get_size();

  const double n = m0 - m1;
  const double d = sqrt(square(sd0 / sqrt(s0)) + square(sd1 / sqrt(s1)));

  return n / d;
}

template<typename T, typename Tpod>
double Stats<T, Tpod>::get_test(const Stats<T> &other) const
{
  if (get_size() >= 30 && other.get_size() >= 30)
    return get_z_test(other);
  else
    return get_t_test(other);
}

template<typename T, typename Tpod>
std::vector<Tpod> Stats<T, Tpod>::get_quantiles(unsigned int quantiles) const
{
  std::vector<Tpod> sorted;
  sorted.reserve(values.size());
  for (const T &v: values)
    sorted.push_back(v);
  std::sort(sorted.begin(), sorted.end());
  std::vector<Tpod> q(quantiles + 1, 0);
  for (unsigned int i = 1; i <= quantiles; ++i)
  {
    unsigned idx = (unsigned)ceil(values.size() * i / (double)quantiles);
    q[i] = sorted[idx - 1];
  }
  if (!is_cached(bit_min))
  {
    min = sorted.front();
    set_cached(bit_min);
  }
  q[0] = min;
  if (!is_cached(bit_max))
  {
    max = sorted.back();
    set_cached(bit_max);
  }
  return q;
}

template<typename T, typename Tpod>
std::vector<size_t> Stats<T, Tpod>::get_bins(unsigned int bins) const
{
  std::vector<size_t> b(bins, 0);
  const double scale = 1.0 / (get_max() - get_min());
  const T base = get_min();
  for (const T &v: values)
  {
    unsigned int idx = (v - base) * scale;
    ++b[idx];
  }
  return b;
}
