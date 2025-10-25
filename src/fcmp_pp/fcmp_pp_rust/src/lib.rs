use rand_core::OsRng;

use ciphersuite::{
    group::{
        ff::{Field, PrimeField},
        Group, GroupEncoding,
    },
    Ciphersuite, Ed25519,
};
use dalek_ff_group::{EdwardsPoint, Scalar};
use helioselene::{
    Field25519 as SeleneScalar, Helios, HeliosPoint, HelioseleneField as HeliosScalar, Selene,
    SelenePoint,
};

use ec_divisors::{DivisorCurve, ScalarDecomposition};
use full_chain_membership_proofs::tree::hash_grow;

use monero_fcmp_plus_plus::{
    fcmps,
    fcmps::{
        BranchBlind, Branches, CBlind, Fcmp, IBlind, IBlindBlind, OBlind, OutputBlinds, Path,
        TreeRoot,
    },
    sal::{OpenedInputTuple, RerandomizedOutput, SpendAuthAndLinkability},
    Curves, FcmpPlusPlus, Input, Output, FCMP_PARAMS, HELIOS_FCMP_GENERATORS,
    SELENE_FCMP_GENERATORS,
};
use monero_generators::{
    FCMP_PLUS_PLUS_U, FCMP_PLUS_PLUS_V, HELIOS_HASH_INIT, SELENE_HASH_INIT, T,
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

ec_elem_from_bytes!(helios_scalar_from_bytes, HeliosScalar, Helios, read_F);
ec_elem_from_bytes!(selene_scalar_from_bytes, SeleneScalar, Selene, read_F);
ec_elem_from_bytes!(helios_point_from_bytes, HeliosPoint, Helios, read_G);
ec_elem_from_bytes!(selene_point_from_bytes, SelenePoint, Selene, read_G);

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

fn ed25519_scalar_from_bytes(ed25519_scalar: *const u8) -> std::io::Result<Scalar> {
    let mut ed25519_scalar = unsafe { core::slice::from_raw_parts(ed25519_scalar, 32) };
    <Ed25519>::read_F(&mut ed25519_scalar)
}

fn hash_array_from_bytes(
    h: *const u8,
) -> std::result::Result<[u8; 32], std::array::TryFromSliceError> {
    unsafe { core::slice::from_raw_parts(h, 32) }.try_into()
}

// @TODO: this is horrible :(, expose direct read/write for Input in the fcmp-plus-plus crate
fn input_from_bytes(input_bytes: &[u8]) -> std::io::Result<Input> {
    let mut rerandomized_output_bytes = [0u8; 8 * 32];
    if input_bytes.len() != 4 * 32 {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            "passed input slice wrong size",
        ));
    }
    rerandomized_output_bytes[0..4 * 32].copy_from_slice(input_bytes);
    let rerandomized_output = RerandomizedOutput::read(&mut rerandomized_output_bytes.as_slice())?;
    Ok(rerandomized_output.input())
}

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

#[allow(non_snake_case)]
#[repr(C)]
pub struct OutputBytes {
    O: [u8; 32],
    I: [u8; 32],
    C: [u8; 32],
}

#[repr(C)]
pub struct Slice<T> {
    buf: *const T,
    len: usize,
}
pub type HeliosScalarSlice = Slice<HeliosScalar>;
pub type SeleneScalarSlice = Slice<SeleneScalar>;
pub type OutputSlice = Slice<OutputBytes>;
pub type HeliosScalarChunks = Slice<HeliosScalarSlice>;
pub type SeleneScalarChunks = Slice<SeleneScalarSlice>;
pub type HeliosBranchBlindSlice = Slice<*const BranchBlind<<Helios as Ciphersuite>::G>>;
pub type SeleneBranchBlindSlice = Slice<*const BranchBlind<<Selene as Ciphersuite>::G>>;
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

//-------------------------------------------------------------------------------------- Path

/// # Safety
///
/// This function assumes that the leaves passed in are composed of 3 * 32 byte slices, that
/// the output_idx is < n leaves, and that the helios and selene layer chunks are composed
/// of scalars of the expected type.
#[no_mangle]
pub unsafe extern "C" fn path_new(
    leaves: OutputSlice,
    output_idx: usize,
    helios_layer_chunks: HeliosScalarChunks,
    selene_layer_chunks: SeleneScalarChunks,
    path_out: *mut *mut Path<Curves>,
) -> c_int {
    if path_out.is_null() {
        return -1;
    }
    if output_idx >= leaves.len {
        return -2;
    }

    // Collect decompressed leaves
    let leaves_slice: &[OutputBytes] = leaves.into();
    let mut leaves: Vec<Output> = Vec::with_capacity(leaves_slice.len());
    #[allow(non_snake_case)]
    for leaf in leaves_slice {
        let O = if let Some(O) = EdwardsPoint::from_bytes(&leaf.O).into() {
            O
        } else {
            return -3;
        };
        let I = if let Some(I) = EdwardsPoint::from_bytes(&leaf.I).into() {
            I
        } else {
            return -4;
        };
        let C = if let Some(C) = EdwardsPoint::from_bytes(&leaf.C).into() {
            C
        } else {
            return -5;
        };

        let Ok(new_output) = Output::new(O, I, C) else {
            return -6;
        };
        leaves.push(new_output);
    }

    // Output
    let output = leaves[output_idx];

    // Collect helios layer chunks
    let helios_layers: &[HeliosScalarSlice] = helios_layer_chunks.into();
    let helios_layers: Vec<Vec<<Helios as Ciphersuite>::F>> = helios_layers
        .iter()
        .map(|x| {
            let inner_slice: &[<Helios as Ciphersuite>::F] = x.into();
            inner_slice.iter().map(|y| y.to_owned()).collect()
        })
        .collect();

    // Collect selene layer chunks
    let selene_layers: &[SeleneScalarSlice] = selene_layer_chunks.into();
    let selene_layers: Vec<Vec<<Selene as Ciphersuite>::F>> = selene_layers
        .iter()
        .map(|x| {
            let inner_slice: &[<Selene as Ciphersuite>::F] = x.into();
            inner_slice.iter().map(|y| y.to_owned()).collect()
        })
        .collect();

    let curve_2_layers = helios_layers;
    let curve_1_layers = selene_layers;

    let path: Path<Curves> = Path {
        output,
        leaves,
        curve_2_layers,
        curve_1_layers,
    };
    *path_out = new_box_raw(path);
    0
}

destroy_fn!(destroy_path, Path<Curves>);

//-------------------------------------------------------------------------------------- Blindings

//---------------------------------------------- RerandomizedOutput

/// # Safety
///
/// This function expects a non-null pointer to FcmpRerandomizedOutputCompressed in rerandomized_output_bytes.
#[no_mangle]
#[allow(non_snake_case)]
pub unsafe extern "C" fn rerandomize_output(
    output: OutputBytes,
    rerandomized_output_bytes: *mut u8,
) -> c_int {
    let O = if let Some(O) = EdwardsPoint::from_bytes(&output.O).into() {
        O
    } else {
        return -1;
    };
    let I = if let Some(I) = EdwardsPoint::from_bytes(&output.I).into() {
        I
    } else {
        return -2;
    };
    let C = if let Some(C) = EdwardsPoint::from_bytes(&output.C).into() {
        C
    } else {
        return -3;
    };

    let Ok(output) = Output::new(O, I, C) else {
        return -4;
    };

    let mut rerandomized_output_bytes =
        core::slice::from_raw_parts_mut(rerandomized_output_bytes, 8 * 32);

    let rerandomized_output = RerandomizedOutput::new(&mut OsRng, output);
    if rerandomized_output
        .write(&mut rerandomized_output_bytes)
        .is_err()
    {
        -5
    } else {
        0
    }
}

//---------------------------------------------- OBlind

/// # Safety
///
/// This function assumes that the rerandomized output byte buffer being passed
/// in is 8*32 bytes.
#[no_mangle]
pub unsafe extern "C" fn o_blind(
    rerandomized_output_bytes: *const u8,
    o_blind: *mut Scalar,
) -> c_int {
    let mut rerandomized_output_bytes =
        core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    match RerandomizedOutput::read(&mut rerandomized_output_bytes) {
        Ok(rerandomized_output) => {
            o_blind.write(rerandomized_output.o_blind());
            0
        }
        Err(_) => -1,
    }
}

/// # Safety
///
/// This function assumes that the scalar being passed in input was
/// allocated on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn blind_o_blind(
    o_blind: *const Scalar,
    blinded_o_blind_out: *mut *mut OBlind<EdwardsPoint>,
) -> c_int {
    if blinded_o_blind_out.is_null() {
        return -1;
    }

    let scalar_decomp = ScalarDecomposition::new(*o_blind);
    let Some(scalar_decomp) = scalar_decomp else {
        return -2;
    };

    let blinded_o_blind = OBlind::new(EdwardsPoint(*T), scalar_decomp);
    *blinded_o_blind_out = new_box_raw(blinded_o_blind);
    0
}

destroy_fn!(destroy_blinded_o_blind, OBlind<EdwardsPoint>);

//---------------------------------------------- CBlind

/// # Safety
///
/// This function assumes that the rerandomized output byte buffer being passed
/// in is 8*32 bytes.
#[no_mangle]
pub unsafe extern "C" fn c_blind(
    rerandomized_output_bytes: *const u8,
    c_blind: *mut Scalar,
) -> c_int {
    let mut rerandomized_output_bytes =
        core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    match RerandomizedOutput::read(&mut rerandomized_output_bytes) {
        Ok(rerandomized_output) => {
            c_blind.write(rerandomized_output.c_blind());
            0
        }
        Err(_) => -1,
    }
}

/// # Safety
///
/// This function assumes that the scalar being passed in input was
/// allocated on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn blind_c_blind(
    c_blind: *const Scalar,
    blinded_c_blind_out: *mut *mut CBlind<EdwardsPoint>,
) -> c_int {
    if blinded_c_blind_out.is_null() {
        return -1;
    }

    let scalar_decomp = ScalarDecomposition::new(*c_blind);
    let Some(scalar_decomp) = scalar_decomp else {
        return -2;
    };

    let blinded_c_blind = CBlind::new(EdwardsPoint::generator(), scalar_decomp);
    *blinded_c_blind_out = new_box_raw(blinded_c_blind);
    0
}

destroy_fn!(destroy_blinded_c_blind, CBlind<EdwardsPoint>);

//---------------------------------------------- IBlind

/// # Safety
///
/// This function assumes that the rerandomized output byte buffer being passed
/// in is 8*32 bytes.
#[no_mangle]
pub unsafe extern "C" fn i_blind(
    rerandomized_output_bytes: *const u8,
    i_blind: *mut Scalar,
) -> c_int {
    let mut rerandomized_output_bytes =
        core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    match RerandomizedOutput::read(&mut rerandomized_output_bytes) {
        Ok(rerandomized_output) => {
            i_blind.write(rerandomized_output.i_blind());
            0
        }
        Err(_) => -1,
    }
}

/// # Safety
///
/// This function assumes that the scalar being passed in input was
/// allocated on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn blind_i_blind(
    i_blind: *const Scalar,
    blinded_i_blind_out: *mut *mut IBlind<EdwardsPoint>,
) -> c_int {
    if blinded_i_blind_out.is_null() {
        return -1;
    }

    let scalar_decomp = ScalarDecomposition::new(*i_blind);
    let Some(scalar_decomp) = scalar_decomp else {
        return -2;
    };

    let blinded_i_blind = IBlind::new(
        EdwardsPoint(*FCMP_PLUS_PLUS_U),
        EdwardsPoint(*FCMP_PLUS_PLUS_V),
        scalar_decomp,
    );
    *blinded_i_blind_out = new_box_raw(blinded_i_blind);
    0
}

destroy_fn!(destroy_blinded_i_blind, IBlind<EdwardsPoint>);

//---------------------------------------------- IBlindBlind

/// # Safety
///
/// This function assumes that the rerandomized output byte buffer being passed
/// in is 8*32 bytes.
#[no_mangle]
pub unsafe extern "C" fn i_blind_blind(
    rerandomized_output_bytes: *const u8,
    i_blind_blind: *mut Scalar,
) -> c_int {
    let mut rerandomized_output_bytes =
        core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    match RerandomizedOutput::read(&mut rerandomized_output_bytes) {
        Ok(rerandomized_output) => {
            i_blind_blind.write(rerandomized_output.i_blind_blind());
            0
        }
        Err(_) => -1,
    }
}

/// # Safety
///
/// This function assumes that the scalar being passed in input was
/// allocated on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn blind_i_blind_blind(
    i_blind_blind: *const Scalar,
    blinded_i_blind_blind_out: *mut *mut IBlindBlind<EdwardsPoint>,
) -> c_int {
    if blinded_i_blind_blind_out.is_null() {
        return -1;
    }

    let scalar_decomp = ScalarDecomposition::new(*i_blind_blind);
    let Some(scalar_decomp) = scalar_decomp else {
        return -2;
    };

    let blinded_i_blind_blind = IBlindBlind::new(EdwardsPoint(*T), scalar_decomp);
    *blinded_i_blind_blind_out = new_box_raw(blinded_i_blind_blind);
    0
}

destroy_fn!(destroy_blinded_i_blind_blind, IBlindBlind<EdwardsPoint>);

//---------------------------------------------- OutputBlinds

/// # Safety
///
/// This function assumes that the blinds passed in input were allocated
/// on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn output_blinds_new(
    o_blind: *const OBlind<EdwardsPoint>,
    i_blind: *const IBlind<EdwardsPoint>,
    i_blind_blind: *const IBlindBlind<EdwardsPoint>,
    c_blind: *const CBlind<EdwardsPoint>,
    output_blinds_out: *mut *mut OutputBlinds<EdwardsPoint>,
) -> c_int {
    if output_blinds_out.is_null() {
        return -1;
    }

    // Clone so that even if the underlying blinds get dropped, the object remains valid
    let output_blinds = OutputBlinds::new(
        (*o_blind).clone(),
        (*i_blind).clone(),
        (*i_blind_blind).clone(),
        (*c_blind).clone(),
    );
    *output_blinds_out = new_box_raw(output_blinds);
    0
}

destroy_fn!(destroy_output_blinds, OutputBlinds<EdwardsPoint>);

//---------------------------------------------- BranchBlind

/// # Safety
///
/// This function allocates a branch blind on the heap via Box::new, then
/// gets its raw pointer via Box::into_raw, then sets the input pointer's
/// address to point to the raw box pointer. The input parameter must not
/// be null. Also be sure to clean up the branch blind with
/// destroy_helios_branch_blind below.
#[no_mangle]
pub unsafe extern "C" fn generate_helios_branch_blind(
    branch_blind_out: *mut *mut BranchBlind<<Helios as Ciphersuite>::G>,
) -> c_int {
    if branch_blind_out.is_null() {
        return -1;
    }

    let scalar_decomp = ScalarDecomposition::new(<Helios as Ciphersuite>::F::random(&mut OsRng));
    let Some(scalar_decomp) = scalar_decomp else {
        return -2;
    };

    let branch_blind = BranchBlind::<<Helios as Ciphersuite>::G>::new(
        HELIOS_FCMP_GENERATORS.generators.h(),
        scalar_decomp,
    );
    *branch_blind_out = new_box_raw(branch_blind);
    0
}

/// # Safety
///
/// This function allocates a branch blind on the heap via Box::new, then
/// gets its raw pointer via Box::into_raw, then sets the input pointer's
/// address to point to the raw box pointer. The input parameter must not
/// be null. Also be sure to clean up the branch blind with
/// destroy_selene_branch_blind below.
#[no_mangle]
pub unsafe extern "C" fn generate_selene_branch_blind(
    branch_blind_out: *mut *mut BranchBlind<<Selene as Ciphersuite>::G>,
) -> c_int {
    if branch_blind_out.is_null() {
        return -1;
    }

    let scalar_decomp = ScalarDecomposition::new(<Selene as Ciphersuite>::F::random(&mut OsRng));
    let Some(scalar_decomp) = scalar_decomp else {
        return -2;
    };

    let branch_blind = BranchBlind::<<Selene as Ciphersuite>::G>::new(
        SELENE_FCMP_GENERATORS.generators.h(),
        scalar_decomp,
    );
    *branch_blind_out = new_box_raw(branch_blind);
    0
}

destroy_fn!(
    destroy_helios_branch_blind,
    BranchBlind<<Helios as Ciphersuite>::G>
);
destroy_fn!(
    destroy_selene_branch_blind,
    BranchBlind<<Selene as Ciphersuite>::G>
);

//-------------------------------------------------------------------------------------- Fcmp

#[derive(Clone)]
pub struct FcmpPpProveMembershipInput {
    path: Path<Curves>,
    output_blinds: OutputBlinds<EdwardsPoint>,
    c1_branch_blinds: Vec<BranchBlind<<Selene as Ciphersuite>::G>>,
    c2_branch_blinds: Vec<BranchBlind<<Helios as Ciphersuite>::G>>,
}

pub type FcmpPpProveMembershipInputSlice = Slice<*const FcmpPpProveMembershipInput>;

unsafe fn prove_membership_native(
    inputs: &[*const FcmpPpProveMembershipInput],
    n_tree_layers: usize,
) -> Option<Fcmp<Curves>> {
    let paths = inputs.iter().cloned().map(|x| (*x).path.clone()).collect();
    let output_blinds = inputs
        .iter()
        .cloned()
        .map(|x| (*x).output_blinds.clone())
        .collect();

    let c1_branch_blinds: Vec<_> = inputs
        .iter()
        .cloned()
        .map(|x| (*x).c1_branch_blinds.clone())
        .collect::<Vec<_>>()
        .into_iter()
        .flatten()
        .collect();
    let c2_branch_blinds: Vec<_> = inputs
        .iter()
        .cloned()
        .map(|x| (*x).c2_branch_blinds.clone())
        .collect::<Vec<_>>()
        .into_iter()
        .flatten()
        .collect();

    let branches = Branches::new(paths)?;

    if branches.necessary_c1_blinds() != c1_branch_blinds.len() {
        return None;
    }
    if branches.necessary_c2_blinds() != c2_branch_blinds.len() {
        return None;
    }

    let n_branch_blinds = (c1_branch_blinds.len() + c2_branch_blinds.len()) / inputs.len();
    if n_tree_layers != n_branch_blinds + 1 {
        return None;
    }

    let blinded_branches = branches
        .blind(output_blinds, c1_branch_blinds, c2_branch_blinds)
        .ok()?;

    Fcmp::prove(&mut OsRng, &*FCMP_PARAMS, blinded_branches).ok()
}

/// # Safety
///
/// This function assumes that Path and OutputBlinds were
/// allocated on the heap (via Box::into_raw(Box::new())), and the branch
/// blinds are slices of BranchBlind allocated on the heap (via CResult).
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_prove_input_new(
    path: *const Path<Curves>,
    output_blinds: *const OutputBlinds<EdwardsPoint>,
    selene_branch_blinds: SeleneBranchBlindSlice,
    helios_branch_blinds: HeliosBranchBlindSlice,
    fcmp_pp_prove_input_out: *mut *mut FcmpPpProveMembershipInput,
) -> c_int {
    if fcmp_pp_prove_input_out.is_null() {
        return -1;
    }

    // Path and output blinds
    let path = (*path).clone();
    let output_blinds = (*output_blinds).clone();

    // Collect branch blinds
    let c1_branch_blinds: &[*const BranchBlind<<Selene as Ciphersuite>::G>] =
        selene_branch_blinds.into();
    let c1_branch_blinds: Vec<BranchBlind<<Selene as Ciphersuite>::G>> = c1_branch_blinds
        .iter()
        .map(|x| (*x.to_owned()).clone())
        .collect();

    let c2_branch_blinds: &[*const BranchBlind<<Helios as Ciphersuite>::G>] =
        helios_branch_blinds.into();
    let c2_branch_blinds: Vec<BranchBlind<<Helios as Ciphersuite>::G>> = c2_branch_blinds
        .iter()
        .map(|x| (*x.to_owned()).clone())
        .collect();

    let fcmp_pp_prove_input = FcmpPpProveMembershipInput {
        path,
        output_blinds,
        c1_branch_blinds,
        c2_branch_blinds,
    };
    *fcmp_pp_prove_input_out = new_box_raw(fcmp_pp_prove_input);
    0
}

destroy_fn!(destroy_fcmp_pp_prove_input, FcmpPpProveMembershipInput);

/// # Safety
///
/// This function assumes that the signable_tx_hash, x, and y are 32 bytes, sal_proof_out is
/// FCMP_PP_SAL_PROOF_SIZE_V1 bytes, and that the rerandomized output is returned from rerandomize_output().
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_prove_sal(
    signable_tx_hash: *const u8,
    x: *const u8,
    y: *const u8,
    rerandomized_output_bytes: *const u8,
    sal_proof_out: *mut u8,
    key_image_out: *mut u8,
) -> c_int {
    let Ok(signable_tx_hash) = hash_array_from_bytes(signable_tx_hash) else {
        return -1;
    };

    let mut rerandomized_output_bytes =
        core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    let Ok(rerandomized_output) = RerandomizedOutput::read(&mut rerandomized_output_bytes) else {
        return -2;
    };

    let Ok(x) = ed25519_scalar_from_bytes(x) else {
        return -3;
    };
    let Ok(y) = ed25519_scalar_from_bytes(y) else {
        return -4;
    };

    let Some(opening) = OpenedInputTuple::open(&rerandomized_output, &x, &y) else {
        return -5;
    };

    let (key_image, proof) = SpendAuthAndLinkability::prove(&mut OsRng, signable_tx_hash, &opening);

    let sal_proof_out = &mut core::slice::from_raw_parts_mut(sal_proof_out, 12 * 32); // @TODO: remove magic number
    let key_image_out = &mut core::slice::from_raw_parts_mut(key_image_out, 32);

    if proof.write(sal_proof_out).is_err() {
        return -6;
    }

    key_image_out.copy_from_slice(&key_image.to_bytes());

    0
}

/// # Safety
///
/// This function assumes inputs are a slice of FcmpPpProveMembershipInputSliceUnsafe, allocated via the FFI.
/// It also assumes fcmp_proof_out was allocated with size proof_len. proof_len should be the size of a
/// membership proof for the given number if inputs and tree layers.
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_prove_membership(
    inputs: FcmpPpProveMembershipInputSlice,
    n_tree_layers: usize,
    proof_len: usize,
    fcmp_proof_out: *mut u8,
) -> c_int {
    let inputs: &[*const FcmpPpProveMembershipInput] = inputs.into();
    debug_assert_eq!(
        proof_len,
        _slow_membership_proof_size(inputs.len(), n_tree_layers)
    );
    let mut buf_out = core::slice::from_raw_parts_mut(fcmp_proof_out, proof_len);

    match prove_membership_native(inputs, n_tree_layers) {
        Some(fcmp) => match fcmp.write(&mut buf_out) {
            Ok(_) => 0,
            Err(_) => -2,
        },
        None => -1,
    }
}

// These functions are slow as implemented. We use a table in fcmp_pp/proof_len.h
#[no_mangle]
pub extern "C" fn _slow_membership_proof_size(n_inputs: usize, n_tree_layers: usize) -> usize {
    Fcmp::<Curves>::proof_size(n_inputs, n_tree_layers)
}

#[no_mangle]
pub extern "C" fn _slow_fcmp_pp_proof_size(n_inputs: usize, n_tree_layers: usize) -> usize {
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
    debug_assert_eq!(proof_len, _slow_fcmp_pp_proof_size(n_inputs, n_tree_layers));

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
/// This function assumes that the signable tx hash is 32 bytes, the input is heap
/// allocated via a CResult, key_image is 32 bytes, and sal_proof is FCMP_PP_SAL_PROOF_SIZE_V1 bytes
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_verify_sal(
    signable_tx_hash: *const u8,
    input_bytes: *const u8,
    key_image: *const u8,
    sal_proof: *const u8,
) -> bool {
    let Ok(signable_tx_hash) = hash_array_from_bytes(signable_tx_hash) else {
        return false;
    };
    let input_bytes = core::slice::from_raw_parts(input_bytes, 4 * 32);
    let Ok(input) = input_from_bytes(input_bytes) else {
        return false;
    };
    let Ok(key_image) = ed25519_point_from_bytes(key_image) else {
        return false;
    };
    let Ok(sal_proof) =
        SpendAuthAndLinkability::read(&mut core::slice::from_raw_parts(sal_proof, 12 * 32))
    else {
        // @TODO: remove magic number
        return false;
    };

    let mut ed_verifier = multiexp::BatchVerifier::new(/*capacity: */ 1);

    sal_proof.verify(
        &mut OsRng,
        &mut ed_verifier,
        signable_tx_hash,
        &input,
        key_image,
    );

    ed_verifier.verify_vartime()
}

/// # Safety
///
/// This function assumes that each element of inputs is heap allocated via a
/// CResult, [fcmp_proof, fcmp_proof+fcmp_proof_len) is a valid readable range
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_verify_membership(
    inputs: Slice<[u8; 4 * 32]>,
    tree_root: *const TreeRoot<Selene, Helios>,
    n_tree_layers: usize,
    fcmp_proof: *const u8,
    fcmp_proof_len: usize,
) -> bool {
    let inputs: &[[u8; 4 * 32]] = inputs.into();
    let Ok(inputs) = inputs
        .iter()
        .map(|i| {
            let i = input_from_bytes(i.as_slice())?;
            fcmps::Input::new(i.O_tilde(), i.I_tilde(), i.R(), i.C_tilde())
        })
        .collect::<Result<Vec<_>, _>>()
    else {
        return false;
    };

    let mut fcmp_proof_buf = core::slice::from_raw_parts(fcmp_proof, fcmp_proof_len);
    let fcmp = match Fcmp::read(&mut fcmp_proof_buf, inputs.len(), n_tree_layers) {
        Ok(p) => p,
        Err(_) => return false,
    };

    let mut c1_verifier = generalized_bulletproofs::Generators::batch_verifier();
    let mut c2_verifier = generalized_bulletproofs::Generators::batch_verifier();

    match fcmp.verify(
        &mut OsRng,
        &mut c1_verifier,
        &mut c2_verifier,
        &*FCMP_PARAMS,
        *tree_root,
        n_tree_layers,
        &inputs,
    ) {
        Ok(()) => {
            SELENE_FCMP_GENERATORS.generators.verify(c1_verifier)
                && HELIOS_FCMP_GENERATORS.generators.verify(c2_verifier)
        }
        Err(_) => false,
    }
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

    // TODO: consider multithreading verify individual proofs, needs internal re-work
    for &input in inputs {
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

    // TODO: consider multithreading
    ed_verifier.verify_vartime()
        && SELENE_FCMP_GENERATORS.generators.verify(c1_verifier)
        && HELIOS_FCMP_GENERATORS.generators.verify(c2_verifier)
}

// https://github.com/rust-lang/rust/issues/79609
#[cfg(all(target_os = "windows", target_arch = "x86"))]
#[no_mangle]
pub extern "C" fn _Unwind_Resume() {}
