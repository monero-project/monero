#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

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
  TimingsDatabase(const std::string &filename);
  ~TimingsDatabase();

  std::vector<instance> get(const char *name) const;
  void add(const char *name, const instance &data);

private:
  bool load();
  bool save();

private:
  std::string filename;
  std::multimap<std::string, instance> instances;
};
