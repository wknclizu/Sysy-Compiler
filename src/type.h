#include <llvm/IR/Type.h>
#include "ast.h"
namespace TypeSystem {
  using namespace frontend;
  llvm::Type* get(ast::ScalarType *type);
}
