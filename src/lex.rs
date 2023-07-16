use std::sync::Arc;

use lasso::ThreadedRodeo;

pub struct Lexer {
    pub rodeo: Arc<ThreadedRodeo>,
}

impl Lexer {
    pub fn lex(&self, source: &str) {
        let key = self.rodeo.get_or_intern(source);
        println!("{key:?} {:?}", self.rodeo.resolve(&key));
    }
}
