#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>


namespace fcmp_pp_rust
{
// ----- deps C bindings -----

/// Inner integer type that the [`Limb`] newtype wraps.
// TODO: test 32-bit platforms
using Word = uintptr_t;

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
  Residue<32 / sizeof(uintptr_t)> _0;
};
static_assert(sizeof(SeleneScalar) == 32, "unexpected size of selene scalar");


/// The field novel to Helios/Selene.
struct HeliosScalar {
  Residue<32 / sizeof(uintptr_t)> _0;
};
static_assert(sizeof(HeliosScalar) == 32, "unexpected size of helios scalar");

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

struct CResult {
  void* value;
  void* err;
};

template<typename T>
struct Slice {
  const T *buf;
  uintptr_t len;
};

struct OutputBytes {
  const uint8_t *O_bytes;
  const uint8_t *I_bytes;
  const uint8_t *C_bytes;
};

using HeliosScalarSlice = Slice<HeliosScalar>;

using SeleneScalarSlice = Slice<SeleneScalar>;

using OutputSlice = Slice<OutputBytes>;

using HeliosScalarChunks = Slice<HeliosScalarSlice>;

using SeleneScalarChunks = Slice<SeleneScalarSlice>;

extern "C" {
HeliosPoint helios_hash_init_point();

SelenePoint selene_hash_init_point();

uint8_t *helios_scalar_to_bytes(HeliosScalar helios_scalar);

uint8_t *selene_scalar_to_bytes(SeleneScalar selene_scalar);

uint8_t *helios_point_to_bytes(HeliosPoint helios_point);

uint8_t *selene_point_to_bytes(SelenePoint selene_point);

HeliosPoint helios_point_from_bytes(const uint8_t *helios_point_bytes);

SelenePoint selene_point_from_bytes(const uint8_t *selene_point_bytes);

SeleneScalar selene_scalar_from_bytes(const uint8_t *selene_scalar_bytes);

HeliosScalar selene_point_to_helios_scalar(SelenePoint selene_point);

SeleneScalar helios_point_to_selene_scalar(HeliosPoint helios_point);

HeliosScalar helios_zero_scalar();

SeleneScalar selene_zero_scalar();

uint8_t *selene_tree_root(SelenePoint selene_point);

uint8_t *helios_tree_root(HeliosPoint helios_point);

CResult hash_grow_helios(HeliosPoint existing_hash,
                                             uintptr_t offset,
                                             HeliosScalar existing_child_at_offset,
                                             HeliosScalarSlice new_children);

CResult hash_trim_helios(HeliosPoint existing_hash,
                                             uintptr_t offset,
                                             HeliosScalarSlice children,
                                             HeliosScalar child_to_grow_back);

CResult hash_grow_selene(SelenePoint existing_hash,
                                             uintptr_t offset,
                                             SeleneScalar existing_child_at_offset,
                                             SeleneScalarSlice new_children);

CResult hash_trim_selene(SelenePoint existing_hash,
                                             uintptr_t offset,
                                             SeleneScalarSlice children,
                                             SeleneScalar child_to_grow_back);

CResult path_new(OutputSlice leaves,
                                             uintptr_t output_idx,
                                             HeliosScalarChunks helios_layer_chunks,
                                             SeleneScalarChunks selene_layer_chunks);

CResult rerandomize_output(OutputBytes output);

uint8_t *pseudo_out(const uint8_t *rerandomized_output);

CResult o_blind(const uint8_t *rerandomized_output);
CResult i_blind(const uint8_t *rerandomized_output);
CResult i_blind_blind(const uint8_t *rerandomized_output);
CResult c_blind(const uint8_t *rerandomized_output);

CResult blind_o_blind(const uint8_t *o_blind);
CResult blind_i_blind(const uint8_t *i_blind);
CResult blind_i_blind_blind(const uint8_t *i_blind_blind);
CResult blind_c_blind(const uint8_t *c_blind);

CResult output_blinds_new(const uint8_t *o_blind,
                                             const uint8_t *i_blind,
                                             const uint8_t *i_blind_blind,
                                             const uint8_t *c_blind);

CResult helios_branch_blind();
CResult selene_branch_blind();

CResult fcmp_prove_input_new(const uint8_t *x,
                                             const uint8_t *y,
                                             const uint8_t *rerandomized_output,
                                             const uint8_t *path,
                                             const uint8_t *output_blinds,
                                             Slice<const uint8_t *>selene_branch_blinds,
                                             Slice<const uint8_t *>helios_branch_blinds);

CResult prove(const uint8_t *signable_tx_hash,
                                             Slice<const uint8_t *> fcmp_prove_inputs,
                                             uintptr_t n_tree_layers);

uintptr_t fcmp_pp_proof_size(uintptr_t n_inputs, uintptr_t n_tree_layers);

bool verify(const uint8_t *signable_tx_hash,
                                             Slice<uint8_t> fcmp_pp_proof_slice,
                                             uintptr_t n_tree_layers,
                                             const uint8_t *tree_root,
                                             Slice<const uint8_t *> pseudo_outs,
                                             Slice<const uint8_t *> key_images);

} // extern "C"
}//namespace fcmp_pp_rust
