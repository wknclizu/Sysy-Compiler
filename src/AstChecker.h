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
        void visit_block(ast::Block &);
        void visit_statement(ast::Statement &);
        void visit_func(ast::Function &);
    private:

        std::vector<std::map<ast::Identifier,int>> m_var;
        std::map<ast::Identifier,int> m_func;

        int get_var(const ast::Identifier &name) const ;
        int get_func(const ast::Identifier &name) const ;
        int add_var(const ast::Identifier &name) ;
        int add_func(const ast::Identifier &name) ;
        
        //check expression
        void check_expr(ast::Expression *) const;
        void check_lval(ast::LValue *) const;
        void check_unary(ast::UnaryExpr *) const;
        void check_binary(ast::BinaryExpr *) const;
        void check_call(ast::Call *) const;

        ErrorReporter& err;
    };

}
