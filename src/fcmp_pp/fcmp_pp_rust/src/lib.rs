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
use full_chain_membership_proofs::tree::{hash_grow, hash_trim};

use monero_fcmp_plus_plus::{
    fcmps::{
        BranchBlind, Branches, CBlind, Fcmp, IBlind, IBlindBlind, OBlind, OutputBlinds, Path,
        TreeRoot,
    },
    sal::{OpenedInputTuple, RerandomizedOutput, SpendAuthAndLinkability},
    Curves, FcmpPlusPlus, Output, FCMP_PARAMS, HELIOS_GENERATORS, HELIOS_HASH_INIT,
    SELENE_GENERATORS, SELENE_HASH_INIT,
};

use monero_generators::{FCMP_U, FCMP_V, T};

// TODO: Use a macro to de-duplicate some of of this code
// TODO: Don't unwrap anywhere and populate the CResult error type in all fn's

//-------------------------------------------------------------------------------------- Curve points

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
        unsafe { core::slice::from_raw_parts(slice.buf, slice.len) }
    }
}
impl<T> From<&Slice<T>> for &[T] {
    fn from(slice: &Slice<T>) -> Self {
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
        // TODO: return defined error here: https://github.com/monero-project/monero/pull/9436#discussion_r1720477391
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
pub extern "C" fn rerandomize_output(output: OutputBytes) -> CResult<RerandomizedOutput, ()> {
    // TODO: CResult::err() on failure of output decompression
    // TODO: take in a de-compressed output
    let output = Output::new(
        ed25519_point_from_bytes(output.O),
        ed25519_point_from_bytes(output.I),
        ed25519_point_from_bytes(output.C),
    )
    .unwrap();

    let rerandomized_output = RerandomizedOutput::new(&mut OsRng, output);
    CResult::ok(rerandomized_output)
}

//---------------------------------------------- OBlind

/// # Safety
///
/// This function assumes that the rerandomized output being passed in input was
/// allocated on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn o_blind(output: *const RerandomizedOutput) -> CResult<Scalar, ()> {
    let rerandomized_output = unsafe { output.read() };
    CResult::ok(rerandomized_output.o_blind())
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
/// This function assumes that the rerandomized output being passed in input was
/// allocated on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn c_blind(output: *const RerandomizedOutput) -> CResult<Scalar, ()> {
    let rerandomized_output = unsafe { output.read() };
    CResult::ok(rerandomized_output.c_blind())
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
/// This function assumes that the rerandomized output being passed in input was
/// allocated on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn i_blind(output: *const RerandomizedOutput) -> CResult<Scalar, ()> {
    let rerandomized_output = unsafe { output.read() };
    CResult::ok(rerandomized_output.i_blind())
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
/// This function assumes that the rerandomized output being passed in input was
/// allocated on the heap and returned through a CResult instance.
#[no_mangle]
pub unsafe extern "C" fn i_blind_blind(output: *const RerandomizedOutput) -> CResult<Scalar, ()> {
    let rerandomized_output = unsafe { output.read() };
    CResult::ok(rerandomized_output.i_blind_blind())
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

#[no_mangle]
pub extern "C" fn selene_branch_blind() -> CResult<BranchBlind<<Selene as Ciphersuite>::G>, ()> {
    CResult::ok(BranchBlind::<<Selene as Ciphersuite>::G>::new(
        SELENE_GENERATORS().h(),
        ScalarDecomposition::new(<Selene as Ciphersuite>::F::random(&mut OsRng)).unwrap(),
    ))
}

//-------------------------------------------------------------------------------------- Fcmp

struct SalInput {
    x: Scalar,
    y: Scalar,
    rerandomized_output: RerandomizedOutput,
}

pub struct FcmpProveInput {
    sal_input: SalInput,
    path: Path<Curves>,
    output_blinds: OutputBlinds<EdwardsPoint>,
    c1_branch_blinds: Vec<BranchBlind<<Selene as Ciphersuite>::G>>,
    c2_branch_blinds: Vec<BranchBlind<<Helios as Ciphersuite>::G>>,
}

pub type FcmpProveInputSlice = Slice<*const FcmpProveInput>;

/// # Safety
///
/// This function assumes that the x and y are 32 byte Ed25519 scalars, that
/// RerandomizedOutput/Path/OutputBlinds were allocated on the heap
/// (via Box::into_raw(Box::new())), and the branch blinds are slices of
/// BranchBlind allocated on the heap (via CResult).
#[no_mangle]
pub unsafe extern "C" fn fcmp_prove_input_new(
    x: *const u8,
    y: *const u8,
    rerandomized_output: *const RerandomizedOutput,
    path: *const Path<Curves>,
    output_blinds: *const OutputBlinds<EdwardsPoint>,
    helios_branch_blinds: HeliosBranchBlindSlice,
    selene_branch_blinds: SeleneBranchBlindSlice,
) -> CResult<FcmpProveInput, ()> {
    // SAL
    let x = ed25519_scalar_from_bytes(x);
    let y = ed25519_scalar_from_bytes(y);
    let rerandomized_output = unsafe { rerandomized_output.read() };
    let sal_input = SalInput {
        x,
        y,
        rerandomized_output,
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

    let fcmp_prove_input = FcmpProveInput {
        sal_input,
        path,
        output_blinds,
        c1_branch_blinds,
        c2_branch_blinds,
    };
    CResult::ok(fcmp_prove_input)
}

/// # Safety
///
/// This function assumes that the x and y are 32 byte Ed25519 scalars, and that all other
/// types passed as input were allocated on the heap (via Box::into_raw(Box::new()))
#[no_mangle]
pub unsafe extern "C" fn prove(
    signable_tx_hash: *const u8,
    inputs: FcmpProveInputSlice,
    // TODO: tree_root is only used for verify, remove it
    tree_root: *const TreeRoot<Selene, Helios>,
) {
    let signable_tx_hash = unsafe { core::slice::from_raw_parts(signable_tx_hash, 32) };
    let signable_tx_hash: [u8; 32] = signable_tx_hash.try_into().unwrap();

    // Collect inputs into a vec
    let inputs: &[*const FcmpProveInput] = inputs.into();
    let inputs: Vec<FcmpProveInput> = inputs.iter().map(|x| unsafe { x.read() }).collect();

    // SAL proofs
    let mut key_images: Vec<EdwardsPoint> = vec![];
    let sal_proofs = inputs
        .iter()
        .map(|prove_input| {
            let sal_input = &prove_input.sal_input;
            let rerandomized_output = &sal_input.rerandomized_output;
            let x = sal_input.x;
            let y = sal_input.y;

            let input = rerandomized_output.input();
            let opening = OpenedInputTuple::open(rerandomized_output.clone(), &x, &y).unwrap();

            #[allow(non_snake_case)]
            let (L, spend_auth_and_linkability) =
                SpendAuthAndLinkability::prove(&mut OsRng, signable_tx_hash, opening);

            // assert_eq!(prove_input.path.output.O(), EdwardsPoint((*x * *EdwardsPoint::generator()) + (*y * *EdwardsPoint(T()))));
            assert_eq!(prove_input.path.output.I() * x, L);

            key_images.push(L);
            (input, spend_auth_and_linkability)
        })
        .collect();

    let paths = inputs.iter().map(|x| x.path.clone()).collect();
    let output_blinds = inputs.iter().map(|x| x.output_blinds.clone()).collect();

    let c1_branch_blinds: Vec<_> = inputs
        .iter()
        .map(|x| x.c1_branch_blinds.clone())
        .collect::<Vec<_>>()
        .into_iter()
        .flatten()
        .collect();
    let c2_branch_blinds: Vec<_> = inputs
        .iter()
        .map(|x| x.c2_branch_blinds.clone())
        .collect::<Vec<_>>()
        .into_iter()
        .flatten()
        .collect();

    let branches = Branches::new(paths).unwrap();

    assert_eq!(branches.necessary_c1_blinds(), c1_branch_blinds.len());
    assert_eq!(branches.necessary_c2_blinds(), c2_branch_blinds.len());
    let n_branch_blinds = (c1_branch_blinds.len() + c2_branch_blinds.len()) / inputs.len();

    let blinded_branches = branches
        .blind(output_blinds, c1_branch_blinds, c2_branch_blinds)
        .unwrap();

    // Membership proof
    let fcmp = Fcmp::prove(&mut OsRng, FCMP_PARAMS(), blinded_branches).unwrap();

    // Combine SAL proofs and membership proof
    let fcmp_plus_plus = FcmpPlusPlus::new(sal_proofs, fcmp);

    // let mut buf = vec![];
    // fcmp_plus_plus.write(&mut buf).unwrap();

    let mut ed_verifier = multiexp::BatchVerifier::new(1);
    let mut c1_verifier = generalized_bulletproofs::Generators::batch_verifier();
    let mut c2_verifier = generalized_bulletproofs::Generators::batch_verifier();

    let tree_root: TreeRoot<Selene, Helios> = unsafe { tree_root.read() };

    let n_layers = 1 + n_branch_blinds;

    // TODO: remove verify
    fcmp_plus_plus
        .verify(
            &mut OsRng,
            &mut ed_verifier,
            &mut c1_verifier,
            &mut c2_verifier,
            tree_root,
            n_layers,
            signable_tx_hash,
            key_images,
        )
        .unwrap();

    assert!(ed_verifier.verify_vartime());
    assert!(SELENE_GENERATORS().verify(c1_verifier));
    assert!(HELIOS_GENERATORS().verify(c2_verifier));
}

// https://github.com/rust-lang/rust/issues/79609
#[cfg(all(target_os = "windows", target_arch = "x86"))]
#[no_mangle]
pub extern "C" fn _Unwind_Resume() {}
