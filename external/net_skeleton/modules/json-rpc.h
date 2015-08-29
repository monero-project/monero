/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef NS_JSON_RPC_HEADER_DEFINED
#define NS_JSON_RPC_HEADER_DEFINED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* JSON-RPC standard error codes */
#define JSON_RPC_PARSE_ERROR              (-32700)
#define JSON_RPC_INVALID_REQUEST_ERROR    (-32600)
#define JSON_RPC_METHOD_NOT_FOUND_ERROR   (-32601)
#define JSON_RPC_INVALID_PARAMS_ERROR     (-32602)
#define JSON_RPC_INTERNAL_ERROR           (-32603)
#define JSON_RPC_SERVER_ERROR             (-32000)

struct ns_rpc_request {
  struct json_token *message;   /* Whole RPC message */
  struct json_token *id;        /* Message ID */
  struct json_token *method;    /* Method name */
  struct json_token *params;    /* Method params */
};

struct ns_rpc_reply {
  struct json_token *message;   /* Whole RPC message */
  struct json_token *id;        /* Message ID */
  struct json_token *result;    /* Remote call result */
};

struct ns_rpc_error {
  struct json_token *message;   /* Whole RPC message */
  struct json_token *id;        /* Message ID */
  struct json_token *error_code;      /* error.code */
  struct json_token *error_message;   /* error.message */
  struct json_token *error_data;      /* error.data, can be NULL */
};

int ns_rpc_parse_request(const char *buf, int len, struct ns_rpc_request *req);

int ns_rpc_parse_reply(const char *buf, int len,
                       struct json_token *toks, int max_toks,
                       struct ns_rpc_reply *, struct ns_rpc_error *);

int ns_rpc_create_request(char *, int, const char *method,
                          const char *id, const char *params_fmt, ...);

int ns_rpc_create_reply(char *, int, const struct ns_rpc_request *req,
                        const char *result_fmt, ...);

int ns_rpc_create_error(char *, int, struct ns_rpc_request *req,
                        int, const char *, const char *, ...);

int ns_rpc_create_std_error(char *, int, struct ns_rpc_request *, int code);

typedef int (*ns_rpc_handler_t)(char *buf, int len, struct ns_rpc_request *);
int ns_rpc_dispatch(const char *buf, int, char *dst, int dst_len,
                    const char **methods, ns_rpc_handler_t *handlers);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif  /* NS_JSON_RPC_HEADER_DEFINED */
