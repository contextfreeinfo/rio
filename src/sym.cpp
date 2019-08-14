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
    case Type::Kind::I32: {
      buf->push_string("i32");
      break;
    }
    case Type::Kind::Array: {
      buf->push_string("rio_Span_");
      name_type_sub(engine, buf, type->arg);
      break;
    }
    default: break; 
  }
}

}
