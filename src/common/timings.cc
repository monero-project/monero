#include <string.h>
#include <errno.h>
#include <time.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include "misc_log_ex.h"
#include "timings.h"

#define N_EXPECTED_FIELDS (8+11)

TimingsDatabase::TimingsDatabase()
{
}

TimingsDatabase::TimingsDatabase(const std::string &filename):
  filename(filename)
{
  load();
}

TimingsDatabase::~TimingsDatabase()
{
  save();
}

bool TimingsDatabase::load()
{
  instances.clear();

  if (filename.empty())
    return true;

  FILE *f = fopen(filename.c_str(), "r");
  if (!f)
  {
    MDEBUG("Failed to load timings file " << filename << ": " << strerror(errno));
    return false;
  }
  while (1)
  {
    char s[4096];
    if (!fgets(s, sizeof(s), f))
      break;
    char *tab = strchr(s, '\t');
    if (!tab)
    {
      MWARNING("Bad format: no tab found");
      continue;
    }
    const std::string name = std::string(s, tab - s);
    std::vector<std::string> fields;
    char *ptr = tab + 1;
    boost::split(fields, ptr, boost::is_any_of(" "));
    if (fields.size() != N_EXPECTED_FIELDS)
    {
      MERROR("Bad format: wrong number of fields: got " << fields.size() << " expected " << N_EXPECTED_FIELDS);
      continue;
    }

    instance i;

    unsigned int idx = 0;
    i.t = atoi(fields[idx++].c_str());
    i.npoints = atoi(fields[idx++].c_str());
    i.min = atof(fields[idx++].c_str());
    i.max = atof(fields[idx++].c_str());
    i.mean = atof(fields[idx++].c_str());
    i.median = atof(fields[idx++].c_str());
    i.stddev = atof(fields[idx++].c_str());
    i.npskew = atof(fields[idx++].c_str());
    i.deciles.reserve(11);
    for (int n = 0; n < 11; ++n)
    {
      i.deciles.push_back(atoi(fields[idx++].c_str()));
    }
    instances.insert(std::make_pair(name, i));
  }
  fclose(f);
  return true;
}

bool TimingsDatabase::save()
{
  if (filename.empty())
    return true;

  FILE *f = fopen(filename.c_str(), "w");
  if (!f)
  {
    MERROR("Failed to write to file " << filename << ": " << strerror(errno));
    return false;
  }
  for (const auto &i: instances)
  {
    fprintf(f, "%s", i.first.c_str());
    fprintf(f, "\t%lu", (unsigned long)i.second.t);
    fprintf(f, " %zu", i.second.npoints);
    fprintf(f, " %f", i.second.min);
    fprintf(f, " %f", i.second.max);
    fprintf(f, " %f", i.second.mean);
    fprintf(f, " %f", i.second.median);
    fprintf(f, " %f", i.second.stddev);
    fprintf(f, " %f", i.second.npskew);
    for (uint64_t v: i.second.deciles)
      fprintf(f, " %lu", (unsigned long)v);
    fputc('\n', f);
  }
  fclose(f);
  return true;
}

std::vector<TimingsDatabase::instance> TimingsDatabase::get(const char *name) const
{
  std::vector<instance> ret;
  auto range = instances.equal_range(name);
  for (auto i = range.first; i != range.second; ++i)
    ret.push_back(i->second);
  std::sort(ret.begin(), ret.end(), [](const instance &e0, const instance &e1){ return e0.t < e1.t; });
  return ret;
}

void TimingsDatabase::add(const char *name, const instance &i)
{
  instances.insert(std::make_pair(name, i));
}
