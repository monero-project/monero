use rand_core::OsRng;

use std::io;

use full_chain_membership_proofs::tree::hash_grow;

use transcript::RecommendedTranscript;

use ciphersuite::{group::{Group, GroupEncoding, ff::{PrimeField, Field}}, Ciphersuite, Ed25519, Selene, Helios};

use ec_divisors::DivisorCurve;

use generalized_bulletproofs::Generators;

// TODO: lint
#[cxx::bridge]
mod ffi {
  // Rust types and signatures exposed to C++.
  #[namespace = "fcmp_rust"]
  extern "Rust" {
      // TODO: Separate Helios and Selene namespaces
      type HeliosGenerators;
      type HeliosPoint;
      type HeliosScalar;

      type SeleneGenerators;
      type SelenePoint;
      type SeleneScalar;

      fn random_helios_generators() -> Box<HeliosGenerators>;
      fn random_helios_hash_init_point() -> Box<HeliosPoint>;

      fn random_selene_generators() -> Box<SeleneGenerators>;
      fn random_selene_hash_init_point() -> Box<SelenePoint>;

      fn clone_helios_scalar(helios_scalar: &Box<HeliosScalar>) -> Box<HeliosScalar>;
      fn clone_selene_scalar(selene_scalar: &Box<SeleneScalar>) -> Box<SeleneScalar>;
      fn clone_helios_point(helios_point: &Box<HeliosPoint>) -> Box<HeliosPoint>;
      fn clone_selene_point(selene_point: &Box<SelenePoint>) -> Box<SelenePoint>;

      fn helios_scalar_to_bytes(helios_scalar: &Box<HeliosScalar>) -> [u8; 32];
      fn selene_scalar_to_bytes(selene_scalar: &Box<SeleneScalar>) -> [u8; 32];
      fn helios_point_to_bytes(helios_point: &Box<HeliosPoint>) -> [u8; 32];
      fn selene_point_to_bytes(selene_point: &Box<SelenePoint>) -> [u8; 32];

      fn ed25519_point_to_selene_scalar(ed25519_point: &[u8; 32]) -> Box<SeleneScalar>;
      fn selene_point_to_helios_scalar(selene_point: &Box<SelenePoint>) -> Box<HeliosScalar>;
      fn helios_point_to_selene_scalar(helios_point: &Box<HeliosPoint>) -> Box<SeleneScalar>;

      fn helios_zero_scalar() -> Box<HeliosScalar>;
      fn selene_zero_scalar() -> Box<SeleneScalar>;

      pub fn hash_grow_helios(
        helios_generators: &Box<HeliosGenerators>,
        existing_hash: &Box<HeliosPoint>,
        offset: usize,
        prior_children: &[Box<HeliosScalar>],
        new_children: &[Box<HeliosScalar>]
      ) -> Result<Box<HeliosPoint>>;

      pub fn hash_grow_selene(
        selene_generators: &Box<SeleneGenerators>,
        existing_hash: &Box<SelenePoint>,
        offset: usize,
        prior_children: &[Box<SeleneScalar>],
        new_children: &[Box<SeleneScalar>]
      ) -> Result<Box<SelenePoint>>;
  }
}

// TODO: cleaner const usage of generators
// TODO: try to get closer to underlying types
// TODO: maybe don't do both tuple and Box? Just make these all boxes
pub struct HeliosGenerators(Generators<RecommendedTranscript, Helios>);
pub struct HeliosPoint(<Helios as Ciphersuite>::G);
pub struct HeliosScalar(<Helios as Ciphersuite>::F);

pub struct SeleneGenerators(Generators<RecommendedTranscript, Selene>);
pub struct SelenePoint(<Selene as Ciphersuite>::G);
pub struct SeleneScalar(<Selene as Ciphersuite>::F);

#[allow(non_snake_case)]
pub fn random_helios_generators() -> Box<HeliosGenerators> {
  let helios_generators = generalized_bulletproofs::tests::generators::<Helios>(512);
  Box::new(HeliosGenerators(helios_generators))
}

#[allow(non_snake_case)]
pub fn random_selene_generators() -> Box<SeleneGenerators> {
  let selene_generators = generalized_bulletproofs::tests::generators::<Selene>(512);
  Box::new(SeleneGenerators(selene_generators))
}

#[allow(non_snake_case)]
pub fn random_helios_hash_init_point() -> Box<HeliosPoint> {
  let helios_hash_init_point = <Helios as Ciphersuite>::G::random(&mut OsRng);
  dbg!(&helios_hash_init_point);
  Box::new(HeliosPoint(helios_hash_init_point))
}

#[allow(non_snake_case)]
pub fn random_selene_hash_init_point() -> Box<SelenePoint> {
  let selene_hash_init_point = <Selene as Ciphersuite>::G::random(&mut OsRng);
  dbg!(&selene_hash_init_point);
  Box::new(SelenePoint(selene_hash_init_point))
}

// TODO: should be able to use generics
// TODO: shorter names
pub fn clone_helios_scalar(helios_scalar: &Box<HeliosScalar>) -> Box<HeliosScalar> {
  Box::new(HeliosScalar(helios_scalar.0))
}

pub fn clone_selene_scalar(selene_scalar: &Box<SeleneScalar>) -> Box<SeleneScalar> {
  Box::new(SeleneScalar(selene_scalar.0))
}

pub fn clone_helios_point(helios_point: &Box<HeliosPoint>) -> Box<HeliosPoint> {
  Box::new(HeliosPoint(helios_point.0))
}

pub fn clone_selene_point(selene_point: &Box<SelenePoint>) -> Box<SelenePoint> {
  Box::new(SelenePoint(selene_point.0))
}

// TODO: generics
pub fn helios_scalar_to_bytes(helios_scalar: &Box<HeliosScalar>) -> [u8; 32] {
  helios_scalar.0.to_repr()
}

pub fn selene_scalar_to_bytes(selene_scalar: &Box<SeleneScalar>) -> [u8; 32] {
  selene_scalar.0.to_repr()
}

pub fn helios_point_to_bytes(helios_point: &Box<HeliosPoint>) -> [u8; 32] {
  helios_point.0.to_bytes()
}

pub fn selene_point_to_bytes(selene_point: &Box<SelenePoint>) -> [u8; 32] {
  selene_point.0.to_bytes()
}

// Get the x coordinate of the ed25519 point
// TODO: use generics for below logic
pub fn ed25519_point_to_selene_scalar(ed25519_point: &[u8; 32]) -> Box<SeleneScalar> {
  // TODO: unwrap or else error
  let ed25519_point = <Ed25519>::read_G(&mut ed25519_point.as_slice()).unwrap();

  let xy_coords = <Ed25519 as Ciphersuite>::G::to_xy(ed25519_point);
  let x: <Selene as Ciphersuite>::F = xy_coords.0;
  Box::new(SeleneScalar(x))
}

// TODO: use generics for below logic
pub fn selene_point_to_helios_scalar(selene_point: &Box<SelenePoint>) -> Box<HeliosScalar> {
  let xy_coords = <Selene as Ciphersuite>::G::to_xy(selene_point.0);
  let x: <Helios as Ciphersuite>::F = xy_coords.0;
  Box::new(HeliosScalar(x))
}

// TODO: use generics for below logic
pub fn helios_point_to_selene_scalar(helios_point: &Box<HeliosPoint>) -> Box<SeleneScalar> {
  let xy_coords = <Helios as Ciphersuite>::G::to_xy(helios_point.0);
  let x: <Selene as Ciphersuite>::F = xy_coords.0;
  Box::new(SeleneScalar(x))
}

pub fn helios_zero_scalar() -> Box<HeliosScalar> {
  Box::new(HeliosScalar(<Helios as Ciphersuite>::F::ZERO))
}

pub fn selene_zero_scalar() -> Box<SeleneScalar> {
  Box::new(SeleneScalar(<Selene as Ciphersuite>::F::ZERO))
}

// TODO: use generics for curves
pub fn hash_grow_helios(
  helios_generators: &Box<HeliosGenerators>,
  existing_hash: &Box<HeliosPoint>,
  offset: usize,
  prior_children: &[Box<HeliosScalar>],
  new_children: &[Box<HeliosScalar>]
) -> Result<Box<HeliosPoint>, io::Error> {
  let prior_children = prior_children.iter().map(|c| c.0).collect::<Vec<_>>();
  let new_children = new_children.iter().map(|c| c.0).collect::<Vec<_>>();

  let hash = hash_grow(
    &helios_generators.0,
    existing_hash.0,
    offset,
    &prior_children,
    &new_children
  );

  if let Some(hash) = hash {
    Ok(Box::new(HeliosPoint(hash)))
  } else {
    Err(io::Error::new(io::ErrorKind::Other, "failed to grow hash"))
  }
}

// TODO: use generics for curves
pub fn hash_grow_selene(
  selene_generators: &Box<SeleneGenerators>,
  existing_hash: &Box<SelenePoint>,
  offset: usize,
  prior_children: &[Box<SeleneScalar>],
  new_children: &[Box<SeleneScalar>]
) -> Result<Box<SelenePoint>, io::Error> {
  let prior_children = prior_children.iter().map(|c| c.0).collect::<Vec<_>>();
  let new_children = new_children.iter().map(|c| c.0).collect::<Vec<_>>();

  let hash = hash_grow(
    &selene_generators.0,
    existing_hash.0,
    offset,
    &prior_children,
    &new_children
  );

  if let Some(hash) = hash {
    Ok(Box::new(SelenePoint(hash)))
  } else {
    Err(io::Error::new(io::ErrorKind::Other, "failed to grow hash"))
  }
}
