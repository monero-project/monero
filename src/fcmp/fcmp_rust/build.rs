fn main() {
  let _ = cxx_build::bridge("src/lib.rs");

  println!("cargo:rerun-if-changed=src/lib.rs");
}