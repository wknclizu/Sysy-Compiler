#pragma once

#include "ast.h"
#include "errorReporter.h"

namespace frontend {

    class AstChecker {
    public:
        explicit AstChecker(ErrorReporter &);

        void visit_compile_unit(ast::CompileUnit &);
        void visit_block(ast::Block &);
        void visit_function(ast::Function &);
    private:
        ErrorReporter& err;
    };

}
