#include "AstChecker.h"
#include "ast.h"
#include "errorReporter.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
namespace frontend {


    AstChecker::AstChecker(ErrorReporter &err_reporter) : err(err_reporter) {}
    
    int AstChecker::add_var(const ast::Identifier &name) {
        auto tname = ast::Identifier(name.name().substr(1), false);
        if (m_func.count(tname)) {
            err.error(ErrorType::IncorrectUseOfFuncName, tname.name());
            return 0;
        }
        if (m_var.back().count(name)) {
            err.error(ErrorType::VarDuplicated, name.name());
        }
        return m_var.back()[name] = 1;
    }
    int AstChecker::get_var(const ast::Identifier &name) const {
        for (auto i = m_var.rbegin(); i != m_var.rend(); ++i) {
            if (i->count(name)) {
                return i->at(name);
            }
        }
        auto tname = ast::Identifier(name.name().substr(1), false);
        if (m_func.count(tname)) {
            err.error(ErrorType::IncorrectUseOfFuncName, name.name());
            return 0;
        }
        err.error(ErrorType::VarUnknown, name.name());
        return 0;
    }
    int AstChecker::add_func(const ast::Identifier &name) {
        for (auto i = m_var.rbegin(); i != m_var.rend(); ++i) {
            if (i->count(ast::Identifier(name.name()))) {
                err.error(ErrorType::CallVarAsFunc, name.name());
                return 0;
            }
        }
        if (m_func.count(name)) {
            err.error(ErrorType::FuncDuplicated, name.name());
        }
        return m_func[name] = 1;
    }
    int AstChecker::get_func(const ast::Identifier &name) const {
        if (!m_func.count(name)) {
            for (auto i = m_var.rbegin(); i != m_var.rend(); ++i) {
                if (i->count(ast::Identifier(name.name()))) {
                    err.error(ErrorType::CallVarAsFunc, name.name());
                    return 0;
                }
            }
            err.error(ErrorType::FuncUnknown, name.name());
            return 0;
        } else {
            return m_func.at(name);
        }
    }

    void AstChecker::visit_compile_unit(ast::CompileUnit &node) {
        m_var.resize(m_var.size() + 1);

        // global var, use map (instead of set) for future use

        for (auto &child : node.children()) {
            if (child.index() == 1) {
                auto &func = std::get<std::unique_ptr<ast::Function>>(child);
                visit_func(*func);
            } else {
                auto &decl = std::get<std::unique_ptr<ast::Declaration>>(child);
                auto &name = decl->ident();
                add_var(name);
            }
        }
        m_var.pop_back();
    }

    void AstChecker::visit_func(ast::Function &node) {
        m_var.resize(m_var.size() + 1);

        auto &name = node.ident();
        add_func(name);

        for (auto &param : node.params()) {
            add_var(param->ident());
        }
        visit_block(*node.body());

        m_var.pop_back();
    }
    void AstChecker::visit_block(ast::Block &node) {
        m_var.resize(m_var.size() + 1);

        auto &children = node.m_children;
        for (size_t i = 0; i < children.size(); ++i) {
            auto &c = children[i];
            if (c.index() != 0) {
                // statement
                auto &stmt = std::get<std::unique_ptr<ast::Statement>>(c);
                visit_statement(*stmt);
            } else {
                // duplicate var
                auto &decl = std::get<std::unique_ptr<ast::Declaration>>(c);
                auto &name = decl->ident();
                add_var(name);
            }
        }
        m_var.pop_back();
    }

    void AstChecker::visit_statement(ast::Statement &node) {
        if (auto *expr_stmt = dynamic_cast<ast::ExprStmt *>(&node)) {
            auto *exp = expr_stmt->expr().get();
            check_expr(exp);

        } else if (auto *assign_stmt = dynamic_cast<ast::Assignment *>(&node)) {
            auto &lval = assign_stmt->lhs();
            auto *lval_expr = dynamic_cast<ast::LValue *>(lval.get());
            assert(lval_expr != nullptr);
            check_lval(lval_expr);

            auto &rval = assign_stmt->rhs();
            auto *rval_expr = dynamic_cast<ast::Expression *>(rval.get());
            assert(rval_expr != nullptr);
            check_expr(rval_expr);

        } else if (auto *block_stmt = dynamic_cast<ast::Block *>(&node)) {
            visit_block(*block_stmt);
        // under isn't checked
        } else if (auto *if_else_stmt = dynamic_cast<ast::IfElse *>(&node)) {
            auto &cond = if_else_stmt->cond();
            auto *cond_expr = dynamic_cast<ast::Expression *>(cond.get());
            assert(cond_expr != nullptr);
            check_expr(cond_expr);

            auto &if_block = if_else_stmt->then();
            auto *if_stmt = dynamic_cast<ast::Statement *>(if_block.get());
            if (if_stmt != nullptr)
                visit_statement(*if_stmt);

            auto &else_block = if_else_stmt->otherwise();
            auto *else_stmt = dynamic_cast<ast::Statement *>(else_block.get());
            if (else_stmt != nullptr)
                visit_statement(*else_stmt);
            
        } else if (auto *while_stmt = dynamic_cast<ast::While *>(&node)) {
            auto &cond = while_stmt->cond();
            auto *cond_expr = dynamic_cast<ast::Expression *>(cond.get());
            assert(cond_expr != nullptr);
            check_expr(cond_expr);

            auto &body = while_stmt->body();
            auto *body_stmt = dynamic_cast<ast::Statement *>(body.get());
            if (body_stmt != nullptr)
                visit_statement(*body_stmt);
            
        } else if (auto *break_stmt = dynamic_cast<ast::Break *>(&node)) {
            // TODO
        } else if (auto *continue_stmt = dynamic_cast<ast::Continue *>(&node)) {
            // TODO
        } else if (auto *return_stmt = dynamic_cast<ast::Return *>(&node)) {
            auto &ret = return_stmt->res();
            auto *ret_expr = dynamic_cast<ast::Expression *>(ret.get());
            if (ret_expr != nullptr)
                check_expr(ret_expr);
        } else {
            assert(false);
        }
    }

    void AstChecker::check_expr(ast::Expression *node) const {
        if (auto *lval_expr = dynamic_cast<ast::LValue *>(node)) {
            check_lval(lval_expr);
        } else if (auto *unary_expr = dynamic_cast<ast::UnaryExpr *>(node)) {
            check_unary(unary_expr);
        } else if (auto *binary_expr = dynamic_cast<ast::BinaryExpr *>(node)) {
            check_binary(binary_expr);
        } else if (auto *literal_expr = dynamic_cast<ast::Literal *>(node)) {
            // do nothing
            assert(literal_expr != nullptr);
        } else if (auto *call_expr = dynamic_cast<ast::Call *>(node)) {
            check_call(call_expr);
        } else {
            assert(false);
        }
    }

    void AstChecker::check_lval(ast::LValue *node)
        const {
        auto &name = node->ident();
        
        int t = get_var(name);

        auto &v_indices = node->indices();
        std::optional<ErrorType> err_type;

        for (auto &expr_stmt : v_indices) {
            check_expr(expr_stmt.get());
        }
    }

    void AstChecker::check_unary(ast::UnaryExpr *node)
        const {
        check_expr(node->operand().get());
    }

    void AstChecker::check_binary(ast::BinaryExpr *node)
        const {
        check_expr(node->lhs().get());
        check_expr(node->rhs().get());
    }
    
    void AstChecker::check_call(ast::Call *node) const {
        auto &name = node->func();
        auto &args = node->args();
        int t = get_func(name);
        for (auto &arg : args) {
            if (arg.index() == 0) {
                auto *expr = std::get<std::unique_ptr<ast::Expression>>(arg).get();
                check_expr(expr);
            } else {
                // TODO: what is the class StringLiteral mean?
            }
        }
    }
}
