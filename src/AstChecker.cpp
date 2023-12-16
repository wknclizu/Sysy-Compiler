#include "AstChecker.h"
#include "ast.h"
#include "errorReporter.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_map>
namespace frontend {


    AstChecker::AstChecker(ErrorReporter &err_reporter) : err(err_reporter) {}
    
    auto AstChecker::is_declared(const std::string &name, std::vector<std::unordered_map<std::string, bool> *> &v)
        const  -> bool {
        for (auto &m : v) {
            if (m->find(name) != m->end())
                return true;
        }
        return false;
    }

    void AstChecker::visit_compile_unit(ast::CompileUnit &node) {
        // global var, use map (instead of set) for future use
        std::unordered_map<std::string, bool> var_;
        std::vector<std::unordered_map<std::string, bool> *> v;
        v.push_back(&var_);

        for (auto &child : node.children()) {
            if (child.index() == 1) {
                auto &func = std::get<std::unique_ptr<ast::Function>>(child);
                visit_block(*func->body(), v);
            } else {
                auto &decl = std::get<std::unique_ptr<ast::Declaration>>(child);
                auto &name = decl->ident().name();
                if (var_.find(name) != var_.end()) {
                    err.error(ErrorType::VarDuplicated, name);
                }
                var_.insert({name, true});
            }
        }
        v.pop_back();
    }

    void AstChecker::visit_block(ast::Block &node, std::vector<std::unordered_map<std::string, bool> *> &v) {
        using Block = ast::Block;
        using Child = ast::Block::Child;
        auto &children = node.m_children;

        // store all var maps
        std::unordered_map<std::string, bool> var;
        v.push_back(&var);

        for (int i = 0; i < children.size(); ++i) {
            auto &c = children[i];
            if (c.index() != 0) {
                // statement
                auto &stmt = std::get<std::unique_ptr<ast::Statement>>(c);

                if (auto *expr_stmt = dynamic_cast<ast::ExprStmt *>(stmt.get())) {
                    auto *exp = expr_stmt->expr().get();
                    check_expr(exp, v);
                } else if (auto *assign_stmt = dynamic_cast<ast::Assignment *>(stmt.get())) {
                    auto &lval = assign_stmt->lhs();
                    auto *lval_expr = dynamic_cast<ast::LValue *>(lval.get());
                    assert(lval_expr != nullptr);
                    check_lval(lval_expr, v);

                    auto &rval = assign_stmt->rhs();
                    auto *rval_expr = dynamic_cast<ast::Expression *>(rval.get());
                    assert(rval_expr != nullptr);
                    check_expr(rval_expr, v);
                } else if (auto *block_stmt = dynamic_cast<ast::Block *>(stmt.get())) {
                    visit_block(*block_stmt, v);
                } else if (auto *if_else_stmt = dynamic_cast<ast::IfElse *>(stmt.get())) {
                } else if (auto *while_stmt = dynamic_cast<ast::While *>(stmt.get())) {
                } else if (auto *break_stmt = dynamic_cast<ast::Break *>(stmt.get())) {
                } else if (auto *continue_stmt = dynamic_cast<ast::Continue *>(stmt.get())) {
                } else if (auto *return_stmt = dynamic_cast<ast::Return *>(stmt.get())) {
                } else {
                    assert(false);
                }

            } else {
                // duplicate var
                auto &decl = std::get<std::unique_ptr<ast::Declaration>>(c);
                auto &name = decl->ident().name();
                if (var.find(name) != var.end()) {
                    err.error(ErrorType::VarDuplicated, name);
                }
                var.insert({name, true});
            }
        }
        v.pop_back();
    }

    void AstChecker::check_expr(ast::Expression *node, std::vector<std::unordered_map<std::string, bool> *> &v) const {
        if (auto *lval_expr = dynamic_cast<ast::LValue *>(node)) {
            check_lval(lval_expr, v);
        } else if (auto *unary_expr = dynamic_cast<ast::UnaryExpr *>(node)) {
        } else if (auto *binary_expr = dynamic_cast<ast::BinaryExpr *>(node)) {
        } else if (auto *literal_expr = dynamic_cast<ast::Literal *>(node)) {
        } else if (auto *call_expr = dynamic_cast<ast::Call *>(node)) {
        } else {
            assert(false);
        }
    }

    void AstChecker::check_lval(ast::LValue *node, std::vector<std::unordered_map<std::string, bool> *> &v)
        const {
        auto &name = node->ident().name();
        if (!is_declared(name, v)) {
            err.error(ErrorType::VarUnknown, name);
        }
        auto &v_indices = node->indices();
        std::optional<ErrorType> err_type;

        for (auto &expr_stmt : v_indices) {
            if (auto lval_expr = dynamic_cast<ast::LValue *>(expr_stmt.get())) {
                check_lval(lval_expr, v);
            } else if (auto *unary_expr = dynamic_cast<ast::UnaryExpr *>(expr_stmt.get())) {
            } else if (auto *binary_expr = dynamic_cast<ast::BinaryExpr *>(expr_stmt.get())) {
            } else if (auto *literal_expr = dynamic_cast<ast::Literal *>(expr_stmt.get())) {
            } else if (auto *call_expr = dynamic_cast<ast::Call *>(expr_stmt.get())) {
            } else {
                assert(false);
            }
        }
    }
}