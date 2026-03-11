use ciphersuite::{
    group::{
        ff::{Field, PrimeField},
        GroupEncoding,
    },
    Ciphersuite,
};

use full_chain_membership_proofs::tree::hash_grow;
use helioselene::{
    Field25519 as SeleneScalar, HeliosPoint, HelioseleneField as HeliosScalar, Selene,
    SelenePoint,
};

use monero_fcmp_plus_plus::{
    HELIOS_FCMP_GENERATORS, SELENE_FCMP_GENERATORS,
};
use monero_generators::{
    HELIOS_HASH_INIT, SELENE_HASH_INIT,
};

use std::os::raw::c_int;

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

// Undefined behavior occurs when the data pointer passed to core::slice::from_raw_parts is null,
// even when len is 0. slice_from_raw_parts_0able() lets you pass p as null, as long as len is 0
const unsafe fn slice_from_raw_parts_0able<'a, T>(p: *const T, len: usize) -> &'a [T] {
    if len == 0 {
        &[]
    } else {
        core::slice::from_raw_parts(p, len)
    }
}

#[repr(C)]
pub struct Slice<T> {
    buf: *const T,
    len: usize,
}

pub type HeliosScalarSlice = Slice<HeliosScalar>;
pub type SeleneScalarSlice = Slice<SeleneScalar>;
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

// https://github.com/rust-lang/rust/issues/79609
#[cfg(all(target_os = "windows", target_arch = "x86"))]
#[no_mangle]
pub extern "C" fn _Unwind_Resume() {}
