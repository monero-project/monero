#pragma once

#include "tags.h"
#include <cstring>
#include <list>
#include <vector>
#include <string>

namespace portable_scheme {

namespace scheme_space {

using namespace tags_space;

template<typename T, typename TT = void>
struct scheme;

template<typename T>
struct pod_tag;

template<typename T>
struct scheme<pod_tag<T>> {
  using tags = span_tag<sizeof(T)>;
  struct read_t {
  private:
    const T &t;
  public:
    read_t(const T &t): t(t) {}
    std::size_t size() const { return sizeof(t); }
    void get(std::uint8_t *out) const {
        static_assert(std::is_pod<T>::value, "");
        static_assert(sizeof(T) < 2 or not std::is_integral<T>::value, "");
        static_assert(sizeof(T) < 2 or not std::is_floating_point<T>::value, "");
        *reinterpret_cast<T *>(out) = t;
    }
  };
  struct write_t {
  private:
    T &t;
  public:
    write_t(T &t): t(t) {}
    bool set(const void *data, std::size_t size) {
      if (size != sizeof(t))
        return false;
      static_assert(std::is_pod<T>::value, "");
      static_assert(sizeof(T) < 2 or not std::is_integral<T>::value, "");
      static_assert(sizeof(T) < 2 or not std::is_floating_point<T>::value, "");
      t = *reinterpret_cast<const T *>(data);
      return true;
    }
  };
};

template<
  typename T,
  bool force = false,
  typename TT = typename T::value_type
>
struct container_pod_tag;

template<bool force, typename T>
struct scheme<container_pod_tag<T, force, typename T::value_type>, typename std::enable_if<(
  std::is_same<T, std::vector<typename T::value_type, typename T::allocator_type>>::value or
  std::is_same<T, std::basic_string<typename T::value_type, std::char_traits<typename T::value_type>, typename T::allocator_type>>::value
), void>::type> {
  using TT = typename T::value_type;
  using tags = span_tag<>;
  struct read_t {
  private:
    const T &t;
  public:
    read_t(const T &t): t(t) {}
    std::size_t size() const { return t.size() * sizeof(TT); }
    void get(std::uint8_t *out) const {
        static_assert(std::is_pod<TT>::value, "");
        static_assert(force or sizeof(TT) < 2 or not std::is_integral<TT>::value, "");
        static_assert(force or sizeof(TT) < 2 or not std::is_floating_point<TT>::value, "");
        std::memcpy(out, reinterpret_cast<void const *>(t.data()), t.size() * sizeof(TT));
    }
  };
  struct write_t {
  private:
    T &t;
  public:
    write_t(T &t): t(t) {}
    bool set(const void *data, std::size_t size) {
      static_assert(std::is_pod<TT>::value, "");
      static_assert(force or sizeof(TT) < 2 or not std::is_integral<TT>::value, "");
      static_assert(force or sizeof(TT) < 2 or not std::is_floating_point<TT>::value, "");
      if (sizeof(TT) == 0 || size % sizeof(TT) != 0)
        return false;
      t.assign(
        reinterpret_cast<const TT *>(data),
        reinterpret_cast<const TT *>(data) + size / sizeof(TT)
      );
      return true;
    }
  };
};

template<bool force, typename T>
struct scheme<container_pod_tag<T, force, typename T::value_type>, typename std::enable_if<(
  std::is_same<T, std::list<typename T::value_type, typename T::allocator_type>>::value
), void>::type> {
  using TT = typename T::value_type;
  using tags = span_tag<>;
  struct read_t {
  private:
    const T &t;
  public:
    read_t(const T &t): t(t) {}
    std::size_t size() const { return t.size() * sizeof(TT); }
    void get(std::uint8_t *out) const {
      static_assert(std::is_pod<TT>::value, "");
      static_assert(sizeof(TT) < 2 or not std::is_integral<TT>::value, "");
      static_assert(sizeof(TT) < 2 or not std::is_floating_point<TT>::value, "");
      for (auto it = t.cbegin(); it != t.cend(); ++it, out += sizeof(TT)) {
        *reinterpret_cast<TT *>(out) = *it;
      }
    }
  };
  struct write_t {
  private:
    T &t;
  public:
    write_t(T &t): t(t) {}
    bool set(const void *data, std::size_t size) {
      static_assert(std::is_pod<TT>::value, "");
      static_assert(sizeof(TT) < 2 or not std::is_integral<TT>::value, "");
      static_assert(sizeof(TT) < 2 or not std::is_floating_point<TT>::value, "");
      if (sizeof(TT) == 0 || size % sizeof(TT) != 0)
        return false;
      t.assign(
        reinterpret_cast<const TT *>(data),
        reinterpret_cast<const TT *>(data) + size / sizeof(TT)
      );
      return true;
    }
  };
};

template<bool force, typename T, typename TT>
struct scheme<container_pod_tag<T, force, TT>, typename std::enable_if<(
  not std::is_same<TT, typename T::value_type>::value and (
    std::is_same<T, std::vector<typename T::value_type, typename T::allocator_type>>::value or
    std::is_same<T, std::list<typename T::value_type, typename T::allocator_type>>::value
  )
), void>::type> {
  using tags = span_tag<>;
  struct read_t {
  private:
    const T &t;
  public:
    read_t(const T &t): t(t) {}
    std::size_t size() const { return t.size() * sizeof(TT); }
    void get(std::uint8_t *out) const {
      static_assert(std::is_pod<TT>::value, "");
      static_assert(sizeof(TT) < 2 or not std::is_integral<TT>::value, "");
      static_assert(sizeof(TT) < 2 or not std::is_floating_point<TT>::value, "");
      for (auto it = t.cbegin(); it != t.cend(); ++it, out += sizeof(TT)) {
        *reinterpret_cast<TT *>(out) = *it;
      }
    }
  };
  struct write_t {
  private:
    T &t;
  public:
    write_t(T &t): t(t) {}
    bool set(const void *data, std::size_t size) {
      static_assert(std::is_pod<TT>::value, "");
      static_assert(sizeof(TT) < 2 or not std::is_integral<TT>::value, "");
      static_assert(sizeof(TT) < 2 or not std::is_floating_point<TT>::value, "");
      if (sizeof(TT) == 0 || size % sizeof(TT) != 0)
        return false;
      t.resize(size / sizeof(TT));
      const TT *ptr = reinterpret_cast<const TT *>(data);
      for (TT &e: t) {
        e = *(ptr++);
      }
      return true;
    }
  };
};

template<
  typename T,
  typename = scheme<typename T::value_type>,
  std::size_t MIN = 0,
  std::size_t MAX = std::size_t(-1),
  std::size_t THRESHOLD = 0,
  std::size_t COMPRESSION_RATIO = 1
>
struct container_tag;

template<typename T, typename S, std::size_t MIN, std::size_t MAX, std::size_t THRESHOLD, std::size_t COMPRESSION_RATIO>
struct scheme<container_tag<T, S, MIN, MAX, THRESHOLD, COMPRESSION_RATIO>> {
  using tags = list_tag<typename S::tags, MIN>;
  struct read_t {
  private:
    const T &t;
  public:
    read_t(const T &t): t(t) {}
    std::size_t size() const { return t.size(); }
    struct const_iterator_t {
    private:
      typename T::const_iterator it;
    public:
      const_iterator_t(const T &t): it(t.cbegin()) {}
      typename S::read_t next() {
        return *(it++);
      }
    };
    const_iterator_t cbegin() const { return t; }
  };
  struct write_t {
  private:
    T &t;
  public:
    write_t(T &t): t(t) {}
    template<std::size_t MIN_ITEM_SIZE>
    static constexpr bool force_item_size(std::size_t items) {
      static_assert(MAX != std::size_t(-1) or sizeof(typename T::value_type) <= COMPRESSION_RATIO * MIN_ITEM_SIZE, "");
      return items >= THRESHOLD;
    }
    bool resize(std::size_t items) {
      if (items > MAX)
        return false;
      t.resize(items);
      return true;
    }
    struct iterator_t {
    private:
      typename T::iterator it;
    public:
      iterator_t(T &t): it(t.begin()) {}
      typename S::write_t next() {
        return *(it++);
      }
    };
    iterator_t begin() { return t; }
  };
};

template<typename T, endian_t endian>
struct scheme<base_tag<T, endian>> {
  using tags = base_tag<T, endian>;
  using read_t = const T &;
  using write_t = T &;
};

template<typename T>
struct scheme<T, typename std::enable_if<(std::is_integral<T>::value or std::is_floating_point<T>::value), void>::type> {
  using tags = base_tag<T>;
  using read_t = const T &;
  using write_t = T &;
};

template<typename T>
constexpr auto is_lvalue_reference(T &&) -> typename std::enable_if<std::is_lvalue_reference<T>::value, void>::type;
#define AS_IS(...) __VA_ARGS__
#define BEGIN_READ_RAW(...) \
  struct read_t {\
  __VA_ARGS__\
  public:\
\
    template<typename ...> struct selector{};
#define BEGIN_READ(TYPE) \
  BEGIN_READ_RAW(\
  private:\
    const TYPE &t;\
  public:\
    read_t(const TYPE &t): t(t) {}\
  )
#define READ_FIELD_CUSTOM_OPT_NAME(NAME, MEMBER, SCHEME, ENABLER) \
    bool enabled(selector<KEY(NAME), typename field_key<typename SCHEME::tags>::type>) const {ENABLER}\
    typename SCHEME::read_t get(selector<KEY(NAME), typename field_key<typename SCHEME::tags>::type>) const {static_assert(std::is_same<void, decltype(is_lvalue_reference(MEMBER))>::value, ""); return MEMBER;}
#define READ_FIELD_CUSTOM_OPT(MEMBER, SCHEME, ENABLER)             READ_FIELD_CUSTOM_OPT_NAME(#MEMBER, t.MEMBER, AS_IS(SCHEME), AS_IS(ENABLER))
#define READ_FIELD_OPT_NAME(NAME, MEMBER, ENABLER)                 READ_FIELD_CUSTOM_OPT_NAME(NAME, MEMBER, scheme<typename std::decay<decltype(MEMBER)>::type>, AS_IS(ENABLER))
#define READ_FIELD_OPT(MEMBER, ENABLER)                            READ_FIELD_CUSTOM_OPT(MEMBER, scheme<typename std::decay<decltype(t.MEMBER)>::type>, AS_IS(ENABLER))
#define READ_FIELD_CUSTOM(MEMBER, SCHEME)                          READ_FIELD_CUSTOM_OPT(MEMBER, AS_IS(SCHEME), {return true;})
#define READ_FIELD_WITH_DEFAULT(MEMBER, DEFAULT)                   READ_FIELD_OPT(MEMBER, {return t.MEMBER != DEFAULT;})
#define READ_FIELD_NAME(NAME, MEMBER)                              READ_FIELD_OPT_NAME(NAME, MEMBER, {return true;})
#define READ_FIELD(MEMBER)                                         READ_FIELD_CUSTOM(MEMBER, scheme<typename std::decay<decltype(t.MEMBER)>::type>)
#define READ_FIELD_LIST_CUSTOM_OPT(MEMBER, SCHEME, ...)            READ_FIELD_CUSTOM_OPT(MEMBER, AS_IS(SCHEME), AS_IS({if (t.MEMBER.empty()) return false; __VA_ARGS__; return true;}))
#define READ_FIELD_LIST_CUSTOM_OPT_NAME(NAME, MEMBER, SCHEME, ...) READ_FIELD_CUSTOM_OPT_NAME(NAME, MEMBER, AS_IS(SCHEME), AS_IS({if (MEMBER.empty()) return false; __VA_ARGS__; return true;}))
#define READ_FIELD_LIST_OPT_NAME(NAME, MEMBER, ...)                READ_FIELD_LIST_CUSTOM_OPT_NAME(NAME, MEMBER, scheme<container_tag<typename std::decay<decltype(MEMBER)>::type>>, AS_IS(__VA_ARGS__))
#define READ_FIELD_LIST_OPT(MEMBER, ...)                           READ_FIELD_LIST_CUSTOM_OPT(MEMBER, scheme<container_tag<typename std::decay<decltype(t.MEMBER)>::type>>, AS_IS(__VA_ARGS__))
#define READ_FIELD_LIST_CUSTOM(MEMBER, ...)                        READ_FIELD_LIST_CUSTOM_OPT(MEMBER, AS_IS(__VA_ARGS__))
#define READ_FIELD_LIST_POD_OPT(MEMBER, ...)                       READ_FIELD_LIST_CUSTOM_OPT(MEMBER, scheme<container_pod_tag<typename std::decay<decltype(t.MEMBER)>::type>>, AS_IS(__VA_ARGS__))
#define READ_FIELD_LIST_POD_OPT_FORCE(MEMBER, ...)                 READ_FIELD_LIST_CUSTOM_OPT(MEMBER, AS_IS(scheme<container_pod_tag<typename std::decay<decltype(t.MEMBER)>::type, true>>), AS_IS(__VA_ARGS__))
#define READ_FIELD_LIST_POD_OPT_NAME(NAME, MEMBER, ...)            READ_FIELD_LIST_CUSTOM_OPT_NAME(NAME, MEMBER, scheme<container_pod_tag<typename std::decay<decltype(MEMBER)>::type>>, AS_IS(__VA_ARGS__))
#define READ_FIELD_LIST_POD(MEMBER)                                READ_FIELD_LIST_CUSTOM(MEMBER, scheme<container_pod_tag<typename std::decay<decltype(t.MEMBER)>::type>>)
#define READ_FIELD_LIST_POD_FORCE(MEMBER)                          READ_FIELD_LIST_CUSTOM(MEMBER, AS_IS(scheme<container_pod_tag<typename std::decay<decltype(t.MEMBER)>::type, true>>))
#define READ_FIELD_LIST_POD_OPT_NAME_FORCE(NAME, MEMBER, ...)      READ_FIELD_LIST_CUSTOM_OPT_NAME(NAME, MEMBER, AS_IS(scheme<container_pod_tag<typename std::decay<decltype(MEMBER)>::type, true>>), AS_IS(__VA_ARGS__))
#define READ_FIELD_LIST(MEMBER)                                    READ_FIELD_LIST_OPT(MEMBER)
#define READ_FIELD_POD(MEMBER)                                     READ_FIELD_CUSTOM(MEMBER, scheme<pod_tag<typename std::decay<decltype(t.MEMBER)>::type>>)
#define READ_FIELD_POD_OPT(MEMBER, ENABLER)                        READ_FIELD_CUSTOM_OPT(MEMBER, scheme<pod_tag<typename std::decay<decltype(t.MEMBER)>::type>>, AS_IS(ENABLER))
#define READ_FIELD_POD_OPT_NAME(NAME, MEMBER, ENABLER)             READ_FIELD_CUSTOM_OPT_NAME(NAME, MEMBER, scheme<pod_tag<typename std::decay<decltype(MEMBER)>::type>>, AS_IS(ENABLER))
#define END_READ() \
    template<typename ...__T> bool enabled() const { return enabled(selector<__T...>()); }\
    template<typename ...__T> auto get() const -> decltype(get(selector<__T...>())) { return get(selector<__T...>()); }\
  };
#define READ(TYPE) \
  BEGIN_READ_RAW(\
  public:\
    read_t(const TYPE &t) {}\
  )\
  END_READ()
#define BEGIN_WRITE_RAW(...) \
  struct write_t {\
  __VA_ARGS__\
  public:\
    template<typename ...> struct selector{};
#define BEGIN_WRITE(TYPE) \
  BEGIN_WRITE_RAW(\
  private:\
    TYPE &t;\
  public:\
    write_t(TYPE &t): t(t) {}\
  )
#define WRITE_FIELD_CUSTOM_OPT_NAME(NAME, MEMBER, SCHEME, SET_DEFAULT) \
    bool set_default(selector<KEY(NAME), typename field_key<typename SCHEME::tags>::type>) {SET_DEFAULT}\
    typename SCHEME::write_t get(selector<KEY(NAME), typename field_key<typename SCHEME::tags>::type>) {static_assert(std::is_same<void, decltype(is_lvalue_reference(MEMBER))>::value, ""); return MEMBER;}
#define WRITE_FIELD_CUSTOM_OPT(MEMBER, SCHEME, SET_DEFAULT)         WRITE_FIELD_CUSTOM_OPT_NAME(#MEMBER, t.MEMBER, AS_IS(SCHEME), AS_IS(SET_DEFAULT))
#define WRITE_FIELD_OPT_NAME(NAME, MEMBER, ENABLER)                 WRITE_FIELD_CUSTOM_OPT_NAME(NAME, MEMBER, scheme<typename std::decay<decltype(MEMBER)>::type>, AS_IS(ENABLER))
#define WRITE_FIELD_OPT(MEMBER, ENABLER)                            WRITE_FIELD_CUSTOM_OPT(MEMBER, scheme<typename std::decay<decltype(t.MEMBER)>::type>, AS_IS(ENABLER))
#define WRITE_FIELD_CUSTOM(MEMBER, SCHEME)                          WRITE_FIELD_CUSTOM_OPT(MEMBER, AS_IS(SCHEME), {return false;})
#define WRITE_FIELD_WITH_DEFAULT(MEMBER, DEFAULT)                   WRITE_FIELD_OPT(MEMBER, {t.MEMBER = DEFAULT; return true;})
#define WRITE_FIELD(MEMBER)                                         WRITE_FIELD_OPT(MEMBER, {return false;})
#define WRITE_FIELD_NAME(NAME, MEMBER)                              WRITE_FIELD_CUSTOM_OPT_NAME(NAME, MEMBER, scheme<typename std::decay<decltype(MEMBER)>::type>, {return false;})
#define WRITE_FIELD_LIST_CUSTOM_OPT(MEMBER, SCHEME, ...)            WRITE_FIELD_CUSTOM_OPT(MEMBER, AS_IS(SCHEME), {AS_IS(__VA_ARGS__); return true;})
#define WRITE_FIELD_LIST_CUSTOM_OPT_NAME(NAME, MEMBER, SCHEME, ...) WRITE_FIELD_CUSTOM_OPT_NAME(NAME, MEMBER, AS_IS(SCHEME), {AS_IS(__VA_ARGS__); return true;})
#define WRITE_FIELD_LIST_OPT_NAME(NAME, MEMBER, ...)                WRITE_FIELD_LIST_CUSTOM_OPT_NAME(NAME, MEMBER, scheme<container_tag<typename std::decay<decltype(MEMBER)>::type>>, AS_IS(__VA_ARGS__))
#define WRITE_FIELD_LIST_OPT(MEMBER, ...)                           WRITE_FIELD_LIST_CUSTOM_OPT(MEMBER, scheme<container_tag<typename std::decay<decltype(t.MEMBER)>::type>>, AS_IS(__VA_ARGS__))
#define WRITE_FIELD_LIST_CUSTOM(MEMBER, ...)                        WRITE_FIELD_LIST_CUSTOM_OPT(MEMBER, AS_IS(__VA_ARGS__), {t.MEMBER.clear();})
#define WRITE_FIELD_LIST_POD_OPT(MEMBER, ...)                       WRITE_FIELD_LIST_CUSTOM_OPT(MEMBER, scheme<container_pod_tag<typename std::decay<decltype(t.MEMBER)>::type>>, AS_IS(__VA_ARGS__))
#define WRITE_FIELD_LIST_POD_OPT_FORCE(MEMBER, ...)                 WRITE_FIELD_LIST_CUSTOM_OPT(MEMBER, AS_IS(scheme<container_pod_tag<typename std::decay<decltype(t.MEMBER)>::type, true>>), AS_IS(__VA_ARGS__))
#define WRITE_FIELD_LIST_POD_OPT_NAME(NAME, MEMBER, ...)            WRITE_FIELD_LIST_CUSTOM_OPT_NAME(NAME, MEMBER, scheme<container_pod_tag<typename std::decay<decltype(MEMBER)>::type>>, AS_IS(__VA_ARGS__))
#define WRITE_FIELD_LIST_POD(MEMBER)                                WRITE_FIELD_LIST_CUSTOM(MEMBER, scheme<container_pod_tag<typename std::decay<decltype(t.MEMBER)>::type>>)
#define WRITE_FIELD_LIST_POD_FORCE(MEMBER)                          WRITE_FIELD_LIST_CUSTOM(MEMBER, AS_IS(scheme<container_pod_tag<typename std::decay<decltype(t.MEMBER)>::type, true>>))
#define WRITE_FIELD_LIST_POD_OPT_NAME_FORCE(NAME, MEMBER, ...)      WRITE_FIELD_LIST_CUSTOM_OPT_NAME(NAME, MEMBER, AS_IS(scheme<container_pod_tag<typename std::decay<decltype(MEMBER)>::type, true>>), AS_IS(__VA_ARGS__))
#define WRITE_FIELD_LIST(MEMBER)                                    WRITE_FIELD_LIST_CUSTOM(MEMBER, scheme<container_tag<typename std::decay<decltype(t.MEMBER)>::type>>)
#define WRITE_FIELD_POD(MEMBER)                                     WRITE_FIELD_CUSTOM(MEMBER, scheme<pod_tag<typename std::decay<decltype(t.MEMBER)>::type>>)
#define WRITE_FIELD_POD_WITH_DEFAULT(MEMBER, DEFAULT)               WRITE_FIELD_CUSTOM_OPT(MEMBER, scheme<pod_tag<typename std::decay<decltype(t.MEMBER)>::type>>, {t.MEMBER = DEFAULT; return true;})
#define WRITE_FIELD_POD_OPT(MEMBER, ENABLER)                        WRITE_FIELD_CUSTOM_OPT(MEMBER, scheme<pod_tag<typename std::decay<decltype(t.MEMBER)>::type>>, AS_IS(ENABLER))
#define WRITE_FIELD_POD_OPT_NAME(NAME, MEMBER, ENABLER)             WRITE_FIELD_CUSTOM_OPT_NAME(NAME, MEMBER, scheme<pod_tag<typename std::decay<decltype(MEMBER)>::type>>, AS_IS(ENABLER))
#define WRITE_CHECK(...)                                            bool set(selector<bool>) {__VA_ARGS__}
#define END_WRITE() \
    template<typename ...__T> bool set_default() { return set_default(selector<__T...>()); }\
    template<typename ...__T> auto get() -> decltype(get(selector<__T...>())) { return get(selector<__T...>()); }\
    template<typename ...__T> bool set(selector<__T...>) { return true; }\
    bool set() { return set(selector<bool>()); }\
  };
#define WRITE(TYPE) \
  BEGIN_WRITE_RAW(\
  public:\
    write_t(TYPE &t) {}\
  )\
  END_WRITE()

}

}
