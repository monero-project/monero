/* Copyright (c) 2014 Cesanta Software Limited */
/* All rights reserved */

#ifndef NS_DISABLE_JSON_RPC

#include "net_skeleton.h"
#include "json-rpc.h"

int ns_rpc_create_reply(char *buf, int len, const struct ns_rpc_request *req,
                        const char *result_fmt, ...) {
  va_list ap;
  int n = 0;

  n += json_emit(buf + n, len - n, "{s:s,s:V,s:",
                 "jsonrpc", "2.0", "id",
                 req->id == NULL ? "null" : req->id->ptr,
                 req->id == NULL ? 4 : req->id->len,
                 "result");
  va_start(ap, result_fmt);
  n += json_emit_va(buf + n, len - n, result_fmt, ap);
  va_end(ap);

  n += json_emit(buf + n, len - n, "}");

  return n;
}

int ns_rpc_create_request(char *buf, int len, const char *method,
                          const char *id, const char *params_fmt, ...) {
  va_list ap;
  int n = 0;

  n += json_emit(buf + n, len - n, "{s:s,s:s,s:s,s:",
                 "jsonrpc", "2.0", "id", id, "method", method, "params");
  va_start(ap, params_fmt);
  n += json_emit_va(buf + n, len - n, params_fmt, ap);
  va_end(ap);

  n += json_emit(buf + n, len - n, "}");

  return n;
}

int ns_rpc_create_error(char *buf, int len, struct ns_rpc_request *req,
                        int code, const char *message, const char *fmt, ...) {
  va_list ap;
  int n = 0;

  n += json_emit(buf + n, len - n, "{s:s,s:V,s:{s:i,s:s,s:",
                 "jsonrpc", "2.0", "id",
                 req->id == NULL ? "null" : req->id->ptr,
                 req->id == NULL ? 4 : req->id->len,
                 "error", "code", code,
                 "message", message, "data");
  va_start(ap, fmt);
  n += json_emit_va(buf + n, len - n, fmt, ap);
  va_end(ap);

  n += json_emit(buf + n, len - n, "}}");

  return n;
}

int ns_rpc_create_std_error(char *buf, int len, struct ns_rpc_request *req,
                            int code) {
  const char *message = NULL;

  switch (code) {
    case JSON_RPC_PARSE_ERROR: message = "parse error"; break;
    case JSON_RPC_INVALID_REQUEST_ERROR: message = "invalid request"; break;
    case JSON_RPC_METHOD_NOT_FOUND_ERROR: message = "method not found"; break;
    case JSON_RPC_INVALID_PARAMS_ERROR: message = "invalid parameters"; break;
    case JSON_RPC_SERVER_ERROR: message = "server error"; break;
    default: message = "unspecified error"; break;
  }

  return ns_rpc_create_error(buf, len, req, code, message, "N");
}

int ns_rpc_dispatch(const char *buf, int len, char *dst, int dst_len,
                    const char **methods, ns_rpc_handler_t *handlers) {
  struct json_token tokens[200];
  struct ns_rpc_request req;
  int i, n;

  memset(&req, 0, sizeof(req));
  n = parse_json(buf, len, tokens, sizeof(tokens) / sizeof(tokens[0]));
  if (n <= 0) {
    int err_code = (n == JSON_STRING_INVALID) ?
      JSON_RPC_PARSE_ERROR : JSON_RPC_SERVER_ERROR;
    return ns_rpc_create_std_error(dst, dst_len, &req, err_code);
  }

  req.message = tokens;
  req.id = find_json_token(tokens, "id");
  req.method = find_json_token(tokens, "method");
  req.params = find_json_token(tokens, "params");

  if (req.id == NULL || req.method == NULL) {
    return ns_rpc_create_std_error(dst, dst_len, &req,
                                   JSON_RPC_INVALID_REQUEST_ERROR);
  }

  for (i = 0; methods[i] != NULL; i++) {
    int mlen = strlen(methods[i]);
    if (mlen == req.method->len &&
        memcmp(methods[i], req.method->ptr, mlen) == 0) break;
  }

  if (methods[i] == NULL) {
    return ns_rpc_create_std_error(dst, dst_len, &req,
                                   JSON_RPC_METHOD_NOT_FOUND_ERROR);
  }

  return handlers[i](dst, dst_len, &req);
}

int ns_rpc_parse_reply(const char *buf, int len,
                       struct json_token *toks, int max_toks,
                       struct ns_rpc_reply *rep, struct ns_rpc_error *er) {
  int n = parse_json(buf, len, toks, max_toks);

  memset(rep, 0, sizeof(*rep));
  memset(er, 0, sizeof(*er));

  if (n > 0) {
    if ((rep->result = find_json_token(toks, "result")) != NULL) {
      rep->message = toks;
      rep->id = find_json_token(toks, "id");
    } else {
      er->message = toks;
      er->id = find_json_token(toks, "id");
      er->error_code = find_json_token(toks, "error.code");
      er->error_message = find_json_token(toks, "error.message");
      er->error_data = find_json_token(toks, "error.data");
    }
  }
  return n;
}

#endif  /* NS_DISABLE_JSON_RPC */
