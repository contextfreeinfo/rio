use serde::{Deserialize, Serialize};

use super::resolve::UidInfo;

#[derive(Clone, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum Typ {
    #[default]
    None,
    Simple(UidInfo),
    // Function(FunctionTyp),
    // Bound(BoundTyp),
    // Generic(GenericTyp),
}
