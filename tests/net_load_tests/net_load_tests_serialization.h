#pragma once

#include "net_load_tests.h"
#include "p2p/portable_scheme/scheme.h"

namespace portable_scheme {

namespace scheme_space {

template<>
struct scheme<net_load_tests::CMD_CLOSE_ALL_CONNECTIONS::request> {
  using T = net_load_tests::CMD_CLOSE_ALL_CONNECTIONS::request;
  using tags = map_tag<>;
  READ(T)
  WRITE(T)
};

template<>
struct scheme<net_load_tests::CMD_START_OPEN_CLOSE_TEST::request> {
  using T = net_load_tests::CMD_START_OPEN_CLOSE_TEST::request;
  using tags = map_tag<
    field_tag<KEY("max_opened_conn_count"), base_tag<std::uint64_t>>,
    field_tag<KEY("open_request_target"), base_tag<std::uint64_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(open_request_target)
    READ_FIELD(max_opened_conn_count)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(open_request_target)
    WRITE_FIELD(max_opened_conn_count)
  END_WRITE()
};

template<>
struct scheme<net_load_tests::CMD_START_OPEN_CLOSE_TEST::response> {
  using T = net_load_tests::CMD_START_OPEN_CLOSE_TEST::response;
  using tags = map_tag<>;
  READ(T)
  WRITE(T)
};

template<>
struct scheme<net_load_tests::CMD_GET_STATISTICS::request> {
  using T = net_load_tests::CMD_GET_STATISTICS::request;
  using tags = map_tag<>;
  READ(T)
  WRITE(T)
};

template<>
struct scheme<net_load_tests::CMD_GET_STATISTICS::response> {
  using T = net_load_tests::CMD_GET_STATISTICS::response;
  using tags = map_tag<
    field_tag<KEY("close_connection_counter"), base_tag<std::uint64_t>>,
    field_tag<KEY("new_connection_counter"), base_tag<std::uint64_t>>,
    field_tag<KEY("opened_connections_count"), base_tag<std::uint64_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(opened_connections_count)
    READ_FIELD(new_connection_counter)
    READ_FIELD(close_connection_counter)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(opened_connections_count)
    WRITE_FIELD(new_connection_counter)
    WRITE_FIELD(close_connection_counter)
  END_WRITE()
};

template<>
struct scheme<net_load_tests::CMD_RESET_STATISTICS::request> {
  using T = net_load_tests::CMD_RESET_STATISTICS::request;
  using tags = map_tag<>;
  READ(T)
  WRITE(T)
};

template<>
struct scheme<net_load_tests::CMD_RESET_STATISTICS::response> {
  using T = net_load_tests::CMD_RESET_STATISTICS::response;
  using tags = map_tag<>;
  READ(T)
  WRITE(T)
};

template<>
struct scheme<net_load_tests::CMD_SHUTDOWN::request> {
  using T = net_load_tests::CMD_SHUTDOWN::request;
  using tags = map_tag<>;
  READ(T)
  WRITE(T)
};

template<>
struct scheme<net_load_tests::CMD_SEND_DATA_REQUESTS::request> {
  using T = net_load_tests::CMD_SEND_DATA_REQUESTS::request;
  using tags = map_tag<
    field_tag<KEY("request_size"), base_tag<std::uint64_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(request_size)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(request_size)
  END_WRITE()
};

template<>
struct scheme<net_load_tests::CMD_DATA_REQUEST::request> {
  using T = net_load_tests::CMD_DATA_REQUEST::request;
  using tags = map_tag<
    field_tag<KEY("data"), span_tag<>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(data)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(data)
  END_WRITE()
};

template<>
struct scheme<net_load_tests::CMD_DATA_REQUEST::response> {
  using T = net_load_tests::CMD_DATA_REQUEST::response;
  using tags = map_tag<>;
  READ(T)
  WRITE(T)
};

}

}
