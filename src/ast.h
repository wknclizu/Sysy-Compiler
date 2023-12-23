#pragma once

#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>

#include <variant>

#include "Display.h"
#include "common.h"

namespace frontend {
    namespace ast {
        extern std::unique_ptr<llvm::LLVMContext> TheContext;
        extern std::unique_ptr<llvm::IRBuilder<>> Builder;
        extern std::unique_ptr<llvm::Module> TheModule;
        extern std::map<std::string, llvm::Value *> NamedValues;

        using UnaryOp = ::UnaryOp;
        using BinaryOp = ::BinaryOp;

        class SysYType : public Display {
        public:
            virtual ~SysYType() = default;
        };

        class ScalarType : public SysYType {
        public:
            using Type = int;

            explicit ScalarType(Type type) : m_type{type} {}
            virtual ~ScalarType() = default;

            void print(std::ostream &out, unsigned indent) const override;

            Type type() const { return m_type; }

            bool operator==(const ScalarType &rhs) const {
                return this->m_type == rhs.m_type;
            }

        private:
            Type m_type;
        };

        class Expression;

        class ArrayType : public SysYType {
        public:
            using Dimension = std::unique_ptr<Expression>;

            ArrayType(ScalarType type, std::vector<Dimension> dimensions,
                      bool omit_first_dimension)
                    : m_type{type}, m_dimensions{std::move(dimensions)},
                      m_omit_first_dimension{omit_first_dimension} {}
            virtual ~ArrayType() = default;

            void print(std::ostream &out, unsigned indent) const override;

            ScalarType::Type base_type() const { return m_type.type(); }
            const std::vector<Dimension> &dimensions() const { return m_dimensions; }
            bool first_dimension_omitted() const { return m_omit_first_dimension; }

        private:
            ScalarType m_type;
            std::vector<Dimension> m_dimensions;
            bool m_omit_first_dimension;
        };

        class Identifier : public Display {
        public:
            Identifier(std::string name, bool const mangle = true)
                    : m_name{mangle ? '$' + name : std::move(name)} {}
            virtual ~Identifier() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const std::string &name() const { return m_name; }

            bool operator==(const Identifier &rhs) const {
                return this->m_name == rhs.m_name;
            }

            bool operator<(const Identifier &rhs) const {
                return this->m_name < rhs.m_name;
            }

        private:
            std::string m_name;
        };

        class Parameter : public Display {
        public:
            Parameter(std::unique_ptr<SysYType> type, Identifier ident)
                    : m_type{std::move(type)}, m_ident{std::move(ident)} {}
            virtual ~Parameter() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const std::unique_ptr<SysYType> &type() const { return m_type; }
            const Identifier &ident() const { return m_ident; }

        public:
            mutable std::shared_ptr<Var> var;

        private:
            std::unique_ptr<SysYType> m_type;
            Identifier m_ident;
        };

        class AstNode : public Display {
        public:
            virtual ~AstNode() = default;
            virtual llvm::Value *CodeGen() {return NULL; }
        };

        class NumberLiteral;

        class Expression : public AstNode {
        public:
            Expression() {}
            // Expression(std::unique_ptr<ScalarType> type,
            //            std::unique_ptr<NumberLiteral> value)
            //     : m_type{std::move(type)}, m_value{std::move(value)} {}
            virtual ~Expression() = default;

            //   NumberLiteral const *value() const { return m_value.get(); }

            // protected:
            //   std::unique_ptr<ScalarType> m_type;
            //   std::unique_ptr<NumberLiteral> m_value;

            virtual std::string to_string() const = 0;

        public:
            // 求得的表达式类型
            mutable std::optional<Type> type;
        };

        class LValue : public Expression {
        public:
            LValue(Identifier ident, std::vector<std::unique_ptr<Expression>> indices)
                    : m_ident{std::move(ident)}, m_indices{std::move(indices)} {}
            virtual ~LValue() = default;

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

            const Identifier &ident() const { return m_ident; }
            const std::vector<std::unique_ptr<Expression>> &indices() const {
                return m_indices;
            }

        public:
            mutable std::shared_ptr<Var> var;

        private:
            Identifier m_ident;
            std::vector<std::unique_ptr<Expression>> m_indices;
        };

        class UnaryExpr : public Expression {
        public:
            UnaryExpr(UnaryOp op, std::unique_ptr<Expression> operand)
                    : m_op{op}, m_operand{std::move(operand)} {}
            virtual ~UnaryExpr() = default;

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

            UnaryOp op() const { return m_op; }
            const std::unique_ptr<Expression> &operand() const { return m_operand; }

        private:
            UnaryOp m_op;
            std::unique_ptr<Expression> m_operand;
        };

        class BinaryExpr : public Expression {
        public:
            BinaryExpr(BinaryOp op, std::unique_ptr<Expression> lhs,
                       std::unique_ptr<Expression> rhs)
                    : m_op{op}, m_lhs{std::move(lhs)}, m_rhs{std::move(rhs)} {}
            virtual ~BinaryExpr() = default;

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

            BinaryOp op() const { return m_op; }
            const std::unique_ptr<Expression> &lhs() const { return m_lhs; }
            const std::unique_ptr<Expression> &rhs() const { return m_rhs; }
            llvm::Value *CodeGen() override {
                llvm::Value *L = m_lhs->CodeGen();
                llvm::Value *R = m_rhs->CodeGen();

                if (!L || !R)
                    return nullptr;
        
                switch (m_op) {
                    case BinaryOp::Add:
                        return Builder->CreateFAdd(L, R, "addtmp");
                    case BinaryOp::Sub:
                        return Builder->CreateFSub(L, R, "subtmp");
                    case BinaryOp::Mul:
                        return Builder->CreateFMul(L, R, "multmp");
                    case BinaryOp::Div:
                        return Builder->CreateFDiv(L, R, "divtmp");
                    case BinaryOp::Eq:
                        return Builder->CreateFCmpOEQ(L, R, "eqtmp");
                    default:
                        assert(false);
                }
            }
        private:
            BinaryOp m_op;
            std::unique_ptr<Expression> m_lhs, m_rhs;
        };

        class Literal : public Expression {
        public:
            virtual ~Literal() = default;
        };

        class NumberLiteral : public Literal {
        public:
            virtual ~NumberLiteral() = default;
        };

        class IntLiteral : public NumberLiteral {
        public:
            using Value = std::int32_t;
            static_assert(sizeof(Value) == 4);

            IntLiteral(Value value) : m_value{value} {}
            virtual ~IntLiteral() = default;

            Value value() const { return m_value; }

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;
            llvm::Value *CodeGen() override {
                return llvm::ConstantInt::get(*TheContext, llvm::APInt(m_value, true));
            }
        private:
            Value m_value;
        };

        class FloatLiteral : public NumberLiteral {
        public:
            using Value = float;
            static_assert(sizeof(Value) == 4);

            FloatLiteral(Value value) : m_value{value} {}
            virtual ~FloatLiteral() = default;

            Value value() const { return m_value; }

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;
            llvm::Value *CodeGen() override {
                return llvm::ConstantFP::get(*TheContext, llvm::APFloat(m_value));
            }

        private:
            Value m_value;
        };

        class StringLiteral : AstNode {
        public:
            using Value = std::string;

            StringLiteral(Value value) : m_value{std::move(value)} {}
            virtual ~StringLiteral() = default;

            const std::string &value() const { return m_value; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            Value m_value;
        };

        class Call : public Expression {
        public:
            using Argument = std::variant<std::unique_ptr<Expression>, StringLiteral>;

            Call(Identifier func, std::vector<Argument> args, unsigned line)
                    : m_func{std::move(func)}, m_args{std::move(args)}, m_line(line) {}
            virtual ~Call() = default;

            const Identifier &func() const { return m_func; }
            const std::vector<Argument> &args() const { return m_args; }
            unsigned line() const { return this->m_line; }

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

        private:
            Identifier m_func;
            std::vector<Argument> m_args;
            unsigned m_line;
        };

        class Statement : public AstNode {
        public:
            virtual ~Statement() = default;
        };

        class ExprStmt : public Statement {
        public:
            explicit ExprStmt(std::unique_ptr<Expression> expr)
                    : m_expr{std::move(expr)} {}
            virtual ~ExprStmt() = default;

            const std::unique_ptr<Expression> &expr() const { return m_expr; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            std::unique_ptr<Expression> m_expr;
        };

        class Assignment : public Statement {
        public:
            Assignment(std::unique_ptr<LValue> lhs, std::unique_ptr<Expression> rhs)
                    : m_lhs{std::move(lhs)}, m_rhs{std::move(rhs)} {}
            virtual ~Assignment() = default;

            const std::unique_ptr<LValue> &lhs() const { return m_lhs; }
            const std::unique_ptr<Expression> &rhs() const { return m_rhs; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            std::unique_ptr<LValue> m_lhs;
            std::unique_ptr<Expression> m_rhs;
        };

        class Initializer : public AstNode {
        public:
            using Value = std::variant<std::unique_ptr<Expression>,
                    std::vector<std::unique_ptr<Initializer>>>;

            explicit Initializer(std::unique_ptr<Expression> value)
                    : m_value{std::move(value)} {}
            explicit Initializer(std::vector<std::unique_ptr<Initializer>> values)
                    : m_value{std::move(values)} {}
            virtual ~Initializer() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const Value &value() const { return m_value; }

        private:
            Value m_value;
        };

        class Declaration : public AstNode {
        public:
            Declaration(std::unique_ptr<SysYType> type, Identifier ident,
                        std::unique_ptr<Initializer> init, bool const_qualified)
                    : m_type{std::move(type)}, m_ident{std::move(ident)},
                      m_init{std::move(init)}, m_const_qualified{const_qualified} {}
            virtual ~Declaration() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const std::unique_ptr<SysYType> &type() const { return m_type; }
            const Identifier &ident() const { return m_ident; }
            const std::unique_ptr<Initializer> &init() const { return m_init; }
            bool const_qualified() const { return m_const_qualified; }

        public:
            mutable std::shared_ptr<Var> var;

        private:
            std::unique_ptr<SysYType> m_type;
            Identifier m_ident;
            std::unique_ptr<Initializer> m_init;
            bool m_const_qualified;
        };

        class Block : public Statement {
        public:
            using Child =
                    std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Statement>>;

            explicit Block(std::vector<Child> children)
                    : m_children{std::move(children)} {}
            virtual ~Block() = default;

            const std::vector<Child> &children() const { return m_children; }

            void print(std::ostream &out, unsigned indent) const override;
            llvm::Value *CodeGen() override {
                for (auto& child : m_children) {
                    if (child.index() == 0) {
                        auto &decl = std::get<std::unique_ptr<Declaration>>(child);
                        return decl->CodeGen();
                    } else {
                        auto &stat = std::get<std::unique_ptr<Statement>>(child);
                        return stat->CodeGen();
                    }
                }
            
            }

        public:
            std::vector<Child> m_children;
        };

        class IfElse : public Statement {
        public:
            IfElse(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> then,
                   std::unique_ptr<Statement> else_)
                    : m_cond{std::move(cond)}, m_then{std::move(then)}, m_else{std::move(
                    else_)} {}
            virtual ~IfElse() = default;

            const std::unique_ptr<Expression> &cond() const { return m_cond; }
            const std::unique_ptr<Statement> &then() const { return m_then; }
            const std::unique_ptr<Statement> &otherwise() const { return m_else; }

            void print(std::ostream &out, unsigned indent) const override;

        public:
            std::unique_ptr<Expression> m_cond;
            std::unique_ptr<Statement> m_then, m_else;
        };

        class While : public Statement {
        public:
            While(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> body)
                    : m_cond{std::move(cond)}, m_body{std::move(body)} {}
            virtual ~While() = default;

            const std::unique_ptr<Expression> &cond() const { return m_cond; }
            const std::unique_ptr<Statement> &body() const { return m_body; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            std::unique_ptr<Expression> m_cond;
            std::unique_ptr<Statement> m_body;
        };

        class Break : public Statement {
        public:
            virtual ~Break() = default;

            void print(std::ostream &out, unsigned indent) const override;
        };

        class Continue : public Statement {
        public:
            virtual ~Continue() = default;

            void print(std::ostream &out, unsigned indent) const override;
        };

        class Return : public Statement {
        public:
            explicit Return(std::unique_ptr<Expression> res) : m_res{std::move(res)} {}
            virtual ~Return() = default;

            const std::unique_ptr<Expression> &res() const { return m_res; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            std::unique_ptr<Expression> m_res;
        };

        class Function : public AstNode {
        public:
            Function(std::unique_ptr<ScalarType> type, Identifier ident,
                     std::vector<std::unique_ptr<Parameter>> params,
                     std::unique_ptr<Block> body)
                    : m_type{std::move(type)}, m_ident{std::move(ident)},
                      m_params{std::move(params)}, m_body{std::move(body)} {}
            virtual ~Function() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const std::unique_ptr<ScalarType> &type() const { return m_type; }
            const Identifier &ident() const { return m_ident; }
            const std::vector<std::unique_ptr<Parameter>> &params() const {
                return m_params;
            }
            const std::unique_ptr<Block> &body() const { return m_body; }

            bool operator==(const Function &rhs) const {
                // this->m_params == rhs.m_params &&
                return this->m_ident == rhs.m_ident &&
                       this->m_body == rhs.m_body;
            }
            // TODO: check if this is correct
            bool operator<(const Function &rhs) const {
                return this->m_ident < rhs.m_ident;
            }
            llvm::Value *CodeGen() override {
                llvm::Function* function = TheModule->getFunction(m_ident.name());
                if (!function) {
                    std::vector<llvm::Type*> args(m_params.size(), llvm::Type::getDoubleTy(*TheContext));
                    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), args, false);
                    function = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, m_ident.name(), TheModule.get());
                    unsigned Idx = 0;
                    for (auto& Arg : function->args()) {
                        Arg.setName(m_params[Idx++]->ident().name());
                    }
                }
                if (!function) 
                    return nullptr;

                llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", function);    
                Builder->SetInsertPoint(BB);

                // Record the function arguments in the NamedValues map.
                NamedValues.clear();
                for (auto &Arg : function->args())
                    NamedValues[std::string(Arg.getName())] = &Arg;

                if (llvm::Value *RetVal = m_body->CodeGen()) {
                    // Finish off the function.
                    Builder->CreateRet(RetVal);

                    // Validate the generated code, checking for consistency.
                    // verifyFunction(*function);

                    return function;
                }

                // Error reading body, remove function.
                function->eraseFromParent();
                return nullptr;
                    
                
            }

        private:
            std::unique_ptr<ScalarType> m_type;
            Identifier m_ident;
            std::vector<std::unique_ptr<Parameter>> m_params;
            std::unique_ptr<Block> m_body;
        };

        class CompileUnit : public AstNode {
        public:
            using Child =
                    std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Function>>;

            explicit CompileUnit(std::vector<Child> children)
                    : m_children{std::move(children)} {}
            virtual ~CompileUnit() = default;

            void print(std::ostream &out, unsigned indent) const override;
            llvm::Value *CodeGen() override {
                for (auto& child : m_children) {
                    if (child.index() == 0) {
                        auto &decl = std::get<std::unique_ptr<Declaration>>(child);
                        return decl->CodeGen(); // TODO: 暂时只返回一个
                    } else {
                        auto &func = std::get<std::unique_ptr<Function>>(child);
                        return func->CodeGen();
                    }
                }
            }
            const std::vector<Child> &children() const { return m_children; }

        private:
            std::vector<Child> m_children;
        };

    } // namespace ast
} // namespace frontend
