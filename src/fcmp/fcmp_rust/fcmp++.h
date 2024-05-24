namespace fcmp_rust {
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

// ----- deps C bindings -----

/// Inner integer type that the [`Limb`] newtype wraps.
// TODO: This is only valid for 64-bit platforms
using Word = uint64_t;

/// Big integers are represented as an array of smaller CPU word-size integers
/// called "limbs".
using Limb = Word;


/// Stack-allocated big unsigned integer.
///
/// Generic over the given number of `LIMBS`
///
/// # Encoding support
/// This type supports many different types of encodings, either via the
/// [`Encoding`][`crate::Encoding`] trait or various `const fn` decoding and
/// encoding functions that can be used with [`Uint`] constants.
///
/// Optional crate features for encoding (off-by-default):
/// - `generic-array`: enables [`ArrayEncoding`][`crate::ArrayEncoding`] trait which can be used to
///   [`Uint`] as `GenericArray<u8, N>` and a [`ArrayDecoding`][`crate::ArrayDecoding`] trait which
///   can be used to `GenericArray<u8, N>` as [`Uint`].
/// - `rlp`: support for [Recursive Length Prefix (RLP)][RLP] encoding.
///
/// [RLP]: https://eth.wiki/fundamentals/rlp
template<uintptr_t LIMBS>
struct Uint {
  /// Inner limb array. Stored from least significant to most significant.
  Limb limbs[LIMBS];
};


/// A residue mod `MOD`, represented using `LIMBS` limbs. The modulus of this residue is constant, so it cannot be set at runtime.
/// Internally, the value is stored in Montgomery form (multiplied by MOD::R) until it is retrieved.
template<uintptr_t LIMBS>
struct Residue {
  Uint<LIMBS> montgomery_form;
};


/// A constant-time implementation of the Ed25519 field.
struct SeleneScalar {
  Residue<4> _0;
};


/// The field novel to Helios/Selene.
struct HeliosScalar {
  Residue<4> _0;
};

struct HeliosPoint {
  SeleneScalar x;
  SeleneScalar y;
  SeleneScalar z;
};

struct SelenePoint {
  HeliosScalar x;
  HeliosScalar y;
  HeliosScalar z;
};

// ----- End deps C bindings -----

template<typename T>
struct CResult {
  T value;
  void* err;
};

template<typename T>
struct Slice {
  const T *buf;
  uintptr_t len;
};

using HeliosScalarSlice = Slice<HeliosScalar>;

using SeleneScalarSlice = Slice<SeleneScalar>;

extern "C" {
HeliosPoint helios_hash_init_point();

SelenePoint selene_hash_init_point();

uint8_t *helios_scalar_to_bytes(HeliosScalar helios_scalar);

uint8_t *selene_scalar_to_bytes(SeleneScalar selene_scalar);

uint8_t *helios_point_to_bytes(HeliosPoint helios_point);

uint8_t *selene_point_to_bytes(SelenePoint selene_point);

SeleneScalar ed25519_point_to_selene_scalar(const uint8_t *ed25519_point);

HeliosScalar selene_point_to_helios_scalar(SelenePoint selene_point);

SeleneScalar helios_point_to_selene_scalar(HeliosPoint helios_point);

HeliosScalar helios_zero_scalar();

SeleneScalar selene_zero_scalar();

CResult<HeliosPoint> hash_grow_helios(HeliosPoint existing_hash,
                                             uintptr_t offset,
                                             HeliosScalarSlice prior_children,
                                             HeliosScalarSlice new_children);

CResult<SelenePoint> hash_grow_selene(SelenePoint existing_hash,
                                             uintptr_t offset,
                                             SeleneScalarSlice prior_children,
                                             SeleneScalarSlice new_children);

} // extern "C"

}
