use crate::lex::Intern;
use crate::refine::resolve::UidInfo;
use std::sync::{Arc, Mutex};

enum Value {
    Claim(bool),
    Int(isize),
    Float(f64),
    Function(Function),
    List(Arc<Vec<Value>>),
    ListBuilder(Arc<Mutex<Vec<Value>>>),
    Ref(usize),
    /// Intern would be risky since strings can be generated in loops.
    Text(Arc<String>),
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

struct Function {
    uid: UidInfo,
    typ: Arc<Typ>,
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

struct StructTyp {
    name: Intern,
    uid: UidInfo,
    fields: Vec<FieldDef>,
}

struct TypParam {
    name: Intern,
    typ: Arc<Typ>,
}
