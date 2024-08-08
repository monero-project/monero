use ciphersuite::{
    group::{
        ff::{Field, PrimeField},
        GroupEncoding,
    },
    Ciphersuite, Helios, Selene,
};
use helioselene::{
    Field25519 as SeleneScalar, HeliosPoint, HelioseleneField as HeliosScalar, SelenePoint,
};

use ec_divisors::DivisorCurve;
use full_chain_membership_proofs::tree::{hash_grow, hash_trim};

use monero_fcmp_plus_plus::{HELIOS_HASH_INIT, SELENE_HASH_INIT, HELIOS_GENERATORS, SELENE_GENERATORS};

// TODO: Use a macro to de-duplicate some of of this code

#[no_mangle]
pub extern "C" fn helios_hash_init_point() -> HeliosPoint {
    HELIOS_HASH_INIT()
}

#[no_mangle]
pub extern "C" fn selene_hash_init_point() -> SelenePoint {
    SELENE_HASH_INIT()
}

fn c_u8_32(bytes: [u8; 32]) -> *const u8 {
    let arr_ptr = Box::into_raw(Box::new(bytes));
    arr_ptr as *const u8
}

#[no_mangle]
pub extern "C" fn helios_scalar_to_bytes(helios_scalar: HeliosScalar) -> *const u8 {
    c_u8_32(helios_scalar.to_repr())
}

#[no_mangle]
pub extern "C" fn selene_scalar_to_bytes(selene_scalar: SeleneScalar) -> *const u8 {
    c_u8_32(selene_scalar.to_repr())
}

#[no_mangle]
pub extern "C" fn helios_point_to_bytes(helios_point: HeliosPoint) -> *const u8 {
    c_u8_32(helios_point.to_bytes())
}

#[no_mangle]
pub extern "C" fn selene_point_to_bytes(selene_point: SelenePoint) -> *const u8 {
    c_u8_32(selene_point.to_bytes())
}

#[allow(clippy::not_unsafe_ptr_arg_deref)]
#[no_mangle]
pub extern "C" fn helios_point_from_bytes(helios_point: *const u8) -> HeliosPoint {
    let mut helios_point = unsafe { core::slice::from_raw_parts(helios_point, 32) };
    // TODO: Return an error here (instead of unwrapping)
    <Helios>::read_G(&mut helios_point).unwrap()
}

#[allow(clippy::not_unsafe_ptr_arg_deref)]
#[no_mangle]
pub extern "C" fn selene_point_from_bytes(selene_point: *const u8) -> SelenePoint {
    let mut selene_point = unsafe { core::slice::from_raw_parts(selene_point, 32) };
    // TODO: Return an error here (instead of unwrapping)
    <Selene>::read_G(&mut selene_point).unwrap()
}

#[allow(clippy::not_unsafe_ptr_arg_deref)]
#[no_mangle]
pub extern "C" fn selene_scalar_from_bytes(selene_scalar: *const u8) -> SeleneScalar {
    let mut selene_scalar = unsafe { core::slice::from_raw_parts(selene_scalar, 32) };
    // TODO: Return an error here (instead of unwrapping)
    <Selene>::read_F(&mut selene_scalar).unwrap()
}

#[no_mangle]
pub extern "C" fn selene_point_to_helios_scalar(selene_point: SelenePoint) -> HeliosScalar {
    let xy_coords = SelenePoint::to_xy(selene_point);
    // TODO: Return an error here (instead of unwrapping)
    let x: HeliosScalar = xy_coords.unwrap().0;
    x
}

#[no_mangle]
pub extern "C" fn helios_point_to_selene_scalar(helios_point: HeliosPoint) -> SeleneScalar {
    let xy_coords = HeliosPoint::to_xy(helios_point);
    // TODO: Return an error here (instead of unwrapping)
    let x: SeleneScalar = xy_coords.unwrap().0;
    x
}

#[no_mangle]
pub extern "C" fn helios_zero_scalar() -> HeliosScalar {
    HeliosScalar::ZERO
}

#[no_mangle]
pub extern "C" fn selene_zero_scalar() -> SeleneScalar {
    SeleneScalar::ZERO
}

#[repr(C)]
pub struct Slice<T> {
    buf: *const T,
    len: usize,
}
pub type HeliosScalarSlice = Slice<HeliosScalar>;
pub type SeleneScalarSlice = Slice<SeleneScalar>;
impl<'a, T> From<Slice<T>> for &'a [T] {
    fn from(slice: Slice<T>) -> Self {
        unsafe { core::slice::from_raw_parts(slice.buf, slice.len) }
    }
}

#[repr(C)]
pub struct CResult<T, E> {
    value: *const T,
    err: *const E,
}
impl<T, E> CResult<T, E> {
    fn ok(value: T) -> Self {
        CResult {
            value: Box::into_raw(Box::new(value)),
            err: core::ptr::null(),
        }
    }
    fn err(err: E) -> Self {
        CResult {
            value: core::ptr::null(),
            err: Box::into_raw(Box::new(err)),
        }
    }
}

#[no_mangle]
pub extern "C" fn hash_grow_helios(
    existing_hash: HeliosPoint,
    offset: usize,
    existing_child_at_offset: HeliosScalar,
    new_children: HeliosScalarSlice,
) -> CResult<HeliosPoint, ()> {
    let hash = hash_grow(
        HELIOS_GENERATORS(),
        existing_hash,
        offset,
        existing_child_at_offset,
        new_children.into(),
    );

    if let Some(hash) = hash {
        CResult::ok(hash)
    } else {
        CResult::err(())
    }
}

#[no_mangle]
pub extern "C" fn hash_trim_helios(
    existing_hash: HeliosPoint,
    offset: usize,
    children: HeliosScalarSlice,
    child_to_grow_back: HeliosScalar,
) -> CResult<HeliosPoint, ()> {
    let hash = hash_trim(
        HELIOS_GENERATORS(),
        existing_hash,
        offset,
        children.into(),
        child_to_grow_back,
    );

    if let Some(hash) = hash {
        CResult::ok(hash)
    } else {
        CResult::err(())
    }
}

#[no_mangle]
pub extern "C" fn hash_grow_selene(
    existing_hash: SelenePoint,
    offset: usize,
    existing_child_at_offset: SeleneScalar,
    new_children: SeleneScalarSlice,
) -> CResult<SelenePoint, ()> {
    let hash = hash_grow(
        SELENE_GENERATORS(),
        existing_hash,
        offset,
        existing_child_at_offset,
        new_children.into(),
    );

    if let Some(hash) = hash {
        CResult::ok(hash)
    } else {
        CResult::err(())
    }
}

#[no_mangle]
pub extern "C" fn hash_trim_selene(
    existing_hash: SelenePoint,
    offset: usize,
    children: SeleneScalarSlice,
    child_to_grow_back: SeleneScalar,
) -> CResult<SelenePoint, ()> {
    let hash = hash_trim(
        SELENE_GENERATORS(),
        existing_hash,
        offset,
        children.into(),
        child_to_grow_back,
    );

    if let Some(hash) = hash {
        CResult::ok(hash)
    } else {
        CResult::err(())
    }
}

// https://github.com/rust-lang/rust/issues/79609
#[cfg(all(target_os = "windows", target_arch = "x86"))]
#[no_mangle]
pub extern "C" fn _Unwind_Resume() {}
