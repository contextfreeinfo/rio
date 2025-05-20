use crate::lex::Intern;
use crate::refine::resolve::UidInfo;
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::Arc;

enum Value {
    // Immediate values for speed?
    Claim(bool),
    Int(isize),
    Float(f64),
    /// Intern would be risky since strings can be generated in loops.
    Function(UidInfo),
    Ref(Ref),
    Typ(Arc<Typ>),
}

enum Typ {
    Array(ArrayTyp),
    Bound(BoundTyp),
    Class(ClassTyp),
    Core(CoreTyp),
    Function(FunctionTyp),
    Generic(GenericTyp),
    Interface(ClassTyp),
    Param(TypParam),
    Struct(StructTyp),
}

/// Opaque extern types only
enum CoreTyp {
    Array,
    Claim,
    Float,
    List,
    ListBuilder,
    Int,
    Text,
    Typ,
    Val,
    Void,
}

struct ArrayTyp {
    item: Vec<Typ>,
    /// The only core type with a non-Type type parameter.
    len: usize,
}

/// For type args on opaque extern types.
struct BoundTyp {
    /// Applies at least to List, ListBuilder, Typ, and Val.
    core: CoreTyp,
    args: Vec<Arc<Typ>>,
}

/// No multithreading in interpreter?
type Buffer = Rc<RefCell<u8>>;

struct ClassTyp {
    name: Intern,
    uid: UidInfo,
    // TODO members
}

struct FieldDef {
    name: Intern,
    typ: Arc<Typ>,
    default: Value,
}

struct FunctionTyp {
    params: Vec<Arc<Typ>>,
    returning: Arc<Typ>,
}

/// For unapplied or partially applied types.
struct GenericTyp {
    base: Arc<Typ>,
    params: Vec<TypParam>,
}

struct Ref {
    address: usize,
    buffer: Buffer,
}

struct StructTyp {
    name: Intern,
    uid: UidInfo,
    fields: Vec<FieldDef>,
}

struct TypParam {
    name: Intern,
    typ: Arc<Typ>,
}
