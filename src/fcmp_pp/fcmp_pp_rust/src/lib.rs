use rand_core::OsRng;

use ciphersuite::{
    group::{
        ff::{Field, PrimeField},
        GroupEncoding,
    },
    Ciphersuite,
};

use ec_divisors::DivisorCurve;

use full_chain_membership_proofs::tree::hash_grow;
use dalek_ff_group::{Ed25519, EdwardsPoint};
use helioselene::{
    Field25519 as SeleneScalar, Helios, HeliosPoint, HelioseleneField as HeliosScalar,
    Selene, SelenePoint,
};

use monero_fcmp_plus_plus::{
    fcmps::{Fcmp, TreeRoot}, Curves, FcmpPlusPlus,
    HELIOS_FCMP_GENERATORS, SELENE_FCMP_GENERATORS,
};
use monero_fcmp_plus_plus_generators::{
    HELIOS_HASH_INIT, SELENE_HASH_INIT,
};

use std::{mem::{align_of, size_of}, os::raw::c_int};

#[allow(non_snake_case)]
#[repr(C)]
pub struct OutputTuple {
    O: [u8; 32],
    I: [u8; 32],
    C: [u8; 32],
}

// Static assertions
// WARNING: if any of these break, fcmp++.h needs to be modified also
macro_rules! static_assert_size_eq {
    ($T:ty, $N:expr) => {
        const _: [(); $N] = [(); size_of::<$T>()];
    };
}

macro_rules! static_assert_alignment {
    ($T:ty, $N:expr) => {
        const _: [(); $N] = [(); align_of::<$T>()];
    };
}

static_assert_size_eq!(SeleneScalar, 32);
static_assert_alignment!(SeleneScalar, size_of::<usize>());

static_assert_size_eq!(HeliosScalar, 32);
static_assert_alignment!(HeliosScalar, size_of::<usize>());

static_assert_size_eq!(HeliosPoint, 32*3);
static_assert_alignment!(HeliosPoint, size_of::<usize>());

static_assert_size_eq!(SelenePoint, 32*3);
static_assert_alignment!(SelenePoint, size_of::<usize>());

static_assert_size_eq!(OutputTuple, 32*3);
static_assert_alignment!(OutputTuple, 1);

//-------------------------------------------------------------------------------------- Box helpers

fn new_box_raw<T>(obj: T) -> *mut T {
    Box::into_raw(Box::new(obj))
}

// https://doc.rust-lang.org/std/boxed/struct.Box.html#method.from_raw
fn destroy_box<T>(ptr: *mut T) {
    let _ = unsafe { Box::from_raw(ptr) };
}

macro_rules! destroy_fn {
    ($fn_name:ident, $type:ty) => {
        /// # Safety
        ///
        /// This function assumes that the obj was allocated on the heap via
        /// Box::into_raw(Box::new())
        #[no_mangle]
        pub unsafe extern "C" fn $fn_name(obj: *mut $type) {
            destroy_box(obj);
        }
    };
}

//-------------------------------------------------------------------------------------- Curve points

#[no_mangle]
pub extern "C" fn helios_hash_init_point() -> HeliosPoint {
    *HELIOS_HASH_INIT
}

#[no_mangle]
pub extern "C" fn selene_hash_init_point() -> SelenePoint {
    *SELENE_HASH_INIT
}

macro_rules! ec_elem_to_bytes {
    ($fn_name:ident, $Type:ty, $to_bytes:ident) => {
        /// # Safety
        ///
        /// This function assumes a raw pointer to expected obj type, and to have
        /// 32 bytes already allocated for bytes_out.
        #[no_mangle]
        pub unsafe extern "C" fn $fn_name(obj: *const $Type, bytes_out: *mut u8) {
            let bytes_out = core::slice::from_raw_parts_mut(bytes_out, 32);
            bytes_out.clone_from_slice(&(*obj).$to_bytes());
        }
    };
}

macro_rules! ec_elem_from_bytes {
    ($fn_name:ident, $Type:ty, $Curve:ty, $from_bytes:ident) => {
        /// # Safety
        ///
        /// This function assumes 32 bytes allocated, and to be passed a raw pointer to
        /// the expected type.
        #[allow(clippy::not_unsafe_ptr_arg_deref)]
        #[no_mangle]
        pub unsafe extern "C" fn $fn_name(bytes: *const u8, ec_elem_out: *mut $Type) -> c_int {
            if ec_elem_out.is_null() {
                return -1;
            }
            let mut bytes = unsafe { core::slice::from_raw_parts(bytes, 32) };
            match <$Curve>::$from_bytes(&mut bytes) {
                Ok(ec_elem) => {
                    *ec_elem_out = ec_elem;
                    0
                }
                Err(_) => -2,
            }
        }
    };
}

ec_elem_to_bytes!(helios_scalar_to_bytes, HeliosScalar, to_repr);
ec_elem_to_bytes!(selene_scalar_to_bytes, SeleneScalar, to_repr);
ec_elem_to_bytes!(helios_point_to_bytes, HeliosPoint, to_bytes);
ec_elem_to_bytes!(selene_point_to_bytes, SelenePoint, to_bytes);

ec_elem_from_bytes!(selene_scalar_from_bytes, SeleneScalar, Selene, read_F);
ec_elem_from_bytes!(helios_point_from_bytes, HeliosPoint, Helios, read_G);
ec_elem_from_bytes!(selene_point_from_bytes, SelenePoint, Selene, read_G);

macro_rules! point_to_cycle_scalar {
    ($fn_name:ident, $Point:ty, $Scalar:ty) => {
        /// # Safety
        ///
        /// This function assumes scalar_out is a non-null pointer to the expected type.
        #[no_mangle]
        pub unsafe extern "C" fn $fn_name(point: $Point, scalar_out: *mut $Scalar) -> c_int {
            if scalar_out.is_null() {
                return -1;
            }
            let Some(xy_coords) = <$Point>::to_xy(point) else {
                return -2;
            };
            *scalar_out = xy_coords.0;
            0
        }
    };
}

point_to_cycle_scalar!(selene_point_to_helios_scalar, SelenePoint, HeliosScalar);
point_to_cycle_scalar!(helios_point_to_selene_scalar, HeliosPoint, SeleneScalar);

// Undefined behavior occurs when the data pointer passed to core::slice::from_raw_parts is null,
// even when len is 0. slice_from_raw_parts_0able() lets you pass p as null, as long as len is 0
const unsafe fn slice_from_raw_parts_0able<'a, T>(p: *const T, len: usize) -> &'a [T] {
    if len == 0 {
        &[]
    } else {
        core::slice::from_raw_parts(p, len)
    }
}

fn ed25519_point_from_bytes(ed25519_point: *const u8) -> std::io::Result<EdwardsPoint> {
    let mut ed25519_point = unsafe { core::slice::from_raw_parts(ed25519_point, 32) };
    <Ed25519>::read_G(&mut ed25519_point)
}

fn hash_array_from_bytes(
    h: *const u8,
) -> std::result::Result<[u8; 32], std::array::TryFromSliceError> {
    unsafe { core::slice::from_raw_parts(h, 32) }.try_into()
}

#[repr(C)]
pub struct Slice<T> {
    buf: *const T,
    len: usize,
}

pub type HeliosScalarSlice = Slice<HeliosScalar>;
static_assert_size_eq!(HeliosScalarSlice, size_of::<*const HeliosScalar>() + size_of::<usize>());
static_assert_alignment!(HeliosScalarSlice, size_of::<usize>());

pub type SeleneScalarSlice = Slice<SeleneScalar>;
static_assert_size_eq!(SeleneScalarSlice, size_of::<*const SeleneScalar>() + size_of::<usize>());
static_assert_alignment!(SeleneScalarSlice, size_of::<usize>());

impl<T> From<Slice<T>> for &[T] {
    fn from(slice: Slice<T>) -> Self {
        unsafe { slice_from_raw_parts_0able(slice.buf, slice.len) }
    }
}
impl<T> From<&Slice<T>> for &[T] {
    fn from(slice: &Slice<T>) -> Self {
        unsafe { slice_from_raw_parts_0able(slice.buf, slice.len) }
    }
}

#[no_mangle]
pub extern "C" fn helios_zero_scalar() -> HeliosScalar {
    HeliosScalar::ZERO
}

#[no_mangle]
pub extern "C" fn selene_zero_scalar() -> SeleneScalar {
    SeleneScalar::ZERO
}

/// # Safety
///
/// This function expects a non-null pointer to TreeRootUnsafe in tree_root_out.
#[no_mangle]
pub unsafe extern "C" fn selene_tree_root(
    selene_point: SelenePoint,
    tree_root_out: *mut *mut TreeRoot<Selene, Helios>,
) -> c_int {
    if tree_root_out.is_null() {
        return -1;
    }

    let tree_root = TreeRoot::<Selene, Helios>::C1(selene_point);
    *tree_root_out = new_box_raw(tree_root);
    0
}

/// # Safety
///
/// This function expects a non-null pointer to TreeRootUnsafe in tree_root_out.
#[no_mangle]
pub unsafe extern "C" fn helios_tree_root(
    helios_point: HeliosPoint,
    tree_root_out: *mut *mut TreeRoot<Selene, Helios>,
) -> c_int {
    if tree_root_out.is_null() {
        return -1;
    }

    let tree_root = TreeRoot::<Selene, Helios>::C2(helios_point);
    *tree_root_out = new_box_raw(tree_root);
    0
}

destroy_fn!(destroy_tree_root, TreeRoot::<Selene, Helios>);

/// # Safety
///
/// This function expects a valid pointer to a HeliosPoint passed in hash_out.
#[no_mangle]
pub unsafe extern "C" fn hash_grow_helios(
    existing_hash: HeliosPoint,
    offset: usize,
    existing_child_at_offset: HeliosScalar,
    new_children: HeliosScalarSlice,
    hash_out: *mut HeliosPoint,
) -> c_int {
    if hash_out.is_null() {
        return -1;
    }

    let hash = hash_grow(
        &HELIOS_FCMP_GENERATORS.generators,
        existing_hash,
        offset,
        existing_child_at_offset,
        new_children.into(),
    );

    let Some(hash) = hash else {
        return -2;
    };

    *hash_out = hash;
    0
}

/// # Safety
///
/// This function expects a valid pointer to a SelenePoint passed in hash_out.
#[no_mangle]
pub unsafe extern "C" fn hash_grow_selene(
    existing_hash: SelenePoint,
    offset: usize,
    existing_child_at_offset: SeleneScalar,
    new_children: SeleneScalarSlice,
    hash_out: *mut SelenePoint,
) -> c_int {
    if hash_out.is_null() {
        return -1;
    }

    let hash = hash_grow(
        &SELENE_FCMP_GENERATORS.generators,
        existing_hash,
        offset,
        existing_child_at_offset,
        new_children.into(),
    );

    let Some(hash) = hash else {
        return -2;
    };

    *hash_out = hash;
    0
}

#[no_mangle]
pub extern "C" fn membership_proof_size(n_inputs: usize, n_tree_layers: usize) -> usize {
    Fcmp::<Curves>::proof_size(n_inputs, n_tree_layers)
}

#[no_mangle]
pub extern "C" fn fcmp_pp_proof_size(n_inputs: usize, n_tree_layers: usize) -> usize {
    FcmpPlusPlus::proof_size(n_inputs, n_tree_layers)
}

pub struct FcmpPpVerifyInput {
    fcmp_pp: FcmpPlusPlus,
    tree_root: TreeRoot<Selene, Helios>,
    n_tree_layers: usize,
    signable_tx_hash: [u8; 32],
    key_images: Vec<EdwardsPoint>,
}

/// # Safety
///
/// This function assumes that the signable tx hash is 32 bytes, the tree root is heap
/// allocated via a CResult, and pseudo outs and key images are 32 bytes each
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_verify_input_new(
    signable_tx_hash: *const u8,
    proof: *const u8,
    proof_len: usize,
    n_tree_layers: usize,
    tree_root: *const TreeRoot<Selene, Helios>,
    pseudo_outs: Slice<*const u8>,
    key_images: Slice<*const u8>,
    fcmp_pp_verify_input_out: *mut *mut FcmpPpVerifyInput,
) -> c_int {
    if fcmp_pp_verify_input_out.is_null() {
        return -1;
    }

    // Early checks
    let n_inputs = pseudo_outs.len;
    if n_inputs == 0 {
        return -2;
    }
    if n_inputs != key_images.len {
        return -3;
    }
    debug_assert_eq!(proof_len, fcmp_pp_proof_size(n_inputs, n_tree_layers));

    let Ok(signable_tx_hash) = hash_array_from_bytes(signable_tx_hash) else {
        return -4;
    };

    let mut proof: &[u8] = unsafe { core::slice::from_raw_parts(proof, proof_len) };

    // 32 byte pseudo outs
    let pseudo_outs: &[*const u8] = pseudo_outs.into();
    let pseudo_outs: Vec<[u8; 32]> = pseudo_outs
        .iter()
        .map(|&x| {
            let x = unsafe { core::slice::from_raw_parts(x, 32) };
            let mut pseudo_out = [0u8; 32];
            pseudo_out.copy_from_slice(x);
            pseudo_out
        })
        .collect();

    // Read the FCMP++ proof
    let Ok(fcmp_plus_plus) = FcmpPlusPlus::read(&pseudo_outs, n_tree_layers, &mut proof) else {
        return -5;
    };

    let tree_root: TreeRoot<Selene, Helios> = unsafe { *tree_root };

    // Collect de-compressed key images into a Vec
    let key_images_slice: &[*const u8] = key_images.into();
    let mut key_images = Vec::with_capacity(key_images_slice.len());
    for compressed_ki in key_images_slice {
        let Ok(key_image) = ed25519_point_from_bytes(*compressed_ki) else {
            return -6;
        };
        key_images.push(key_image);
    }

    let fcmp_pp_verify_input = FcmpPpVerifyInput {
        fcmp_pp: fcmp_plus_plus,
        tree_root,
        n_tree_layers,
        signable_tx_hash,
        key_images,
    };

    *fcmp_pp_verify_input_out = new_box_raw(fcmp_pp_verify_input);
    0
}

destroy_fn!(destroy_fcmp_pp_verify_input, FcmpPpVerifyInput);

/// # Safety
///
/// This function assumes that the inputs are from fcmp_pp_verify_input_new
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_n_inputs(input: *const FcmpPpVerifyInput) -> usize {
    assert!(!input.is_null());
    let fcmp_pp_verify_input = &*input;
    fcmp_pp_verify_input.key_images.len()
}

/// # Safety
///
/// This function assumes that the inputs are from fcmp_pp_verify_input_new
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_verify(inputs: Slice<*const FcmpPpVerifyInput>) -> bool {
    let inputs: &[*const FcmpPpVerifyInput] = inputs.into();

    let mut ed_verifier = multiexp::BatchVerifier::new(inputs.len());
    let mut c1_verifier = generalized_bulletproofs::Generators::batch_verifier();
    let mut c2_verifier = generalized_bulletproofs::Generators::batch_verifier();

    for &input in inputs {
        if input.is_null() {
            return false;
        }

        // Use ref so the input doesn't get consumed (the caller handles de-allocating)
        let fcmp_pp_verify_input = &*input;

        let Ok(_) = fcmp_pp_verify_input.fcmp_pp.verify(
            &mut OsRng,
            &mut ed_verifier,
            &mut c1_verifier,
            &mut c2_verifier,
            fcmp_pp_verify_input.tree_root,
            fcmp_pp_verify_input.n_tree_layers,
            fcmp_pp_verify_input.signable_tx_hash,
            fcmp_pp_verify_input.key_images.clone(),
        ) else {
            return false;
        };
    }

    ed_verifier.verify_vartime()
        && SELENE_FCMP_GENERATORS.generators.verify(c1_verifier)
        && HELIOS_FCMP_GENERATORS.generators.verify(c2_verifier)
}

// https://github.com/rust-lang/rust/issues/79609
#[cfg(all(target_os = "windows", target_arch = "x86"))]
#[no_mangle]
pub extern "C" fn _Unwind_Resume() {}
