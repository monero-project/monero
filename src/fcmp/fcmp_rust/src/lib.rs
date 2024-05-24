use std::io;

use rand_core::OsRng;

use transcript::RecommendedTranscript;
use helioselene::{HeliosPoint, SelenePoint, HelioseleneField as HeliosScalar, Field25519 as SeleneScalar};
use ciphersuite::{group::{Group, GroupEncoding, ff::{PrimeField, Field}}, Ciphersuite, Ed25519, Selene, Helios};

use generalized_bulletproofs::Generators;

use ec_divisors::DivisorCurve;
use full_chain_membership_proofs::tree::hash_grow;

// TODO: cleaner const usage of generators
// TODO: try to get closer to underlying types
// TODO: maybe don't do both tuple and Box? Just make these all boxes
#[repr(C)]
pub struct HeliosGenerators {
  generators: Box<Generators<RecommendedTranscript, Helios>>
}
#[repr(C)]
pub struct SeleneGenerators {
  generators: Box<Generators<RecommendedTranscript, Selene>>
}

#[no_mangle]
pub extern "C" fn random_helios_generators(n: usize) -> HeliosGenerators {
  let helios_generators = generalized_bulletproofs::tests::generators::<Helios>(n);
  HeliosGenerators { generators: Box::new(helios_generators) }
}

#[no_mangle]
pub extern "C" fn random_selene_generators(n: usize) -> SeleneGenerators {
  let selene_generators = generalized_bulletproofs::tests::generators::<Selene>(n);
  SeleneGenerators { generators: Box::new(selene_generators) }
}

#[no_mangle]
pub extern "C" fn random_helios_hash_init_point() -> HeliosPoint {
  HeliosPoint::random(&mut OsRng)
}

#[no_mangle]
pub extern "C" fn random_selene_hash_init_point() -> SelenePoint {
  SelenePoint::random(&mut OsRng)
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
// TODO: use generics for below logic
#[no_mangle]
pub extern "C" fn ed25519_point_to_selene_scalar(ed25519_point: *const u8) -> SeleneScalar {
  // TODO: unwrap or else error
  let mut ed25519_point = unsafe { core::slice::from_raw_parts(ed25519_point, 32) };
  let ed25519_point = <Ed25519>::read_G(&mut ed25519_point).unwrap();

  let xy_coords = <Ed25519 as Ciphersuite>::G::to_xy(ed25519_point);
  let x: SeleneScalar = xy_coords.0;
  x
}

// TODO: use generics for below logic
#[no_mangle]
pub extern "C" fn selene_point_to_helios_scalar(selene_point: SelenePoint) -> HeliosScalar {
  let xy_coords = SelenePoint::to_xy(selene_point);
  let x: HeliosScalar = xy_coords.0;
  x
}

// TODO: use generics for below logic
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
impl<'a, T> Into<&'a [T]> for Slice<T> {
  fn into(self) -> &'a [T] {
    unsafe { core::slice::from_raw_parts(self.buf, self.len) }
  }
}

#[repr(C)]
pub struct CResult<T, E> {
  value: T,
  err: *const E,
}
impl<T, E> CResult<T, E> {
  fn ok(value: T) -> Self {
    CResult { value, err: core::ptr::null() }
  }
  fn err(default: T, err: E) -> Self {
    CResult { value: default, err: Box::into_raw(Box::new(err)) }
  }
}

// TODO: use generics for curves
#[no_mangle]
pub extern "C" fn hash_grow_helios(
  helios_generators: &HeliosGenerators,
  existing_hash: HeliosPoint,
  offset: usize,
  prior_children: HeliosScalarSlice,
  new_children: HeliosScalarSlice
) -> CResult<HeliosPoint, io::Error> {
  let hash = hash_grow(
    &helios_generators.generators,
    existing_hash,
    offset,
    prior_children.into(),
    new_children.into(),
  );

  if let Some(hash) = hash {
    CResult::ok(hash)
  } else {
    CResult::err(HeliosPoint::identity(), io::Error::new(io::ErrorKind::Other, "failed to grow hash"))
  }
}

// TODO: use generics for curves
#[no_mangle]
pub extern "C" fn hash_grow_selene(
  selene_generators: &SeleneGenerators,
  existing_hash: SelenePoint,
  offset: usize,
  prior_children: SeleneScalarSlice,
  new_children: SeleneScalarSlice
) -> CResult<SelenePoint, io::Error> {
  let hash = hash_grow(
    &selene_generators.generators,
    existing_hash,
    offset,
    prior_children.into(),
    new_children.into(),
  );

  if let Some(hash) = hash {
    CResult::ok(hash)
  } else {
    CResult::err(SelenePoint::identity(), io::Error::new(io::ErrorKind::Other, "failed to grow hash"))
  }

}
