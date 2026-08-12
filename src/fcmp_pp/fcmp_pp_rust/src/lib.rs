use ciphersuite::Ciphersuite;

use helioselene::{Field25519 as SeleneScalar, Selene};

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

static_assert_size_eq!(OutputTuple, 32*3);
static_assert_alignment!(OutputTuple, 1);

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

ec_elem_from_bytes!(selene_scalar_from_bytes, SeleneScalar, Selene, read_F);

// https://github.com/rust-lang/rust/issues/79609
#[cfg(all(target_os = "windows", target_arch = "x86"))]
#[no_mangle]
pub extern "C" fn _Unwind_Resume() {}
