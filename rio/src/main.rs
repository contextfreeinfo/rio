fn main() {
    let lexer = rio_engine::Lexer::new(None);
    let tokens = lexer.lex(SOURCE);
    println!("Tokens: {}", tokens.len());
}

const SOURCE: &str = r#"
pub fun main(sys)
   # Prove we can call twice.
   greet()
   greet()
end

fun greet()
   # Had some parsing bugs, so try string variety.
   log("I say, \"Hi!\"")
   log("Now bye.")
   log("")
end
"#;
