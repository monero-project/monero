use std::{io, sync::OnceLock};

use rand_core::OsRng;

use ciphersuite::{
    group::{
        ff::{Field, PrimeField},
        Group, GroupEncoding,
    },
    Ciphersuite, Ed25519, Helios, Selene,
};
use helioselene::{
    Field25519 as SeleneScalar, HeliosPoint, HelioseleneField as HeliosScalar, SelenePoint,
};
use transcript::RecommendedTranscript;

use generalized_bulletproofs::Generators;

use ec_divisors::DivisorCurve;
use full_chain_membership_proofs::tree::hash_grow;

// TODO: Use a macro to de-duplicate some of of this code

pub const HELIOS_GENERATORS_LENGTH: usize = 128;
pub const SELENE_GENERATORS_LENGTH: usize = 256;

static HELIOS_GENERATORS: OnceLock<Generators<RecommendedTranscript, Helios>> = OnceLock::new();
static SELENE_GENERATORS: OnceLock<Generators<RecommendedTranscript, Selene>> = OnceLock::new();

static HELIOS_HASH_INIT: OnceLock<HeliosPoint> = OnceLock::new();
static SELENE_HASH_INIT: OnceLock<SelenePoint> = OnceLock::new();

// TODO: Don't use random generators
fn helios_generators() -> &'static Generators<RecommendedTranscript, Helios> {
    HELIOS_GENERATORS.get_or_init(|| {
        generalized_bulletproofs::tests::generators::<Helios>(HELIOS_GENERATORS_LENGTH)
    })
}

fn selene_generators() -> &'static Generators<RecommendedTranscript, Selene> {
    SELENE_GENERATORS.get_or_init(|| {
        generalized_bulletproofs::tests::generators::<Selene>(SELENE_GENERATORS_LENGTH)
    })
}

#[no_mangle]
pub extern "C" fn helios_hash_init_point() -> HeliosPoint {
    *HELIOS_HASH_INIT.get_or_init(|| HeliosPoint::random(&mut OsRng))
}

#[no_mangle]
pub extern "C" fn selene_hash_init_point() -> SelenePoint {
    *SELENE_HASH_INIT.get_or_init(|| SelenePoint::random(&mut OsRng))
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

// Get the x coordinate of the ed25519 point
// TODO: Move this to C++
#[allow(clippy::not_unsafe_ptr_arg_deref)]
#[no_mangle]
pub extern "C" fn ed25519_point_to_selene_scalar(ed25519_point: *const u8) -> SeleneScalar {
    let mut ed25519_point = unsafe { core::slice::from_raw_parts(ed25519_point, 32) };
    // TODO: If not moved to C++, at least return an error here (instead of unwrapping)
    let ed25519_point = <Ed25519>::read_G(&mut ed25519_point).unwrap();

    let xy_coords = <Ed25519 as Ciphersuite>::G::to_xy(ed25519_point);
    let x: SeleneScalar = xy_coords.0;
    x
}

#[no_mangle]
pub extern "C" fn selene_point_to_helios_scalar(selene_point: SelenePoint) -> HeliosScalar {
    let xy_coords = SelenePoint::to_xy(selene_point);
    let x: HeliosScalar = xy_coords.0;
    x
}

#[no_mangle]
pub extern "C" fn helios_point_to_selene_scalar(helios_point: HeliosPoint) -> SeleneScalar {
    let xy_coords = HeliosPoint::to_xy(helios_point);
    let x: SeleneScalar = xy_coords.0;
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
    value: T,
    err: *const E,
}
impl<T, E> CResult<T, E> {
    fn ok(value: T) -> Self {
        CResult {
            value,
            err: core::ptr::null(),
        }
    }
    fn err(default: T, err: E) -> Self {
        CResult {
            value: default,
            err: Box::into_raw(Box::new(err)),
        }
    }
}

#[no_mangle]
pub extern "C" fn hash_grow_helios(
    existing_hash: HeliosPoint,
    offset: usize,
    prior_children: HeliosScalarSlice,
    new_children: HeliosScalarSlice,
) -> CResult<HeliosPoint, io::Error> {
    let hash = hash_grow(
        helios_generators(),
        existing_hash,
        offset,
        prior_children.into(),
        new_children.into(),
    );

    if let Some(hash) = hash {
        CResult::ok(hash)
    } else {
        CResult::err(
            HeliosPoint::identity(),
            io::Error::new(io::ErrorKind::Other, "failed to grow hash"),
        )
    }
}

#[no_mangle]
pub extern "C" fn hash_grow_selene(
    existing_hash: SelenePoint,
    offset: usize,
    prior_children: SeleneScalarSlice,
    new_children: SeleneScalarSlice,
) -> CResult<SelenePoint, io::Error> {
    let hash = hash_grow(
        selene_generators(),
        existing_hash,
        offset,
        prior_children.into(),
        new_children.into(),
    );

    if let Some(hash) = hash {
        CResult::ok(hash)
    } else {
        CResult::err(
            SelenePoint::identity(),
            io::Error::new(io::ErrorKind::Other, "failed to grow hash"),
        )
    }
}
