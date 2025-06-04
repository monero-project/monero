use rand_core::OsRng;

use ciphersuite::{
    group::{
        ff::{Field, PrimeField},
        Group, GroupEncoding,
    },
    Ciphersuite, Ed25519, Helios, Selene,
};
use dalek_ff_group::{EdwardsPoint, Scalar};
use helioselene::{
    Field25519 as SeleneScalar, HeliosPoint, HelioseleneField as HeliosScalar, SelenePoint,
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
    Curves, FcmpPlusPlus, Input, Output, FCMP_PARAMS, HELIOS_GENERATORS, HELIOS_HASH_INIT,
    SELENE_GENERATORS, SELENE_HASH_INIT,
};

use monero_generators::{FCMP_U, FCMP_V, T};

use std::os::raw::c_int;

// TODO: everything allocated with Box::into_raw needs to be freed manually using from_raw when done
// https://doc.rust-lang.org/std/boxed/struct.Box.html#method.from_raw
// https://internals.rust-lang.org/t/manually-freeing-the-pointer-from-into-raw/19002
// Current idea is to use C++ types initalized with a raw pointer from a fn over the FFI, and make destructors for each
// type that call the respective destructor for that raw pointer (which calls Box::from_raw over the FFI to clean up)
// TODO: don't use CResult for types the FFI supports (e.g. SelenePoint, SeleneScalar), don't want the raw * because need to manually from_raw it to dealloc
// TODO: Use a macro to de-duplicate some of of this code
// TODO: Don't unwrap anywhere and populate the CResult error type in all fn's
// TODO: repalce asserts with error returns where reasonable

//-------------------------------------------------------------------------------------- Curve points

#[no_mangle]
pub extern "C" fn helios_hash_init_point() -> HeliosPoint {
    HELIOS_HASH_INIT()
}

#[no_mangle]
pub extern "C" fn selene_hash_init_point() -> SelenePoint {
    SELENE_HASH_INIT()
}

fn new_box_raw<T>(obj: T) -> *mut T {
    Box::into_raw(Box::new(obj))
}

// https://doc.rust-lang.org/std/boxed/struct.Box.html#method.from_raw
fn free_box<T>(ptr: *mut T) {
    let _ = unsafe { Box::from_raw(ptr) };
}

fn c_u8_32(bytes: [u8; 32]) -> *const u8 {
    let arr_ptr = Box::into_raw(Box::new(bytes));
    arr_ptr as *const u8
}

#[no_mangle]
pub unsafe extern "C" fn helios_scalar_to_bytes(helios_scalar: *const HeliosScalar, bytes_out: *mut u8) {
    let bytes_out = core::slice::from_raw_parts_mut(bytes_out, 32);
    bytes_out.clone_from_slice(&(&*helios_scalar).to_repr());
}

#[no_mangle]
pub unsafe extern "C" fn selene_scalar_to_bytes(selene_scalar: *const SeleneScalar, bytes_out: *mut u8) {
    let bytes_out = core::slice::from_raw_parts_mut(bytes_out, 32);
    bytes_out.clone_from_slice(&(&*selene_scalar).to_repr());
}

#[no_mangle]
pub unsafe extern "C" fn helios_point_to_bytes(helios_point: *const HeliosPoint, bytes_out: *mut u8) {
    let bytes_out = core::slice::from_raw_parts_mut(bytes_out, 32);
    bytes_out.clone_from_slice(&(&*helios_point).to_bytes());
}

#[no_mangle]
pub unsafe extern "C" fn selene_point_to_bytes(selene_point: *const SelenePoint, bytes_out: *mut u8) {
    let bytes_out = core::slice::from_raw_parts_mut(bytes_out, 32);
    bytes_out.clone_from_slice(&(&*selene_point).to_bytes());
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

// Undefined behavior occurs when the data pointer passed to core::slice::from_raw_parts is null,
// even when len is 0. slice_from_raw_parts_0able() lets you pass p as null, as long as len is 0
const unsafe fn slice_from_raw_parts_0able<'a, T>(p: *const T, len: usize) -> &'a [T] {
    if len == 0 {
        &[]
    } else {
        core::slice::from_raw_parts(p, len)
    }
}

fn ed25519_point_from_bytes(ed25519_point: *const u8) -> EdwardsPoint {
    let mut ed25519_point = unsafe { core::slice::from_raw_parts(ed25519_point, 32) };
    // TODO: Return an error here (instead of unwrapping)
    <Ed25519>::read_G(&mut ed25519_point).unwrap()
}

fn ed25519_scalar_from_bytes(ed25519_scalar: *const u8) -> Scalar {
    let mut ed25519_scalar = unsafe { core::slice::from_raw_parts(ed25519_scalar, 32) };
    // TODO: Return an error here (instead of unwrapping)
    <Ed25519>::read_F(&mut ed25519_scalar).unwrap()
}

fn hash_array_from_bytes(h: *const u8) -> [u8; 32] {
    unsafe { core::slice::from_raw_parts(h, 32) }.try_into().unwrap()
}

// @TODO: this is horrible :(, expose direct read/write for Input in the fcmp-plus-plus crate
fn input_from_bytes(input_bytes: &[u8]) -> std::io::Result<Input> {
    let mut rerandomized_output_bytes = [0u8; 8 * 32];
    if input_bytes.len() != 4 * 32 {
        return Err(std::io::Error::new(std::io::ErrorKind::InvalidInput, "passed input slice wrong size"));
    }
    rerandomized_output_bytes[0..4*32].copy_from_slice(input_bytes);
    let rerandomized_output = RerandomizedOutput::read(&mut rerandomized_output_bytes.as_slice())?;
    Ok(rerandomized_output.input())
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

#[no_mangle]
pub extern "C" fn selene_tree_root(selene_point: SelenePoint) -> *const u8 {
    Box::into_raw(Box::new(TreeRoot::<Selene, Helios>::C1(selene_point))) as *const u8
}

#[no_mangle]
pub extern "C" fn helios_tree_root(helios_point: HeliosPoint) -> *const u8 {
    Box::into_raw(Box::new(TreeRoot::<Selene, Helios>::C2(helios_point))) as *const u8
}

#[allow(non_snake_case)]
#[repr(C)]
pub struct OutputBytes {
    O: *const u8,
    I: *const u8,
    C: *const u8,
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
        // TODO: return defined error here: https://github.com/monero-project/monero/pull/9436#discussion_r1720477391
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
        // TODO: return defined error here: https://github.com/monero-project/monero/pull/9436#discussion_r1720477391
        CResult::err(())
    }
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
) -> CResult<Path<Curves>, ()> {
    // Collect decompressed leaves
    let leaves: &[OutputBytes] = leaves.into();
    let leaves: Vec<Output> = leaves
        .iter()
        .map(|x| {
            // TODO: don't unwrap, error
            Output::new(
                ed25519_point_from_bytes(x.O),
                ed25519_point_from_bytes(x.I),
                ed25519_point_from_bytes(x.C),
            )
            .unwrap()
        })
        .collect();

    // Output
    if output_idx >= leaves.len() {
        // TODO: return error instead of panic
        panic!("output_idx is too high");
    }
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
    CResult::ok(path)
}

//-------------------------------------------------------------------------------------- Blindings

//---------------------------------------------- RerandomizedOutput

#[no_mangle]
pub unsafe extern "C" fn rerandomize_output(output: OutputBytes,
    rerandomized_output_bytes: *mut u8
) -> c_int {
    let Ok(output) = Output::new(
        ed25519_point_from_bytes(output.O),
        ed25519_point_from_bytes(output.I),
        ed25519_point_from_bytes(output.C),
    ) else { return -1 };

    let mut rerandomized_output_bytes = core::slice::from_raw_parts_mut(rerandomized_output_bytes, 8 * 32);

    let rerandomized_output = RerandomizedOutput::new(&mut OsRng, output);
    if rerandomized_output.write(&mut rerandomized_output_bytes).is_err() { -1 } else { 0 }
}

//---------------------------------------------- OBlind

/// # Safety
///
/// This function assumes that the rerandomized output byte buffer being passed
/// in is 8*32 bytes.
#[no_mangle]
pub unsafe extern "C" fn o_blind(rerandomized_output_bytes: *const u8,
    o_blind: *mut Scalar
) -> c_int {
    let mut rerandomized_output_bytes = core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    match RerandomizedOutput::read(&mut rerandomized_output_bytes) {
        Ok(rerandomized_output) => { o_blind.write(rerandomized_output.o_blind()); 0 }
        Err(_) => -1
    }
}

/// # Safety
///
/// This function assumes that the scalar being passed in input was
/// allocated on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn blind_o_blind(
    o_blind: *const Scalar,
) -> CResult<OBlind<EdwardsPoint>, ()> {
    if let Some(o_blind) = ScalarDecomposition::new(o_blind.read()) {
        let blind = OBlind::new(EdwardsPoint(T()), o_blind);
        CResult::ok(blind)
    } else {
        CResult::err(())
    }
}

//---------------------------------------------- CBlind

/// # Safety
///
/// This function assumes that the rerandomized output byte buffer being passed
/// in is 8*32 bytes.
#[no_mangle]
pub unsafe extern "C" fn c_blind(rerandomized_output_bytes: *const u8,
    c_blind: *mut Scalar
) -> c_int {
    let mut rerandomized_output_bytes = core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    match RerandomizedOutput::read(&mut rerandomized_output_bytes) {
        Ok(rerandomized_output) => { c_blind.write(rerandomized_output.c_blind()); 0 }
        Err(_) => -1
    }
}

/// # Safety
///
/// This function assumes that the scalar being passed in input is from
/// allocated on the heap has been returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn blind_c_blind(
    c_blind: *const Scalar,
) -> CResult<CBlind<EdwardsPoint>, ()> {
    if let Some(c_blind) = ScalarDecomposition::new(c_blind.read()) {
        let blind = CBlind::new(EdwardsPoint::generator(), c_blind);
        CResult::ok(blind)
    } else {
        CResult::err(())
    }
}

//---------------------------------------------- IBlind

/// # Safety
///
/// This function assumes that the rerandomized output byte buffer being passed
/// in is 8*32 bytes.
#[no_mangle]
pub unsafe extern "C" fn i_blind(rerandomized_output_bytes: *const u8,
    i_blind: *mut Scalar
) -> c_int {
    let mut rerandomized_output_bytes = core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    match RerandomizedOutput::read(&mut rerandomized_output_bytes) {
        Ok(rerandomized_output) => { i_blind.write(rerandomized_output.i_blind()); 0 }
        Err(_) => -1
    }
}

/// # Safety
///
/// This function assumes that the scalar being passed in input is from
/// allocated on the heap has been returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn blind_i_blind(
    i_blind: *const Scalar,
) -> CResult<IBlind<EdwardsPoint>, ()> {
    if let Some(i_blind) = ScalarDecomposition::new(i_blind.read()) {
        let blind = IBlind::new(EdwardsPoint(FCMP_U()), EdwardsPoint(FCMP_V()), i_blind);
        CResult::ok(blind)
    } else {
        CResult::err(())
    }
}

//---------------------------------------------- IBlindBlind

/// # Safety
///
/// This function assumes that the rerandomized output byte buffer being passed
/// in is 8*32 bytes.
#[no_mangle]
pub unsafe extern "C" fn i_blind_blind(rerandomized_output_bytes: *const u8,
    i_blind_blind: *mut Scalar
) -> c_int {
    let mut rerandomized_output_bytes = core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    match RerandomizedOutput::read(&mut rerandomized_output_bytes) {
        Ok(rerandomized_output) => { i_blind_blind.write(rerandomized_output.i_blind_blind()); 0 }
        Err(_) => -1
    }
}

/// # Safety
///
/// This function assumes that the scalar being passed in input is from
/// allocated on the heap has been returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn blind_i_blind_blind(
    i_blind_blind: *const Scalar,
) -> CResult<IBlindBlind<EdwardsPoint>, ()> {
    if let Some(i_blind_blind) = ScalarDecomposition::new(i_blind_blind.read()) {
        let blind = IBlindBlind::new(EdwardsPoint(T()), i_blind_blind);
        CResult::ok(blind)
    } else {
        CResult::err(())
    }
}

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
) -> CResult<OutputBlinds<EdwardsPoint>, ()> {
    CResult::ok(OutputBlinds::new(
        o_blind.read(),
        i_blind.read(),
        i_blind_blind.read(),
        c_blind.read(),
    ))
}

//---------------------------------------------- BranchBlind

#[no_mangle]
pub extern "C" fn helios_branch_blind() -> CResult<BranchBlind<<Helios as Ciphersuite>::G>, ()> {
    CResult::ok(BranchBlind::<<Helios as Ciphersuite>::G>::new(
        HELIOS_GENERATORS().h(),
        ScalarDecomposition::new(<Helios as Ciphersuite>::F::random(&mut OsRng)).unwrap(),
    ))
}

/// # Safety
///
/// This function allocates a branch blind on the heap via Box::new, then
/// gets its raw pointer via Box::into_raw, then sets the input pointer's
/// address to point to the raw box pointer. The input parameter must not
/// be null. Also be sure to clean up the branch blind with
/// free_helios_branch_blind below.
#[no_mangle]
pub unsafe extern "C" fn new_helios_branch_blind(
    branch_blind_out: *mut *mut BranchBlind::<<Helios as Ciphersuite>::G>,
) -> c_int {
    let scalar_decomp = ScalarDecomposition::new(<Helios as Ciphersuite>::F::random(&mut OsRng));
    let Some(scalar_decomp) = scalar_decomp else {
        return -1;
    };

    let branch_blind =
        BranchBlind::<<Helios as Ciphersuite>::G>::new(HELIOS_GENERATORS().h(), scalar_decomp);

    unsafe { *branch_blind_out = new_box_raw(branch_blind) };

    0
}

/// # Safety
///
/// This function assumes that branch_blind was allocated on the heap via
/// Box::into_raw(Box::new())
#[no_mangle]
pub unsafe extern "C" fn free_helios_branch_blind(
    branch_blind: *mut BranchBlind<<Helios as Ciphersuite>::G>,
) {
    free_box(branch_blind);
}

#[no_mangle]
pub extern "C" fn selene_branch_blind() -> CResult<BranchBlind<<Selene as Ciphersuite>::G>, ()> {
    CResult::ok(BranchBlind::<<Selene as Ciphersuite>::G>::new(
        SELENE_GENERATORS().h(),
        ScalarDecomposition::new(<Selene as Ciphersuite>::F::random(&mut OsRng)).unwrap(),
    ))
}

//-------------------------------------------------------------------------------------- Fcmp

#[derive(Clone)]
pub struct FcmpPpProveInput {
    rerandomized_output: RerandomizedOutput,

    // FCMP-specific
    path: Path<Curves>,
    output_blinds: OutputBlinds<EdwardsPoint>,
    c1_branch_blinds: Vec<BranchBlind<<Selene as Ciphersuite>::G>>,
    c2_branch_blinds: Vec<BranchBlind<<Helios as Ciphersuite>::G>>,

    // SA/L-specific, ignore when only doing membership proofs
    x: Scalar,
    y: Scalar
}

pub type FcmpPpProveInputSlice = Slice<*const FcmpPpProveInput>;

unsafe fn prove_membership_native(inputs: &[*const FcmpPpProveInput], n_tree_layers: usize) -> Option<Fcmp<Curves>> {
    let paths = inputs.iter().cloned().map(|x| (*x).path.clone()).collect();
    let output_blinds = inputs.iter().cloned().map(|x| (*x).output_blinds.clone()).collect();

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

    assert_eq!(branches.necessary_c1_blinds(), c1_branch_blinds.len());
    assert_eq!(branches.necessary_c2_blinds(), c2_branch_blinds.len());

    let n_branch_blinds = (c1_branch_blinds.len() + c2_branch_blinds.len()) / inputs.len();
    assert_eq!(n_tree_layers, n_branch_blinds + 1);

    let blinded_branches = branches
        .blind(output_blinds, c1_branch_blinds, c2_branch_blinds)
        .ok()?;

    Fcmp::prove(&mut OsRng, FCMP_PARAMS(), blinded_branches).ok()
}

/// # Safety
///
/// This function assumes that RerandomizedOutput/Path/OutputBlinds were
/// allocated on the heap (via Box::into_raw(Box::new())), and the branch
/// blinds are slices of BranchBlind allocated on the heap (via CResult).
#[no_mangle]
pub unsafe extern "C" fn fcmp_prove_input_new(rerandomized_output_bytes: *const u8,
    path: *const Path<Curves>,
    output_blinds: *const OutputBlinds<EdwardsPoint>,
    selene_branch_blinds: SeleneBranchBlindSlice,
    helios_branch_blinds: HeliosBranchBlindSlice 
) -> CResult<FcmpPpProveInput, ()> {
    let zero = [0u8; 32];
    let pzero = &zero as *const u8;
    fcmp_pp_prove_input_new(pzero,
        pzero,
        rerandomized_output_bytes,
        path,
        output_blinds,
        selene_branch_blinds,
        helios_branch_blinds)
}

/// # Safety
///
/// This function assumes that the x and y are 32 byte Ed25519 scalars, that
/// RerandomizedOutput/Path/OutputBlinds were allocated on the heap
/// (via Box::into_raw(Box::new())), and the branch blinds are slices of
/// BranchBlind allocated on the heap (via CResult).
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_prove_input_new(
    x: *const u8,
    y: *const u8,
    rerandomized_output_bytes: *const u8,
    path: *const Path<Curves>,
    output_blinds: *const OutputBlinds<EdwardsPoint>,
    selene_branch_blinds: SeleneBranchBlindSlice,
    helios_branch_blinds: HeliosBranchBlindSlice,
) -> CResult<FcmpPpProveInput, ()> {
    // SAL
    let x = ed25519_scalar_from_bytes(x);
    let y = ed25519_scalar_from_bytes(y);
    let mut rerandomized_output_bytes = core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    let Ok(rerandomized_output) = RerandomizedOutput::read(&mut rerandomized_output_bytes) else {
        return CResult::err(());
    };

    // Path and output blinds
    let path = unsafe { path.read() };
    let output_blinds = unsafe { output_blinds.read() };

    // Collect branch blinds
    let c1_branch_blinds: &[*const BranchBlind<<Selene as Ciphersuite>::G>] =
        selene_branch_blinds.into();
    let c1_branch_blinds: Vec<BranchBlind<<Selene as Ciphersuite>::G>> = c1_branch_blinds
        .iter()
        .map(|x| unsafe { (*x.to_owned()).clone() })
        .collect();

    let c2_branch_blinds: &[*const BranchBlind<<Helios as Ciphersuite>::G>] =
        helios_branch_blinds.into();
    let c2_branch_blinds: Vec<BranchBlind<<Helios as Ciphersuite>::G>> = c2_branch_blinds
        .iter()
        .map(|x| unsafe { (*x.to_owned()).clone() })
        .collect();

    let fcmp_prove_input = FcmpPpProveInput {
        rerandomized_output,
        path,
        output_blinds,
        c1_branch_blinds,
        c2_branch_blinds,
        x,
        y
    };
    CResult::ok(fcmp_prove_input)
}

/// # Safety
///
/// This function assumes that the sum_input_masks and sum_output_masks are 32 byte Ed25519 scalars,
/// and that the inputs are a slice of inputs returned from fcmp_prove_input_new.
#[no_mangle]
pub unsafe extern "C" fn balance_last_pseudo_out(
    sum_input_masks: *const u8,
    sum_output_masks: *const u8,
    inputs: FcmpPpProveInputSlice,
) -> CResult<FcmpPpProveInput, ()> {
    let mut sum_input_masks = ed25519_scalar_from_bytes(sum_input_masks);
    let sum_output_masks = ed25519_scalar_from_bytes(sum_output_masks);

    let inputs: &[*const FcmpPpProveInput] = inputs.into();

    if inputs.len() == 0 {
        return CResult::err(());
    }

    // Get the sum of the input commitment masks excluding the last one
    for i in 0..inputs.len() - 1 {
        // Read the input without consuming it
        let input: &FcmpPpProveInput = unsafe { &*inputs[i] };
        sum_input_masks += input.rerandomized_output.r_c();
    }

    // Re-calculate the last scalar so that the sum of inputs == sum of outputs
    let new_last_r_c = sum_output_masks - sum_input_masks;

    // Update the last r_c on the last reandomized output, which also updates its C_tilde
    // TODO: modify the inputs slice directly
    let mut last_input = inputs.last().unwrap().read();
    let last_commitment = last_input.path.output.C();
    last_input.rerandomized_output.set_r_c(last_commitment, new_last_r_c);

    // Blind the updated C blind (this is expensive)
    let new_c_blind = ScalarDecomposition::new(last_input.rerandomized_output.c_blind()).unwrap();
    let new_blinded_c_blind = CBlind::new(EdwardsPoint::generator(), new_c_blind);
    last_input.output_blinds.set_c_blind(new_blinded_c_blind);

    CResult::ok(last_input)
}

/// # Safety
///
/// This function assumes that the inputs are a slice of inputs returned from fcmp_prove_input_new.
#[no_mangle]
pub unsafe extern "C" fn read_input_pseudo_out(
    input: *const FcmpPpProveInput,
) -> *const u8 {
    // Read the input without consuming it
    let input: &FcmpPpProveInput = unsafe { &*input };
    c_u8_32(input.rerandomized_output.input().C_tilde().to_bytes())
}

/// # Safety
///
/// This function assumes that the signable_tx_hash is 32 bytes and that the inputs are a slice
/// of inputs returned from fcmp_pp_prove_input_new.
#[no_mangle]
pub unsafe extern "C" fn prove(
    signable_tx_hash: *const u8,
    inputs: FcmpPpProveInputSlice,
    n_tree_layers: usize,
) -> CResult<*const u8, ()> {
    let signable_tx_hash = unsafe { core::slice::from_raw_parts(signable_tx_hash, 32) };
    let signable_tx_hash: [u8; 32] = signable_tx_hash.try_into().unwrap();

    let inputs: &[*const FcmpPpProveInput] = inputs.into();

    // SAL proofs
    let sal_proofs = inputs
        .iter()
        .cloned()
        .map(|prove_input| {
            let rerandomized_output = &(*prove_input).rerandomized_output;
            let x = &(*prove_input).x;
            let y = &(*prove_input).y;

            assert_eq!(
                (*prove_input).path.output.O(),
                EdwardsPoint((**x * *EdwardsPoint::generator()) + (**y * *EdwardsPoint(T())))
            );

            let input = rerandomized_output.input();
            let opening = OpenedInputTuple::open(rerandomized_output.clone(), &x, &y).unwrap();

            let (key_image, spend_auth_and_linkability) =
                SpendAuthAndLinkability::prove(&mut OsRng, signable_tx_hash, opening);

            assert_eq!((*prove_input).path.output.I() * x, key_image);

            (input, spend_auth_and_linkability)
        })
        .collect();

    // Membership proof
    let fcmp = match prove_membership_native(inputs, n_tree_layers) {
        Some(x) => x,
        None => return CResult::err(())
    };

    // Combine SAL proofs and membership proof
    let fcmp_plus_plus = FcmpPlusPlus::new(sal_proofs, fcmp);

    let mut buf = vec![];
    fcmp_plus_plus.write(&mut buf).unwrap();
    debug_assert_eq!(buf.len(), _slow_fcmp_pp_proof_size(inputs.len(), n_tree_layers));

    // Leak the buf into a ptr that the C++ can handle
    // TODO: Use Box::leak instead, and then in destructor convert back to box https://doc.rust-lang.org/std/boxed/struct.Box.html#method.leak
    let ptr = buf.leak().as_ptr();
    CResult::ok(ptr)
}

/// # Safety
///
/// This function assumes that the signable_tx_hash, x, and y are 32 bytes, sal_proof_out is
/// FCMP_PP_SAL_PROOF_SIZE_V1 bytes, and that the rerandomized output is returned from rerandomize_output().
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_prove_sal(signable_tx_hash: *const u8,
    x: *const u8,
    y: *const u8,
    rerandomized_output_bytes: *const u8,
    sal_proof_out: *mut u8,
    key_image_out: *mut u8
) -> c_int {
    let signable_tx_hash = hash_array_from_bytes(signable_tx_hash);

    let mut rerandomized_output_bytes = core::slice::from_raw_parts(rerandomized_output_bytes, 8 * 32);
    let Ok(rerandomized_output) = RerandomizedOutput::read(&mut rerandomized_output_bytes) else {
        return -1;
    };

    let Some(opening) = OpenedInputTuple::open(rerandomized_output,
        &ed25519_scalar_from_bytes(x),
        &ed25519_scalar_from_bytes(y)
    ) else {
        return -2;
    };

    let (key_image, proof) = SpendAuthAndLinkability::prove(&mut OsRng,
        signable_tx_hash,
        opening);

    let sal_proof_out = &mut core::slice::from_raw_parts_mut(sal_proof_out, 12*32); // @TODO: remove magic number
    let key_image_out = &mut core::slice::from_raw_parts_mut(key_image_out, 32);

    if let Err(_) = proof.write(sal_proof_out) {
        return -3;
    }

    key_image_out.copy_from_slice(&key_image.to_bytes());

    0
}

#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_prove_membership(inputs: FcmpPpProveInputSlice,
    n_tree_layers: usize,
    proof_len: usize,
    fcmp_proof_out: *mut u8,
    fcmp_proof_out_len: *mut usize
) -> CResult<(), ()> {
    let inputs: &[*const FcmpPpProveInput] = inputs.into();
    let capacity = fcmp_proof_out_len.read();
    debug_assert_eq!(proof_len, _slow_membership_proof_size(inputs.len(), n_tree_layers));
    if capacity < proof_len {
        return CResult::err(())
    }
    fcmp_proof_out_len.write(proof_len);
    let mut buf_out = core::slice::from_raw_parts_mut(fcmp_proof_out, proof_len);

    match prove_membership_native(inputs, n_tree_layers) {
        Some(fcmp) => match fcmp.write(&mut buf_out) {
            Ok(_) => CResult::ok(()),
            Err(_) => CResult::err(())
        },
        None => CResult::err(())
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

pub struct FcmpPpVerifyInput
{
    fcmp_pp: FcmpPlusPlus,
    tree_root: TreeRoot<Selene, Helios>,
    n_tree_layers: usize,
    signable_tx_hash: [u8; 32],
    key_images: Vec<EdwardsPoint>,
}

unsafe fn fcmp_pp_verify_input_new_inner(
    signable_tx_hash: *const u8,
    proof: *const u8,
    proof_len: usize,
    n_tree_layers: usize,
    tree_root: *const TreeRoot<Selene, Helios>,
    pseudo_outs: Slice<*const u8>,
    key_images: Slice<*const u8>,
) -> std::io::Result<FcmpPpVerifyInput> {
    // Early checks
    let n_inputs = pseudo_outs.len;
    if n_inputs == 0 {
        return Err(std::io::Error::new(std::io::ErrorKind::InvalidInput, "passed in 0 inputs"));
    }
    if n_inputs != key_images.len {
        return Err(std::io::Error::new(std::io::ErrorKind::InvalidInput, "passed invalid number of inputs"));
    }
    debug_assert_eq!(proof_len, _slow_fcmp_pp_proof_size(n_inputs, n_tree_layers));

    let signable_tx_hash = unsafe { core::slice::from_raw_parts(signable_tx_hash, 32) };
    let signable_tx_hash: [u8; 32] = signable_tx_hash.try_into().unwrap();

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
    let fcmp_plus_plus = FcmpPlusPlus::read(&pseudo_outs, n_tree_layers, &mut proof).unwrap();

    let tree_root: TreeRoot<Selene, Helios> = unsafe { tree_root.read() };

    // Collect de-compressed key images into a Vec
    let key_images: &[*const u8] = key_images.into();
    let key_images: Vec<EdwardsPoint> = key_images
        .iter()
        .map(|&x| ed25519_point_from_bytes(x))
        .collect();

    let fcmp_pp_verify_input = FcmpPpVerifyInput {
        fcmp_pp: fcmp_plus_plus,
        tree_root,
        n_tree_layers,
        signable_tx_hash,
        key_images,
    };
    Ok(fcmp_pp_verify_input)
}

/// # Safety
///
/// This function assumes that the signable tx hash is 32 bytes, the tree root is heap
/// allocated via a CResult, and pseudo outs and key images are 32 bytes each
#[no_mangle]
pub unsafe extern "C" fn verify(
    signable_tx_hash: *const u8,
    proof: *const u8,
    proof_len: usize,
    n_tree_layers: usize,
    tree_root: *const TreeRoot<Selene, Helios>,
    pseudo_outs: Slice<*const u8>,
    key_images: Slice<*const u8>,
) -> bool {
    let Ok(fcmp_pp_verify_input) = fcmp_pp_verify_input_new_inner(
        signable_tx_hash,
        proof,
        proof_len,
        n_tree_layers,
        tree_root,
        pseudo_outs,
        key_images
    ) else {
        return false;
    };

    let n_inputs = fcmp_pp_verify_input.key_images.len();

    let mut ed_verifier = multiexp::BatchVerifier::new(n_inputs);
    let mut c1_verifier = generalized_bulletproofs::Generators::batch_verifier();
    let mut c2_verifier = generalized_bulletproofs::Generators::batch_verifier();

    match fcmp_pp_verify_input.fcmp_pp
        .verify(
            &mut OsRng,
            &mut ed_verifier,
            &mut c1_verifier,
            &mut c2_verifier,
            fcmp_pp_verify_input.tree_root,
            fcmp_pp_verify_input.n_tree_layers,
            fcmp_pp_verify_input.signable_tx_hash,
            fcmp_pp_verify_input.key_images,
        )
    {
        Ok(()) => ed_verifier.verify_vartime()
            && SELENE_GENERATORS().verify(c1_verifier)
            && HELIOS_GENERATORS().verify(c2_verifier),
        Err(_) => false
    }    
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
) -> CResult<FcmpPpVerifyInput, ()> {
    match fcmp_pp_verify_input_new_inner(
        signable_tx_hash,
        proof,
        proof_len,
        n_tree_layers,
        tree_root,
        pseudo_outs,
        key_images
    )
    {
        Ok(res) => CResult::ok(res),
        Err(_) => CResult::err(())
    }
}

/// # Safety
///
/// This function assumes that the signable tx hash is 32 bytes, the input is heap
/// allocated via a CResult, key_image is 32 bytes, and sal_proof is FCMP_PP_SAL_PROOF_SIZE_V1 bytes
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_verify_sal(signable_tx_hash: *const u8,
    input_bytes: *const u8,
    key_image: *const u8,
    sal_proof: *const u8
) -> bool {
    let signable_tx_hash = hash_array_from_bytes(signable_tx_hash);
    let input_bytes = core::slice::from_raw_parts(input_bytes, 4 * 32);
    let Ok(input) = input_from_bytes(input_bytes) else {
        return false;
    };
    let key_image = ed25519_point_from_bytes(key_image);
    let Ok(sal_proof) = SpendAuthAndLinkability::read(&mut core::slice::from_raw_parts(sal_proof, 12*32)) else { // @TODO: remove magic number
        return false;
    };

    let mut ed_verifier = multiexp::BatchVerifier::new(/*capacity: */1);

    sal_proof.verify(&mut OsRng, &mut ed_verifier, signable_tx_hash, &input, key_image);

    ed_verifier.verify_vartime()
}

/// # Safety
///
/// This function assumes that each element of inputs is heap allocated via a
/// CResult, [fcmp_proof, fcmp_proof+fcmp_proof_len) is a valid readable range
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_verify_membership(inputs: Slice<[u8; 4 * 32]>,
    tree_root: *const TreeRoot<Selene, Helios>,
    n_tree_layers: usize,
    fcmp_proof: *const u8,
    fcmp_proof_len: usize
) -> bool {
    let inputs: &[[u8; 4 * 32]] = inputs.into();
    let Ok(inputs) = inputs
        .iter()
        .map(|i| { let i = input_from_bytes(&mut i.as_slice())?; fcmps::Input::new(i.O_tilde(), i.I_tilde(), i.R(), i.C_tilde())})
        .collect::<Result<Vec<_>, _>>()
        else { return false };

    let tree_root = tree_root.read();

    let mut fcmp_proof_buf = core::slice::from_raw_parts(fcmp_proof, fcmp_proof_len);
    let fcmp = match Fcmp::read(&mut fcmp_proof_buf, inputs.len(), n_tree_layers) {
        Ok(p) => p,
        Err(_) => return false
    };

    let mut c1_verifier = generalized_bulletproofs::Generators::batch_verifier();
    let mut c2_verifier = generalized_bulletproofs::Generators::batch_verifier();

    match fcmp
        .verify(
            &mut OsRng,
            &mut c1_verifier,
            &mut c2_verifier,
            FCMP_PARAMS(),
            tree_root,
            n_tree_layers,
            &inputs
        )
    {
        Ok(()) => SELENE_GENERATORS().verify(c1_verifier)
            && HELIOS_GENERATORS().verify(c2_verifier),
        Err(_) => false
    }
}

/// # Safety
///
/// This function assumes that the inputs are from fcmp_pp_verify_input_new
#[no_mangle]
pub unsafe extern "C" fn fcmp_pp_batch_verify(inputs: Slice<*const FcmpPpVerifyInput>) -> bool {
    let inputs: &[*const FcmpPpVerifyInput] = inputs.into();
    let inputs: Vec<FcmpPpVerifyInput> = inputs.iter().map(|x| unsafe { x.read() }).collect();

    let mut ed_verifier = multiexp::BatchVerifier::new(inputs.len());
    let mut c1_verifier = generalized_bulletproofs::Generators::batch_verifier();
    let mut c2_verifier = generalized_bulletproofs::Generators::batch_verifier();

    // TODO: consider multithreading verify individual proofs, needs internal re-work
    for i in 0..inputs.len() {
        let fcmp_pp_verify_input = &inputs[i];

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
        && SELENE_GENERATORS().verify(c1_verifier)
        && HELIOS_GENERATORS().verify(c2_verifier)
}

// https://github.com/rust-lang/rust/issues/79609
#[cfg(all(target_os = "windows", target_arch = "x86"))]
#[no_mangle]
pub extern "C" fn _Unwind_Resume() {}
