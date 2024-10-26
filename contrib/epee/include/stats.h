#pragma once

#include <vector>

template<typename T, typename Tpod = T>
class Stats
{
public:
  Stats(const std::vector<T> &v): values(v), cached(0) {}
  ~Stats() {}

  size_t get_size() const;
  Tpod get_min() const;
  Tpod get_max() const;
  Tpod get_median() const;
  double get_mean() const;
  double get_confidence_interval_95() const;
  double get_confidence_interval_99() const;
  double get_standard_deviation() const;
  double get_standard_error() const;
  double get_variance() const;
  double get_kurtosis() const;
  double get_non_parametric_skew() const;
  double get_t_test(T t) const;
  double get_t_test(size_t npoints, double mean, double stddev) const;
  double get_t_test(const Stats<T> &other) const;
  double get_z_test(const Stats<T> &other) const;
  double get_test(const Stats<T> &other) const;
  std::vector<Tpod> get_quantiles(unsigned int quantiles) const;
  std::vector<size_t> get_bins(unsigned int bins) const;
  bool is_same_distribution_95(size_t npoints, double mean, double stddev) const;
  bool is_same_distribution_95(const Stats<T> &other) const;
  bool is_same_distribution_99(size_t npoints, double mean, double stddev) const;
  bool is_same_distribution_99(const Stats<T> &other) const;

  double get_cdf95(size_t df) const;
  double get_cdf95(const Stats<T> &other) const;
  double get_cdf99(size_t df) const;
  double get_cdf99(const Stats<T> &other) const;

private:
  inline bool is_cached(int bit) const;
  inline void set_cached(int bit) const;

  const std::vector<T> &values;

  mutable uint64_t cached;
  mutable Tpod min;
  mutable Tpod max;
  mutable Tpod median;
  mutable double mean;
  mutable double standard_deviation;
  mutable double standard_error;
  mutable double variance;
  mutable double kurtosis;
};

#include "stats.inl"
