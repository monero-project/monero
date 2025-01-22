#pragma once

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

class TimingsDatabase
{
public:
  struct instance
  {
    time_t t;
    size_t npoints;
    double min, max, mean, median, stddev, npskew;
    std::vector<uint64_t> deciles;
  };

public:
  TimingsDatabase();
  TimingsDatabase(const std::string &filename, const bool load_previous = false);
  ~TimingsDatabase();

  const instance* get_most_recent(const char *name) const;
  void add(const char *name, const instance &data);
  bool save(const bool print_current_time = true);

private:
  bool load();

private:
  std::string filename;
  std::vector<std::pair<std::string, instance>> instances;
};
