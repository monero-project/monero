#pragma once

namespace tools
{
  static const int WALLET_RETURN_SUCCESS = 0;
  static const int WALLET_RETURN_UNKNOWN_ERROR = 1;
  static const int WALLET_RETURN_INVALID_PASSPHRASE = 10;
  static const int WALLET_RETURN_MISSING_KEYS_FILE = 11;
  static const int WALLET_RETURN_COULD_NOT_READ_KEYS_FILE = 12;
  static const int WALLET_RETURN_MISMATCHED_KEYS_FILE = 13;
  static const int WALLET_RETURN_INVALID_IP = 14;
  static const int WALLET_RETURN_INVALID_PORT = 15;
  static const int WALLET_RETURN_FAILED_TO_SEND_STOP_REQUEST = 16;
}
