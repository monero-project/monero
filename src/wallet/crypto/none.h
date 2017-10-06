#pragma once

#include "crypto/crypto.h"

namespace tools {
    namespace wallet_only {
        namespace none {
            using crypto::generate_key_derivation;
            using crypto::derive_public_key;
        }
    }
}
