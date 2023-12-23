#pragma once

#include "SysyBaseVisitor.h"
#include "SysyParser.h"
#include "ast.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

namespace frontend {

    using namespace ast;

    class AstVisitor : public SysyBaseVisitor {
    public:
        [[nodiscard]] std::unique_ptr<CompileUnit> compileUnit();

        antlrcpp::Any visitCompUnit(SysyParser::CompUnitContext *ctx) override;

        antlrcpp::Any visitConstDecl(SysyParser::ConstDeclContext *ctx) override;

        antlrcpp::Any visitInt(SysyParser::IntContext *ctx) override;

        antlrcpp::Any visitFloat(SysyParser::FloatContext *ctx) override;

        antlrcpp::Any visitVarDecl(SysyParser::VarDeclContext *ctx) override;

        antlrcpp::Any visitInit(SysyParser::InitContext *ctx) override;

        antlrcpp::Any visitInitList(SysyParser::InitListContext *ctx) override;

        antlrcpp::Any visitFuncDef(SysyParser::FuncDefContext *ctx) override;

        antlrcpp::Any visitVoid(SysyParser::VoidContext *ctx) override;

        antlrcpp::Any visitScalarParam(SysyParser::ScalarParamContext *ctx) override;

        antlrcpp::Any visitArrayParam(SysyParser::ArrayParamContext *ctx) override;

        antlrcpp::Any visitBlock(SysyParser::BlockContext *ctx) override;

        antlrcpp::Any visitAssign(SysyParser::AssignContext *ctx) override;

        antlrcpp::Any visitExprStmt(SysyParser::ExprStmtContext *ctx) override;

        antlrcpp::Any visitBlockStmt(SysyParser::BlockStmtContext *ctx) override;

        antlrcpp::Any visitIfElse(SysyParser::IfElseContext *ctx) override;

        antlrcpp::Any visitWhile(SysyParser::WhileContext *ctx) override;

        antlrcpp::Any visitBreak(SysyParser::BreakContext *ctx) override;

        antlrcpp::Any visitContinue(SysyParser::ContinueContext *ctx) override;

        antlrcpp::Any visitReturn(SysyParser::ReturnContext *ctx) override;

        antlrcpp::Any visitLVal(SysyParser::LValContext *ctx) override;

        antlrcpp::Any visitPrimaryExp_(SysyParser::PrimaryExp_Context *ctx) override;

        antlrcpp::Any visitLValExpr(SysyParser::LValExprContext *ctx) override;

        antlrcpp::Any visitDecIntConst(SysyParser::DecIntConstContext *ctx) override;

        antlrcpp::Any visitOctIntConst(SysyParser::OctIntConstContext *ctx) override;

        antlrcpp::Any visitHexIntConst(SysyParser::HexIntConstContext *ctx) override;

        antlrcpp::Any
        visitDecFloatConst(SysyParser::DecFloatConstContext *ctx) override;

        antlrcpp::Any
        visitHexFloatConst(SysyParser::HexFloatConstContext *ctx) override;

        antlrcpp::Any visitCall(SysyParser::CallContext *ctx) override;

        antlrcpp::Any visitUnaryAdd(SysyParser::UnaryAddContext *ctx) override;

        antlrcpp::Any visitUnarySub(SysyParser::UnarySubContext *ctx) override;

        antlrcpp::Any visitNot(SysyParser::NotContext *ctx) override;

        antlrcpp::Any visitStringConst(SysyParser::StringConstContext *ctx) override;

        antlrcpp::Any visitDiv(SysyParser::DivContext *ctx) override;

        antlrcpp::Any visitMod(SysyParser::ModContext *ctx) override;

        antlrcpp::Any visitMul(SysyParser::MulContext *ctx) override;

        antlrcpp::Any visitAdd(SysyParser::AddContext *ctx) override;

        antlrcpp::Any visitSub(SysyParser::SubContext *ctx) override;

        antlrcpp::Any visitGeq(SysyParser::GeqContext *ctx) override;

        antlrcpp::Any visitLt(SysyParser::LtContext *ctx) override;

        antlrcpp::Any visitLeq(SysyParser::LeqContext *ctx) override;

        antlrcpp::Any visitGt(SysyParser::GtContext *ctx) override;

        antlrcpp::Any visitNeq(SysyParser::NeqContext *ctx) override;

        antlrcpp::Any visitEq(SysyParser::EqContext *ctx) override;

        antlrcpp::Any visitAnd(SysyParser::AndContext *ctx) override;

        antlrcpp::Any visitOr(SysyParser::OrContext *ctx) override;

        antlrcpp::Any visitNumber(SysyParser::NumberContext *ctx) override;

    private:
        std::vector<std::unique_ptr<Expression>>
        visitDimensions(const std::vector<SysyParser::ExpContext *> &ctxs);

        std::unique_ptr<CompileUnit> m_compile_unit;
    };

} // namespace frontend
