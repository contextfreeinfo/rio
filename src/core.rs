use std::{collections::HashMap, fmt::Debug};

use crate::lex::{Intern, Interner};

pub struct Core {
    pub members: HashMap<Intern, NativeDef>,
}

pub enum NativeDef {
    Fun(NativeFun),
    Type(NativeType),
}

pub struct NativeFun {
    pub name: &'static str,
    pub fun: Box<dyn FnMut(&[Value]) -> Vec<Value>>,
    pub params_in: Vec<NativeParam>,
    pub params_out: Vec<NativeParam>,
}

impl Debug for NativeFun {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        f.debug_struct("NativeFun")
            .field("name", &self.name)
            // .field("fun", &self.fun)
            .field("params_in", &self.params_in)
            .field("params_out", &self.params_out)
            .finish()
    }
}

#[derive(Debug)]
pub struct NativeParam {
    pub name: &'static str,
    pub typ: NativeType,
}

#[derive(Clone, Copy, Debug)]
pub struct NativeType {
    pub name: &'static str,
}

#[derive(Debug)]
pub enum Value {
    Fun(NativeFun),
    Null(),
    Text(String),
}

pub fn make_core(interner: Interner) -> Core {
    let mut members = HashMap::<Intern, NativeDef>::new();
    let null_typ = add_type(&mut members, &interner, "Null");
    add_type(&mut members, &interner, "Text");
    {
        let name = "print";
        members.insert(
            interner.get_or_intern(name),
            NativeDef::Fun(NativeFun {
                name,
                fun: Box::new(&print_fun),
                params_in: vec![],
                params_out: vec![NativeParam {
                    name: "",
                    typ: null_typ,
                }],
            }),
        );
    }
    Core { members }
}

fn add_type(
    members: &mut HashMap<Intern, NativeDef>,
    interner: &Interner,
    name: &'static str,
) -> NativeType {
    let typ = NativeType { name };
    members.insert(interner.get_or_intern(name), NativeDef::Type(typ));
    typ
}

fn print_fun(args_in: &[Value]) -> Vec<Value> {
    assert_eq!(1, args_in.len());
    if let Value::Text(text) = &args_in[0] {
        println!("{text}");
    } else {
        panic!("bad value: {:?}", &args_in[0]);
    }
    vec![]
}
