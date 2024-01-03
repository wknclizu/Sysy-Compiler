#include "type.h"
#include "ast.h"
#include "common.h"
#include <stdexcept>
using namespace frontend;
llvm::Type* TypeSystem::get(ast::ScalarType *type) {
  if (!type) throw std::runtime_error{"null type"};
  switch (type->type()) {
    case Int:
      return llvm::Type::getInt32Ty(*ast::TheContext);
    case Float:
      return llvm::Type::getFloatTy(*ast::TheContext);
    case String:
      throw std::runtime_error{"current can't handle string"};
    
  }
  throw std::runtime_error{"unknown type"};
}

