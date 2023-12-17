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
    
    auto AstChecker::is_declared(const ast::Identifier &name, std::vector<VarM *> &v)
        const  -> bool {
        // std::cerr << "checking Identifier: " << name << std::endl;
        // std::cerr << "check map: v.size() = " << v.size() << std::endl;
        for (auto &m : v) {
            // for (auto &p : *m) {
            //     std::cerr << p.first.name() << "  ";
            // }
            // std::cerr << std::endl;
            if (m->find(name) != m->end())
                return true;
        }
        return false;
    }

    auto AstChecker::is_func_declared(const ast::Identifier &name, std::vector<FunM *> &v)
        const -> bool {
        // std::cerr << "checking Identifier: " << name;
        // std::cerr << "check map: v.size() = " << v.size() << std::endl;
        for (auto &m : v) {
            // for (auto &p : *m) {
            //     std::cerr << p.first.name() << "  ";
            // }
            // std::cerr << std::endl;
            if (m->find(name) != m->end())
                return true;
        }
        return false;
    }

    void AstChecker::visit_compile_unit(ast::CompileUnit &node) {
        // global var, use map (instead of set) for future use
        VarM var;
        std::vector<VarM *> v_var;
        v_var.push_back(&var);

        FunM fun;
        std::vector<FunM *> v_func;
        v_func.push_back(&fun);

        for (auto &child : node.children()) {
            if (child.index() == 1) {
                auto &func = std::get<std::unique_ptr<ast::Function>>(child);
                auto &name = func->ident();
                if (is_func_declared(name, v_func)) {
                    err.error(ErrorType::FuncDuplicated, func->ident().name());
                } else {
                    // fun.emplace(ast::Identifier(std::string(func->ident().name()), false), func->params());
                    fun.insert({name, true});
                }
                visit_block(*func->body(), v_var, v_func);
            } else {
                auto &decl = std::get<std::unique_ptr<ast::Declaration>>(child);
                auto &name = decl->ident();
                if (var.find(name) != var.end()) {
                    err.error(ErrorType::VarDuplicated, name.name());
                }
                var.insert({name, true});
            }
        }
        v_var.pop_back();
        v_func.pop_back();
    }

    void AstChecker::visit_block(ast::Block &node, std::vector<VarM *> &v_var, std::vector<FunM *> &v_func) {
        // store all var maps
        VarM var;
        v_var.push_back(&var);
        // store all func maps
        FunM fun;
        v_func.push_back(&fun);

        auto &children = node.m_children;
        for (size_t i = 0; i < children.size(); ++i) {
            auto &c = children[i];
            if (c.index() != 0) {
                // statement
                auto &stmt = std::get<std::unique_ptr<ast::Statement>>(c);
                visit_statement(*stmt, v_var, v_func);
            } else {
                // duplicate var
                auto &decl = std::get<std::unique_ptr<ast::Declaration>>(c);
                auto &name = decl->ident();
                if (var.find(name) != var.end()) {
                    err.error(ErrorType::VarDuplicated, name.name());
                }
                var.insert({name, true});
            }
        }
        v_var.pop_back();
        v_func.pop_back();
    }

    void AstChecker::visit_statement(ast::Statement &node, std::vector<VarM *> &v_var, std::vector<FunM *> &v_func) {
        if (auto *expr_stmt = dynamic_cast<ast::ExprStmt *>(&node)) {
            auto *exp = expr_stmt->expr().get();
            check_expr(exp, v_var, v_func);

        } else if (auto *assign_stmt = dynamic_cast<ast::Assignment *>(&node)) {
            auto &lval = assign_stmt->lhs();
            auto *lval_expr = dynamic_cast<ast::LValue *>(lval.get());
            assert(lval_expr != nullptr);
            check_lval(lval_expr, v_var, v_func);

            auto &rval = assign_stmt->rhs();
            auto *rval_expr = dynamic_cast<ast::Expression *>(rval.get());
            assert(rval_expr != nullptr);
            check_expr(rval_expr, v_var, v_func);

        } else if (auto *block_stmt = dynamic_cast<ast::Block *>(&node)) {
            visit_block(*block_stmt, v_var, v_func);
        // under isn't checked
        } else if (auto *if_else_stmt = dynamic_cast<ast::IfElse *>(&node)) {
            auto &cond = if_else_stmt->cond();
            auto *cond_expr = dynamic_cast<ast::Expression *>(cond.get());
            assert(cond_expr != nullptr);
            check_expr(cond_expr, v_var, v_func);

            auto &if_block = if_else_stmt->then();
            auto *if_stmt = dynamic_cast<ast::Statement *>(if_block.get());
            if (if_stmt != nullptr)
                visit_statement(*if_stmt, v_var, v_func);

            auto &else_block = if_else_stmt->otherwise();
            auto *else_stmt = dynamic_cast<ast::Statement *>(else_block.get());
            if (else_stmt != nullptr)
                visit_statement(*else_stmt, v_var, v_func);
            
        } else if (auto *while_stmt = dynamic_cast<ast::While *>(&node)) {
            auto &cond = while_stmt->cond();
            auto *cond_expr = dynamic_cast<ast::Expression *>(cond.get());
            assert(cond_expr != nullptr);
            check_expr(cond_expr, v_var, v_func);

            auto &body = while_stmt->body();
            auto *body_stmt = dynamic_cast<ast::Statement *>(body.get());
            if (body_stmt != nullptr)
                visit_statement(*body_stmt, v_var, v_func);
            
        } else if (auto *break_stmt = dynamic_cast<ast::Break *>(&node)) {
            // TODO
        } else if (auto *continue_stmt = dynamic_cast<ast::Continue *>(&node)) {
            // TODO
        } else if (auto *return_stmt = dynamic_cast<ast::Return *>(&node)) {
            auto &ret = return_stmt->res();
            auto *ret_expr = dynamic_cast<ast::Expression *>(ret.get());
            if (ret_expr != nullptr)
                check_expr(ret_expr, v_var, v_func);
        } else {
            assert(false);
        }
    }

    void AstChecker::check_expr(ast::Expression *node, std::vector<VarM *> &v_var, std::vector<FunM *> &v_func) const {
        if (auto *lval_expr = dynamic_cast<ast::LValue *>(node)) {
            check_lval(lval_expr, v_var, v_func);
        } else if (auto *unary_expr = dynamic_cast<ast::UnaryExpr *>(node)) {
            check_unary(unary_expr, v_var, v_func);
        } else if (auto *binary_expr = dynamic_cast<ast::BinaryExpr *>(node)) {
            check_binary(binary_expr, v_var, v_func);
        } else if (auto *literal_expr = dynamic_cast<ast::Literal *>(node)) {
            // do nothing
            assert(literal_expr != nullptr);
        } else if (auto *call_expr = dynamic_cast<ast::Call *>(node)) {
            check_call(call_expr, v_var, v_func);
        } else {
            assert(false);
        }
    }

    void AstChecker::check_lval(ast::LValue *node, std::vector<VarM *> &v_var, std::vector<FunM *> &v_func)
        const {
        auto &name = node->ident();
        
        if (!is_declared(name, v_var)) {
            err.error(ErrorType::VarUnknown, name.name());
        }

        auto &v_indices = node->indices();
        std::optional<ErrorType> err_type;

        for (auto &expr_stmt : v_indices) {
            check_expr(expr_stmt.get(), v_var, v_func);
        }
    }

    void AstChecker::check_unary(ast::UnaryExpr *node, std::vector<VarM *> &v_var, std::vector<FunM *> & v_func)
        const {
        check_expr(node->operand().get(), v_var, v_func);
    }

    void AstChecker::check_binary(ast::BinaryExpr *node, std::vector<VarM *> &v_var, std::vector<FunM *> & v_func)
        const {
        check_expr(node->lhs().get(), v_var, v_func);
        check_expr(node->rhs().get(), v_var, v_func);
    }
    
    void AstChecker::check_call(ast::Call *node, std::vector<VarM *> &v_var, std::vector<FunM *> &v_func) const {
        auto &name = node->func();
        auto &args = node->args();
        if (!is_func_declared(name, v_func)) {
            err.error(ErrorType::FuncUnknown, name.name());
        }
        for (auto &arg : args) {
            if (arg.index() == 0) {
                auto *expr = std::get<std::unique_ptr<ast::Expression>>(arg).get();
                check_expr(expr, v_var, v_func);
            } else {
                // TODO: what is the class StringLiteral mean?
            }
        }
    }
}