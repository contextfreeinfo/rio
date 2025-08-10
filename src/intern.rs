use std::{cell::Ref, collections::HashMap, ops::Index};

pub type Intern = usize;

#[derive(Default)]
pub struct Interner {
    // We claim static, but really they're refs into the texts.
    map: HashMap<&'static str, usize>,
    texts: Vec<Box<str>>,
}

impl Interner {
    pub fn intern(&mut self, s: &str) -> Intern {
        if let Some(&id) = self.map.get(s) {
            return id;
        }
        let id = self.texts.len();
        let boxed: Box<str> = s.to_owned().into_boxed_str();
        let slice: &str = &boxed;
        // SAFETY: This references into the text box, which we control until drop.
        let static_ref: &'static str = unsafe { std::mem::transmute::<&str, &'static str>(slice) };
        self.texts.push(boxed);
        self.map.insert(static_ref, id);
        id
    }

    pub fn resolve(&self, id: Intern) -> &str {
        // Presents in the call as bound to the Interner lifetime, which is correct.
        &self.texts[id]
    }
}

impl Index<Intern> for Interner {
    type Output = str;

    fn index(&self, intern: Intern) -> &Self::Output {
        self.resolve(intern)
    }
}
