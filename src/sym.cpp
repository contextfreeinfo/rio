#include "main.h"
#include "node.h"

namespace rio {

auto name_type_sub(Engine* engine, StrBuf* buf, const Type* type) -> void;

auto name_type(Engine* engine, StrBuf* buf, const Type& type) -> string {
  buf->clear();
  name_type_sub(engine, buf, &type);
  if (buf->len) {
    return intern_str(engine, *buf);
  } else {
    return "";
  }
}

auto name_type_sub(Engine* engine, StrBuf* buf, const Type* type) -> void {
  if (!type) {
    return;
  }
  switch (type->kind) {
    case Type::Kind::Array: {
      buf->push_string("rio_Span_");
      name_type_sub(engine, buf, type->arg);
      break;
    }
    case Type::Kind::Bool: {
      buf->push_string("bool");
      break;
    }
    case Type::Kind::Float: {
      // This is rio_float as opposed to c_float.
      buf->push_string("float");
      break;
    }
    case Type::Kind::I32: {
      buf->push_string("i32");
      break;
    }
    case Type::Kind::Int: {
      // This is rio_int as opposed to c_int.
      buf->push_string("int");
      break;
    }
    case Type::Kind::Range: {
      buf->push_string("rio_Range_");
      name_type_sub(engine, buf, type->arg);
      break;
    }
    case Type::Kind::String: {
      buf->push_string("string");
      break;
    }
    case Type::Kind::User: {
      if (type->def) {
        if (type->def->mod) {
          buf->push_string(type->def->mod->name);
          buf->push_string("_");
        }
        buf->push_string(type->def->name);
      }
      break;
    }
    default: break; 
  }
}

}
