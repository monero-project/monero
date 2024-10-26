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

TimingsDatabase::TimingsDatabase(const std::string &filename, const bool load_previous /*=false*/):
  filename(filename)
{
  if (load_previous)
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
    instances.emplace_back(name, i);
  }
  fclose(f);
  return true;
}

bool TimingsDatabase::save(const bool save_current_time /*=true*/)
{
  if (filename.empty() || instances.empty())
    return true;

  FILE *f = fopen(filename.c_str(), "a");  // append
  if (!f)
  {
    MERROR("Failed to write to file " << filename << ": " << strerror(errno));
    return false;
  }

  if (save_current_time)
  {
    // save current time in readable format (UTC)
    std::time_t sys_time{std::time(nullptr)};
    std::tm *utc_time = std::gmtime(&sys_time);    //GMT is equivalent to UTC

    // format: year-month-day : hour:minute:second
    std::string current_time{};
    if (utc_time && sys_time != (std::time_t)(-1))
    {
      char timeString[22];  //length = std::size("yyyy-mm-dd : hh:mm:ss")  (constexpr std::size is C++17)
      std::strftime(timeString, 22, "%F : %T", utc_time);
      current_time += timeString;
    }
    else
    {
        current_time += "TIME_ERROR_";
    }
    fputc('\n', f);  // add an extra line before each 'print time'
    fprintf(f, "%s", current_time.c_str());
    fputc('\n', f);
  }

  for (const auto &i: instances)
  {
    fprintf(f, "%s,", i.first.c_str());

    if (i.second.npoints > 0)
    {
      fprintf(f, "%lu,", (unsigned long)i.second.t);
      fprintf(f, "%zu,", i.second.npoints);
      fprintf(f, "%f,", i.second.min);
      fprintf(f, "%f,", i.second.max);
      fprintf(f, "%f,", i.second.mean);
      fprintf(f, "%f,", i.second.median);
      fprintf(f, "%f,", i.second.stddev);
      fprintf(f, "%f,", i.second.npskew);
      for (uint64_t v: i.second.deciles)
        fprintf(f, "%lu,", (unsigned long)v);

      // note: only add a new line if there are points; assume that 'no points' means i.first is a message meant to be
      //       prepended to the next save operation
      fputc('\n', f);
    }
  }
  fclose(f);

  // after saving, clear so next save does not append the same stuff over again
  instances.clear();

  return true;
}

const TimingsDatabase::instance* TimingsDatabase::get_most_recent(const char *name) const
{
  time_t latest_time = 0;
  const TimingsDatabase::instance *instance_ptr = nullptr;

  for (const auto &i: instances)
  {
    if (i.first != name)
      continue;
    if (i.second.t < latest_time)
      continue;

    latest_time  = i.second.t;
    instance_ptr = &i.second;
  }
  return instance_ptr;
}

void TimingsDatabase::add(const char *name, const instance &i)
{
  instances.emplace_back(name, i);
}
