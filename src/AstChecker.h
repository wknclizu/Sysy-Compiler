#pragma once

#include "ast.h"
#include "errorReporter.h"
#include <optional>
#include <map>
#include <vector>

namespace frontend {

    class AstChecker {
    public:
        using VarM = std::map<ast::Identifier, bool>;
        // the map from function name to its parameters
        // using FunM = std::map<ast::Identifier, std::vector<std::unique_ptr<ast::Parameter>> &>;
        using FunM = std::map<ast::Identifier, bool>;

        explicit AstChecker(ErrorReporter &);
        void visit_compile_unit(ast::CompileUnit &);
        void visit_block(ast::Block &, std::vector<VarM *> &, std::vector<FunM *> &);
        void visit_statement(ast::Statement &, std::vector<VarM *> &, std::vector<FunM *> &);
    private:
        /**
         * @brief  check whether a variable is declared
         */
        auto is_declared(const ast::Identifier &, std::vector<VarM *> &) const -> bool;

        /**
         * @brief check whether a function is declared, DID NOT check the parameter
         */
        auto is_func_declared(const ast::Identifier &, std::vector<FunM *> &) const -> bool;

        //check expression
        void check_expr(ast::Expression *, std::vector<VarM *> &, std::vector<FunM *> &) const;
        void check_lval(ast::LValue *, std::vector<VarM *> &, std::vector<FunM *> &) const;
        void check_unary(ast::UnaryExpr *, std::vector<VarM *> &, std::vector<FunM *> &) const;
        void check_binary(ast::BinaryExpr *, std::vector<VarM *> &, std::vector<FunM *> &) const;
        void check_call(ast::Call *, std::vector<VarM *> &, std::vector<FunM *> &) const;

        ErrorReporter& err;
    };

}
