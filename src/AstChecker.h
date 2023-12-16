#pragma once

#include "ast.h"
#include "errorReporter.h"
#include <optional>
#include <unordered_map>
#include <vector>

namespace frontend {

    class AstChecker {
    public:
        explicit AstChecker(ErrorReporter &);
        void visit_compile_unit(ast::CompileUnit &);
        void visit_block(ast::Block &, std::vector<std::unordered_map<std::string, bool> *> &);
        void visit_function(ast::Function &);
    private:
        // check whether a variable is declared
        auto is_declared(const std::string &, std::vector<std::unordered_map<std::string, bool> *> &) const -> bool;

        //check expression
        void check_expr(ast::Expression *, std::vector<std::unordered_map<std::string, bool> *> &) const;
        void check_lval(ast::LValue *, std::vector<std::unordered_map<std::string, bool> *> &) const;

        ErrorReporter& err;
    };

}
