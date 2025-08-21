// Copyright (c) 2014-2024, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifdef __linux__

// ---------- Project headers ----------
#include "landlock.h"
#include "misc_log_ex.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon.landlock"

// ---------- Standard library ----------
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

// ---------- Linux / POSIX ----------
#include <linux/landlock.h>  // kernel UAPI for structs and constants
#include <errno.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/syscall.h>     // syscall(2) + __NR_landlock_* numbers
#include <unistd.h>

// ---------- Third-party ----------
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

// OpenSSL runtime discovery (OpenSSL 3 required at link time)
#include <openssl/crypto.h>   // OPENSSL_info, OPENSSL_INFO_*
#include <openssl/x509.h>     // X509_get_default_cert_*

using boost::filesystem::path;

namespace landlock_integration
{

// ---------- Compile-time requirements & compatibility shims -------------------

#ifndef __NR_landlock_create_ruleset
#error "Landlock syscalls are required: __NR_landlock_create_ruleset is missing."
#endif
#ifndef __NR_landlock_add_rule
#error "Landlock syscalls are required: __NR_landlock_add_rule is missing."
#endif
#ifndef __NR_landlock_restrict_self
#error "Landlock syscalls are required: __NR_landlock_restrict_self is missing."
#endif

// Newer filesystem access-rights/flags may not exist on older headers. Make them 0 so
// the code still compiles and simply won't use the newer features.
#ifndef LANDLOCK_ACCESS_FS_TRUNCATE
#define LANDLOCK_ACCESS_FS_TRUNCATE 0ULL
#endif
#ifndef LANDLOCK_ACCESS_FS_REFER
#define LANDLOCK_ACCESS_FS_REFER 0ULL
#endif
#ifndef LANDLOCK_ACCESS_FS_IOCTL_DEV
#define LANDLOCK_ACCESS_FS_IOCTL_DEV 0ULL
#endif

// Some older headers predate scoped fields; we only touch
// those fields when we can infer header support from the presence of their flags.
#ifndef LANDLOCK_SCOPE_ABSTRACT_UNIX_SOCKET
#define LANDLOCK_SCOPE_ABSTRACT_UNIX_SOCKET 0ULL
#endif
#ifndef LANDLOCK_SCOPE_SIGNAL
#define LANDLOCK_SCOPE_SIGNAL 0ULL
#endif

#if (LANDLOCK_SCOPE_ABSTRACT_UNIX_SOCKET || LANDLOCK_SCOPE_SIGNAL)
#define LL_HAS_SCOPED_FIELD 1
#else
#define LL_HAS_SCOPED_FIELD 0
#endif

// ---------- File-scope constants ---------------------------------------------

// Read-only access masks reused across functions.
constexpr __u64 kRoFile = LANDLOCK_ACCESS_FS_READ_FILE;
constexpr __u64 kRoDir  = LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR;

// ---------- Minimal syscall wrappers -----------------------------------------

static inline int ll_create_ruleset(const struct landlock_ruleset_attr* attr, size_t size, __u32 flags)
{
  return static_cast<int>(syscall(__NR_landlock_create_ruleset, attr, size, flags));
}

static inline int ll_add_rule(int ruleset_fd, enum landlock_rule_type type, const void* rule_attr, __u32 flags)
{
  return static_cast<int>(syscall(__NR_landlock_add_rule, ruleset_fd, type, rule_attr, flags));
}

static inline int ll_restrict_self(int ruleset_fd, __u32 flags)
{
  return static_cast<int>(syscall(__NR_landlock_restrict_self, ruleset_fd, flags));
}

// ---------- Error handling helpers -------------------------------------------

[[noreturn]] static void fatal_exit(const char* what, const char* detail = nullptr)
{
  if (detail && *detail)
    MERROR("Landlock: " << what << ": " << detail);
  else
    MERROR("Landlock: " << what);
  std::exit(EXIT_FAILURE);
}

// ---------- Helpers -----------------------------------------------------------

static void ensure_single_threaded_or_die()
{
  const path task_dir("/proc/self/task");

  boost::system::error_code ec;
  if (!boost::filesystem::exists(task_dir, ec) || ec)
    fatal_exit("cannot access /proc/self/task", ec ? ec.message().c_str() : "not found");

  std::size_t count = 0;
  boost::filesystem::directory_iterator it(task_dir, ec);
  boost::filesystem::directory_iterator it_end;
  for (; !ec && it != it_end; ++it)
  {
    const std::string name = it->path().filename().string();
    if (!name.empty() && name[0] != '.')
      ++count;
  }
  if (ec)
    fatal_exit("error iterating /proc/self/task", ec.message().c_str());

  if (count > 1)
    fatal_exit("Landlock must be enabled in a single-threaded state", "more than one thread detected");
}

// Returns true on success; on failure returns false and preserves errno.
static bool add_path_rule_allowlist(int ruleset_fd, const path& p, __u64 allowed_subset)
{
  int path_fd = ::open(p.string().c_str(), O_PATH | O_CLOEXEC);
  if (path_fd < 0)
  {
    return false; // errno set by open
  }

  struct landlock_path_beneath_attr path_beneath = {};
  path_beneath.allowed_access = allowed_subset;
  path_beneath.parent_fd = path_fd;

  int rc = ll_add_rule(ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, &path_beneath, 0);
  int saved = errno;
  ::close(path_fd);
  if (rc < 0)
  {
    errno = saved;
    return false;
  }
  MGINFO("Landlock: adding path " << p.string());
  return true;
}

// Optional-path variant: ignore missing files.
static bool add_path_rule_allowlist_if_exists(int ruleset_fd, const path& p, __u64 allowed_subset)
{
  if (::access(p.string().c_str(), F_OK) != 0)
  {
    // Missing file is fine for optional paths (e.g., distro-specific DNS config).
    return true;
  }
  return add_path_rule_allowlist(ruleset_fd, p, allowed_subset);
}

// Require-success wrapper: exits on failure, but still treats missing files as OK.
static void must_add_rule_if_exists(int ruleset_fd, const path& p, __u64 allowed_subset)
{
  if (!add_path_rule_allowlist_if_exists(ruleset_fd, p, allowed_subset))
    fatal_exit("failed to add allowlist rule", (p.string() + ": " + std::strerror(errno)).c_str());
}

// Split a list of paths and allow-list them.
static void add_list_dirs(int ruleset_fd, const std::string& list, const std::string& sep, __u64 mask)
{
  if (list.empty()) return;
  std::size_t start = 0;
  while (start <= list.size())
  {
    std::size_t pos = list.find(sep, start);
    std::string item = (pos == std::string::npos) ? list.substr(start) : list.substr(start, pos - start);
    if (!item.empty())
      must_add_rule_if_exists(ruleset_fd, path(item), mask);
    if (pos == std::string::npos) break;
    start = pos + sep.size();
  }
}

// ---------- Small regex helpers ----------------------------------------------

// A minimal “regex replace with callback” for Boost.Regex (since it doesn’t accept lambdas directly).
template<class Callback>
static std::string regex_replace_all_cb(const std::string& input, const boost::regex& re, Callback cb)
{
  std::string out;
  out.reserve(input.size());
  auto it = input.begin();
  auto end_it = input.end();

  boost::match_results<std::string::const_iterator> m;
  while (boost::regex_search(it, end_it, m, re))
  {
    out.append(it, m[0].first);        // text before match
    out += cb(m);                      // replacement for the match
    it = m[0].second;                  // advance
  }
  out.append(it, end_it);              // tail
  return out;
}

// --- expand ${ENV::VAR} / $ENV::VAR in include paths (regex-based) ---
static std::string expand_env_vars(const std::string& input)
{
  static const boost::regex braced(R"(\$\{ENV::([A-Za-z0-9_]+)\})");
  static const boost::regex bare(R"(\$ENV::([A-Za-z0-9_]+))");

  auto env_cb = [](const boost::smatch& m)->std::string {
    const char* v = std::getenv(m[1].str().c_str());
    return v ? std::string(v) : std::string();
  };

  std::string s = regex_replace_all_cb(input, braced, env_cb);
  s = regex_replace_all_cb(s, bare, env_cb);
  return s;
}

static std::string trim_unquote(const std::string& input)
{
  // Trim leading/trailing whitespace in a single pass (left or right).
  static const boost::regex ws(R"(^\s+|\s+$)");
  std::string s = boost::regex_replace(input, ws, "");

  // Remove surrounding single or double quotes if present.
  static const boost::regex quoted(R"(^(['"])(.*)\1$)");
  boost::smatch m;
  if (boost::regex_match(s, m, quoted))
    return m[2].str();
  return s;
}

// Parse an OpenSSL config file for include directives and allow-list them.
// OpenSSL semantics (config(5)):
//  - .pragma includedir:DIR   (optional '=' before key)
//  - .include / include PATH  (optional '=' before value)
//    * Relative PATH: prefix $OPENSSL_CONF_INCLUDE if set, else current .pragma includedir, else CWD.
//    * Directory PATH: include all *.cnf / *.conf in that directory (sorted), not recursive.
//    * While parsing a file reached from a directory include, ignore any directory includes.
static void add_includes_from_openssl_conf(int ruleset_fd, const path& conf_path, std::unordered_set<std::string>& seen, std::string includedir = std::string(), bool ignore_dir_includes = false)
{
  boost::system::error_code ec;
  if (!boost::filesystem::exists(conf_path, ec) || ec) return;

  // De-duplicate using canonical path where possible.
  path canon = boost::filesystem::canonical(conf_path, ec);
  const std::string key = (ec ? conf_path.string() : canon.string());
  if (!seen.insert(key).second) return;

  std::ifstream in(conf_path.string(), std::ios::in | std::ios::binary);
  if (!in) return;

  // Slurp whole file and run a single multiline regex over it.
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

  // One pattern that matches either:
  //   - .pragma includedir: VALUE
  //   - .include / include  VALUE
  // VALUE is (quoted-string) or (any run of non-# chars), so trailing # comments are ignored.
  static const boost::regex directive_re(
      // (?m) -> multiline anchors; keep the rest as before
      R"((?m)^\s*(?:)"
      R"((?:\.pragma)\s*(?:=\s*)?includedir\s*:\s*((?:"[^"]*"|'[^']*'|[^#\r\n])+))"
      R"(|(?:\.include|include)\s*(?:=\s*)?((?:"[^"]*"|'[^']*'|[^#\r\n])+))"
      R"()\s*(?:#.*)?$)",
      boost::regex_constants::perl | boost::regex_constants::icase);

  // Case-insensitive match for *.cnf / *.conf
  static const boost::regex conf_file_re(R"((?i)\.(cnf|conf)$)");

  auto add_file_and_parent = [&](const path& f) {
    must_add_rule_if_exists(ruleset_fd, f, kRoFile);
    if (f.has_parent_path())
      must_add_rule_if_exists(ruleset_fd, f.parent_path(), kRoDir);
  };

  auto resolve_include_path = [&](std::string token)->path {
    token = trim_unquote(expand_env_vars(token));
    if (token.empty()) return {};
    path p(token);
    if (p.is_relative()) {
      if (const char* pfx = std::getenv("OPENSSL_CONF_INCLUDE")) {
        if (pfx && *pfx) p = path(pfx) / p;
      } else if (!includedir.empty()) {
        p = path(includedir) / p;
      } else {
        // Relative to CWD (leave as-is).
      }
    }
    return p;
  };

  for (boost::sregex_iterator it(content.begin(), content.end(), directive_re), end_it; it != end_it; ++it)
  {
    const boost::smatch& m = *it;

    // Group 1 => .pragma includedir ; Group 2 => include
    if (m[1].matched)
    {
      includedir = trim_unquote(expand_env_vars(m[1].str()));
      continue;
    }

    // include / .include
    std::string token = m[2].str();
    path inc = resolve_include_path(token);
    if (inc.empty()) continue;

    boost::system::error_code iec;
    if (!boost::filesystem::exists(inc, iec) || iec) continue;

    if (boost::filesystem::is_directory(inc, iec))
    {
      if (ignore_dir_includes) {
        // When parsing a file found by directory scan, ignore directory includes.
        continue;
      }

      // Allow the directory, then process its *.cnf / *.conf files (non-recursive).
      must_add_rule_if_exists(ruleset_fd, inc, kRoDir);

      std::vector<path> files;
      boost::filesystem::directory_iterator dit(inc, iec), dend;
      for (; !iec && dit != dend; ++dit)
      {
        const auto& p = dit->path();
        if (boost::filesystem::is_regular_file(p))
        {
          const std::string leaf = p.filename().string();
          if (boost::regex_search(leaf, conf_file_re))
            files.push_back(p);
        }
      }
      std::sort(files.begin(), files.end()); // deterministic order

      for (const auto& f : files)
      {
        add_file_and_parent(f);
        add_includes_from_openssl_conf(ruleset_fd, f, seen, includedir, true);
      }
    }
    else
    {
      // Regular file include
      add_file_and_parent(inc);
      add_includes_from_openssl_conf(ruleset_fd, inc, seen, includedir, false);
    }
  }
}

// ---------- OpenSSL runtime allowlist rules ----------------------------------

static void add_openssl_allowlist_rules(int ruleset_fd)
{
  // Linux-only: path list separator is ':'.
  const std::string list_sep = ":";

  // Track seen config files for include parsing.
  std::unordered_set<std::string> seen;

  // OpenSSL configuration (file)
  if (const char* conf_env = std::getenv("OPENSSL_CONF"))
  {
    path p(conf_env);
    must_add_rule_if_exists(ruleset_fd, p, kRoFile);
    if (p.has_parent_path())
      must_add_rule_if_exists(ruleset_fd, p.parent_path(), kRoDir);
    add_includes_from_openssl_conf(ruleset_fd, p, seen);
  }

  if (const char* confdir = OPENSSL_info(OPENSSL_INFO_CONFIG_DIR))
  {
    if (confdir && *confdir)
    {
      path d(confdir);
      must_add_rule_if_exists(ruleset_fd, d, kRoDir);

      const path main_cnf = d / "openssl.cnf";
      must_add_rule_if_exists(ruleset_fd, main_cnf, kRoFile);
      add_includes_from_openssl_conf(ruleset_fd, main_cnf, seen);

      const path fips_cnf = d / "fipsmodule.cnf";
      must_add_rule_if_exists(ruleset_fd, fips_cnf, kRoFile);
      add_includes_from_openssl_conf(ruleset_fd, fips_cnf, seen);

      const path providers_cnf = d / "providers.cnf";
      must_add_rule_if_exists(ruleset_fd, providers_cnf, kRoFile);
      add_includes_from_openssl_conf(ruleset_fd, providers_cnf, seen);
    }
  }

  // OpenSSL provider modules directory
  if (const char* mods_env = std::getenv("OPENSSL_MODULES"))
  {
    add_list_dirs(ruleset_fd, mods_env, list_sep, kRoDir);
  }
  else
  {
    if (const char* modsdir = OPENSSL_info(OPENSSL_INFO_MODULES_DIR))
      if (modsdir && *modsdir)
        must_add_rule_if_exists(ruleset_fd, path(modsdir), kRoDir);
  }

  // CA bundle locations (OpenSSL defaults + env overrides)
  if (const char* cafile_env = std::getenv("SSL_CERT_FILE"))
    if (cafile_env && *cafile_env)
      must_add_rule_if_exists(ruleset_fd, path(cafile_env), kRoFile);

  if (const char* cadir_env = std::getenv("SSL_CERT_DIR"))
    if (cadir_env && *cadir_env)
      add_list_dirs(ruleset_fd, cadir_env, list_sep, kRoDir);

  if (const char* cafile = X509_get_default_cert_file())
    if (cafile && *cafile)
      must_add_rule_if_exists(ruleset_fd, path(cafile), kRoFile);

  if (const char* cadirs = X509_get_default_cert_dir())
    if (cadirs && *cadirs)
      add_list_dirs(ruleset_fd, cadirs, list_sep, kRoDir);
}

// ---------- System filesystem rules (read-only allowlist) ---------------------

static void add_system_allowlist_rules(int ruleset_fd)
{
  // === /proc interfaces ===
  must_add_rule_if_exists(ruleset_fd, path("/proc/self/task"), kRoDir);
  must_add_rule_if_exists(ruleset_fd, path("/proc/sys/net/ipv4/ip_local_port_range"), kRoFile);

  // === DNS resolution files ===
  must_add_rule_if_exists(ruleset_fd, path("/etc/resolv.conf"), kRoFile);
  must_add_rule_if_exists(ruleset_fd, path("/run/systemd/resolve/resolv.conf"), kRoFile);
  must_add_rule_if_exists(ruleset_fd, path("/run/systemd/resolve/stub-resolv.conf"), kRoFile);
  must_add_rule_if_exists(ruleset_fd, path("/run/resolvconf/resolv.conf"), kRoFile);
  must_add_rule_if_exists(ruleset_fd, path("/etc/hosts"), kRoFile);
  must_add_rule_if_exists(ruleset_fd, path("/etc/nsswitch.conf"), kRoFile);
  must_add_rule_if_exists(ruleset_fd, path("/etc/host.conf"), kRoFile);
  must_add_rule_if_exists(ruleset_fd, path("/etc/gai.conf"), kRoFile);

  // === OpenSSL (runtime discovery) ===
  add_openssl_allowlist_rules(ruleset_fd);
}

// ---------- Public API --------------------------------------------------------

void enable_landlock_sandbox(const path& data_dir, const path& log_file_path, bool rotate_logs)
{
  // Query the running kernel's Landlock ABI.
  int abi = ll_create_ruleset(nullptr, 0, LANDLOCK_CREATE_RULESET_VERSION);
  if (abi < 0)
    fatal_exit("Landlock not available (kernel/LSM/permissions)", std::strerror(errno));

  // Build a ruleset attr compatible with our headers and the running ABI.
  struct landlock_ruleset_attr ruleset_attr = {};
  __u64 handled_fs =
      LANDLOCK_ACCESS_FS_EXECUTE     |
      LANDLOCK_ACCESS_FS_WRITE_FILE  |
      LANDLOCK_ACCESS_FS_READ_FILE   |
      LANDLOCK_ACCESS_FS_READ_DIR    |
      LANDLOCK_ACCESS_FS_REMOVE_DIR  |
      LANDLOCK_ACCESS_FS_REMOVE_FILE |
      LANDLOCK_ACCESS_FS_MAKE_CHAR   |
      LANDLOCK_ACCESS_FS_MAKE_DIR    |
      LANDLOCK_ACCESS_FS_MAKE_REG    |
      LANDLOCK_ACCESS_FS_MAKE_SOCK   |
      LANDLOCK_ACCESS_FS_MAKE_FIFO   |
      LANDLOCK_ACCESS_FS_MAKE_BLOCK  |
      LANDLOCK_ACCESS_FS_MAKE_SYM;
  if (abi >= 2) handled_fs |= LANDLOCK_ACCESS_FS_REFER;
  if (abi >= 3) handled_fs |= LANDLOCK_ACCESS_FS_TRUNCATE;
  if (abi >= 5) handled_fs |= LANDLOCK_ACCESS_FS_IOCTL_DEV;

  ruleset_attr.handled_access_fs = handled_fs;

#if LL_HAS_SCOPED_FIELD
  if (abi >= 6)
  {
    ruleset_attr.scoped =
        LANDLOCK_SCOPE_ABSTRACT_UNIX_SOCKET |
        LANDLOCK_SCOPE_SIGNAL;
  }
#endif

  // Create the ruleset FD.
  int ruleset_fd = ll_create_ruleset(&ruleset_attr, sizeof(ruleset_attr), 0);
  if (ruleset_fd < 0)
    fatal_exit("create_ruleset failed", std::strerror(errno));

  // Allow data_dir read/write + basic file ops.
  __u64 data_allow =
      LANDLOCK_ACCESS_FS_READ_FILE   |
      LANDLOCK_ACCESS_FS_WRITE_FILE  |
      LANDLOCK_ACCESS_FS_READ_DIR    |
      LANDLOCK_ACCESS_FS_MAKE_DIR    |
      LANDLOCK_ACCESS_FS_MAKE_REG    |
      LANDLOCK_ACCESS_FS_REMOVE_FILE;
  if (abi >= 3) data_allow |= LANDLOCK_ACCESS_FS_TRUNCATE;
  if (abi >= 2) data_allow |= LANDLOCK_ACCESS_FS_REFER;

  if (!add_path_rule_allowlist(ruleset_fd, data_dir, data_allow))
    fatal_exit("failed to add data_dir allowlist rule", (data_dir.string() + ": " + std::strerror(errno)).c_str());

  // Logging policy
  if (!rotate_logs)
  {
    // Allow writing to the log file
    int fd = ::open(log_file_path.string().c_str(), O_WRONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0644);
    if (fd < 0)
      fatal_exit("failed to create/open log file", (log_file_path.string() + ": " + std::strerror(errno)).c_str());
    ::close(fd);

    __u64 log_allow = LANDLOCK_ACCESS_FS_WRITE_FILE;
    if (abi >= 3) log_allow |= LANDLOCK_ACCESS_FS_TRUNCATE;
    if (!add_path_rule_allowlist(ruleset_fd, log_file_path, log_allow))
      fatal_exit("failed to add log file allowlist rule", (log_file_path.string() + ": " + std::strerror(errno)).c_str());
  }
  else
  {
    // Allow file ops necessary for logs rotation
    const path log_dir = log_file_path.has_parent_path() ? log_file_path.parent_path() : path(".");
    __u64 logdir_allow =
        LANDLOCK_ACCESS_FS_READ_DIR    |
        LANDLOCK_ACCESS_FS_READ_FILE   |
        LANDLOCK_ACCESS_FS_WRITE_FILE  |
        LANDLOCK_ACCESS_FS_MAKE_REG    |
        LANDLOCK_ACCESS_FS_REMOVE_FILE;
    if (abi >= 2) logdir_allow |= LANDLOCK_ACCESS_FS_REFER;
    if (abi >= 3) logdir_allow |= LANDLOCK_ACCESS_FS_TRUNCATE;

    if (!add_path_rule_allowlist(ruleset_fd, log_dir, logdir_allow))
      fatal_exit("failed to add log directory allowlist rule", (log_dir.string() + ": " + std::strerror(errno)).c_str());
  }

  // System read-only allowlist rules.
  add_system_allowlist_rules(ruleset_fd);

  // Enforce on current thread and all future children.
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0)
    fatal_exit("PR_SET_NO_NEW_PRIVS failed", std::strerror(errno));

  if (ll_restrict_self(ruleset_fd, 0) != 0)
    fatal_exit("restrict_self failed", std::strerror(errno));

  ensure_single_threaded_or_die();

  ::close(ruleset_fd);
}

} // namespace landlock_integration

#endif // __linux__