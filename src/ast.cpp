#include "ast.h"
#include "utils.h"
#include "SymbolTable.h"
#include "type.h"

#include <cassert>
#include <llvm/IR/LLVMContext.h>
#include <ostream>
#include <stdexcept>
#include <string>
#include <stack>
#include <string_view>
#include <utility>

using namespace frontend::ast;
using std::ostream;

using AstScalarType = frontend::ast::ScalarType;

// 用于记录循环信息，在continue/break时知道应该跳转到哪里
struct LoopInfo{
    llvm::BasicBlock *continueBB;
    llvm::BasicBlock *breakBB;
};

std::unique_ptr<llvm::LLVMContext> frontend::ast::TheContext = std::make_unique<llvm::LLVMContext>();
std::unique_ptr<llvm::IRBuilder<>> frontend::ast::Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
std::unique_ptr<llvm::Module> frontend::ast::TheModule = std::make_unique<llvm::Module>("testModule", *TheContext);
std::map<std::string, llvm::Value *> frontend::ast::NamedValues;
SymbolTable<llvm::Value*> symbolTable; // 默认全局变量，只管理变量不管理函数
llvm::Function *nowFunction;
std::stack<LoopInfo> loops;

std::string_view op_string(UnaryOp op) {
    switch (op) {
        case UnaryOp::Add:
            return "+";
        case UnaryOp::Sub:
            return "-";
        case UnaryOp::Not:
            return "!";
    }
    return "<unknown_unary_op>";
}

std::string_view op_string(BinaryOp op) {
    unsigned i = static_cast<unsigned>(op);
    if (op >= BinaryOp::NR_OPS)
        return "<unknown_binary_op>";
    static constexpr std::string_view op_strs[] = {
            "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=", "&&", "||"};
    return op_strs[i];
}

std::string_view type_string(int type) {
    switch (type) {
        case Int:
            return "int";
        case Float:
            return "float";
        default:
            break;
    }
    return "<unknown_scalar_type>";
}

std::string_view type_string(const AstScalarType *type) {
    if (!type)
        return "void";
    return type_string(type->type());
}

ostream &operator<<(ostream &os, const std::unique_ptr<AstScalarType> &type) {
    os << type_string(type.get());
    return os;
}

ostream &operator<<(ostream &os, const Identifier &ident) {
    os << ident.name();
    return os;
}

ostream &operator<<(ostream &os, const ArrayType &type) {
    os << type_string(type.base_type());
    int n_dims = type.dimensions().size();
    if (type.first_dimension_omitted())
        n_dims++;
    for (int i = 0; i < n_dims; ++i)
        os << "[]";
    // TODO: dimension的具体信息
    return os;
}

ostream &operator<<(ostream &os, const std::unique_ptr<SysYType> &type) {
    if (!type) {
        os << "void";
        return os;
    }

    auto raw = type.get();
    if (auto scalar_type = dynamic_cast<AstScalarType *>(raw)) {
        os << type_string(scalar_type);
    } else if (auto array_type = dynamic_cast<ArrayType *>(raw)) {
        os << *array_type;
    } else {
        assert(false);
    }
    return os;
}

ostream &operator<<(ostream &os, const Parameter &param) {
    os << param.type() << ' ' << param.ident();
    return os;
}

void AstScalarType::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << type_string(this) << '\n';
}

void ArrayType::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << *this << '\n';
}

void Identifier::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << *this << '\n';
}

void Parameter::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << *this << '\n';
}

void LValue::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "LValue " << m_ident;
    for (auto const &index : this->m_indices) {
        out << "[\n";
        index->print(out, indent + INDENT_LEN);
        print_indent(out, indent);
        out << ']';
    }
    out << '\n';
}

void UnaryExpr::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "UnaryExpr " << op_string(m_op) << '\n';
    assert(m_operand);
    m_operand->print(out, indent + INDENT_LEN);
}

void BinaryExpr::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "BinaryExpr " << op_string(m_op) << '\n';
    assert(m_lhs);
    assert(m_rhs);
    m_lhs->print(out, indent + INDENT_LEN);
    m_rhs->print(out, indent + INDENT_LEN);
}

void IntLiteral::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "IntLiteral " << m_value << '\n';
}

void FloatLiteral::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "FloatLiteral " << m_value << '\n';
}

void StringLiteral::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "StringLiteral " << m_value << '\n';
}

void Call::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "Call " << m_func << '\n';
    for (auto &arg : m_args) {
        if (arg.index() == 0) {
            auto &expr = std::get<std::unique_ptr<Expression>>(arg);
            assert(expr);
            expr->print(out, indent + INDENT_LEN);
        } else {
            auto &literal = std::get<StringLiteral>(arg);
            literal.print(out, indent + INDENT_LEN);
        }
    }
}

void ExprStmt::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "ExprStmt\n";
    if (m_expr)
        m_expr->print(out, indent + INDENT_LEN);
}

void Assignment::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "Assignment\n";
    assert(m_lhs);
    m_lhs->print(out, indent + INDENT_LEN);
    assert(m_rhs);
    m_rhs->print(out, indent + INDENT_LEN);
}

void Initializer::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "Initializer\n";
    if (m_value.index() == 0) {
        auto &expr = std::get<std::unique_ptr<Expression>>(m_value);
        assert(expr);
        expr->print(out, indent + INDENT_LEN);
    } else {
        auto &initializers =
                std::get<std::vector<std::unique_ptr<Initializer>>>(m_value);
        for (auto &initializer : initializers) {
            assert(initializer);
            initializer->print(out, indent + INDENT_LEN);
        }
    }
}

void Declaration::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "Declaration ";
    if (m_const_qualified)
        out << "const ";
    out << m_type << ' ' << m_ident << '\n';
    if (m_init)
        m_init->print(out, indent + INDENT_LEN);
}

void Block::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "Block\n";
    for (auto &child : m_children) {
        if (child.index() == 0) {
            auto &decl = std::get<std::unique_ptr<Declaration>>(child);
            assert(decl);
            decl->print(out, indent + INDENT_LEN);
        } else {
            auto &stmt = std::get<std::unique_ptr<Statement>>(child);
            assert(stmt);
            stmt->print(out, indent + INDENT_LEN);
        }
    }
}

void IfElse::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "IfElse\n";

    print_indent(out, indent + INDENT_LEN);
    out << "Cond\n";
    assert(m_cond);
    m_cond->print(out, indent + INDENT_LEN * 2);

    print_indent(out, indent + INDENT_LEN);
    out << "Then\n";
    assert(m_then);
    m_then->print(out, indent + INDENT_LEN * 2);

    if (m_else) {
        print_indent(out, indent + INDENT_LEN);
        out << "Else\n";
        m_else->print(out, indent + INDENT_LEN * 2);
    }
}

void While::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "While\n";

    print_indent(out, indent + INDENT_LEN);
    out << "Cond\n";
    assert(m_cond);
    m_cond->print(out, indent + INDENT_LEN * 2);

    print_indent(out, indent + INDENT_LEN);
    out << "Body\n";
    assert(m_body);
    m_body->print(out, indent + INDENT_LEN * 2);
}

void Break::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "Break\n";
}

void Continue::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "Continue\n";
}

void Return::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "Return\n";
    if (m_res)
        m_res->print(out, indent + INDENT_LEN);
}

void Function::print(std::ostream &out, unsigned int indent) const {
    // this->CodeGen()->print(llvm::errs(), false);

    print_indent(out, indent);
    out << "Function " << m_type << ' ' << m_ident;
    out << '(';
    for (int i = 0; i < m_params.size(); ++i) {
        if (i)
            out << ", ";
        assert(m_params[i]);
        out << *m_params[i];
    }
    out << ")\n";
    assert(m_body);
    m_body->print(out, indent + INDENT_LEN);
}

void CompileUnit::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << "CompileUnit\n";
    for (auto &child : m_children) {
        if (child.index() == 0) {
            auto &decl = std::get<std::unique_ptr<Declaration>>(child);
            decl->print(out, indent + INDENT_LEN);
        } else {
            auto &func = std::get<std::unique_ptr<Function>>(child);
            func->print(out, indent + INDENT_LEN);
        }
    }
}

bool swappable(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:
        case BinaryOp::Mul:
        case BinaryOp::Eq:
        case BinaryOp::Neq:
        case BinaryOp::Lt:
        case BinaryOp::Gt:
        case BinaryOp::Leq:
        case BinaryOp::Geq:
            return true;
        default:
            return false;
    }
}

std::string LValue::to_string() const {
    std::string s = m_ident.name();
    for (auto &index : m_indices) {
        s += "[";
        s += index->to_string();
        s += "]";
    }
    return s;
}

std::string UnaryExpr::to_string() const {
    auto op = std::string{op_string(m_op)};
    return op + m_operand->to_string();
}

std::string BinaryExpr::to_string() const {
    auto op = m_op;
    auto lhs = m_lhs->to_string();
    auto rhs = m_rhs->to_string();
    if (op == BinaryOp::Gt) {
        op = BinaryOp::Lt;
        std::swap(lhs, rhs);
    }
    if (op == BinaryOp::Geq) {
        op = BinaryOp::Leq;
        std::swap(lhs, rhs);
    }

    if (swappable(op) && !(lhs < rhs))
        std::swap(lhs, rhs);
    auto ops = std::string{op_string(op)};
    return lhs + ops + rhs;
}

std::string IntLiteral::to_string() const {
    return std::to_string(m_value);
}

std::string FloatLiteral::to_string() const {
    return std::to_string(m_value);
}

std::string Call::to_string() const {
    std::string s = m_func.name() + "(";
    auto arg_string = [](const Argument &arg) {
        if (arg.index() == 0)
            return std::get<0>(arg)->to_string();
        else
            return std::get<1>(arg).value();
    };
    if (!m_args.empty())
        s += arg_string(m_args[0]);
    for (size_t i = 1; i < m_args.size(); ++i) {
        s += ", ";
        s += arg_string(m_args[i]);
    }
    s += ")";
    return s;
}
llvm::Value *BinaryExpr::CodeGen() {
    llvm::Value *L = m_lhs->CodeGen();
    llvm::Value *R = m_rhs->CodeGen();

    if (!L || !R)
        return nullptr;

    
    // L->print(llvm::errs(), false);
    // R->print(llvm::errs(), false);
    switch (m_op) {
        case BinaryOp::Add:
            std::cerr<< "ADD\n";
            return Builder->CreateAdd(L, R);
        case BinaryOp::Sub:
            return Builder->CreateSub(L, R, "subtmp");
        case BinaryOp::Mul:
            return Builder->CreateMul(L, R, "multmp");
        case BinaryOp::Div:
            return Builder->CreateFDiv(L, R, "divtmp"); // TODO: 换成int的
        case BinaryOp::Eq:
            return Builder->CreateFCmpOEQ(L, R, "eqtmp");
        default:
            assert(false);
    }
}
llvm::Value *IntLiteral::CodeGen() {
    // std::cerr<<m_value<< '\n';
    auto toret = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), m_value);
    return toret;
    // auto toret = llvm::ConstantInt::get(*TheContext, llvm::APInt(int(m_value), true)); RE
}
llvm::Value *FloatLiteral::CodeGen() {
    return nullptr;
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(m_value));
}
llvm::Value *Function::CodeGen() {
    // std::cerr<<  m_ident.name()<< std::endl;
    llvm::Function* function = TheModule->getFunction(m_ident.name());
    // llvm::Function* function = nullptr;
    if (!function) {
        // std::cerr<< "NEW FUNC\n";
        // 暂时所有参数类型都是返回值类型
        std::vector<llvm::Type*> args(m_params.size(), TypeSystem::get(type().get()));
        llvm::FunctionType *FT = llvm::FunctionType::get(TypeSystem::get(type().get()), args, false);
        function = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, m_ident.name(), TheModule.get());
        unsigned Idx = 0;
        for (auto& Arg : function->args()) {
            Arg.setName(m_params[Idx++]->ident().name());
        }
    }
    if (!function) 
        return nullptr;
    nowFunction = function;

    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", function);    
    Builder->SetInsertPoint(BB);
    if (Builder->GetInsertBlock()) {
        std::cerr<< "GetInsertBlock ok\n";
    }

    // Record the function arguments in the NamedValues map.
    NamedValues.clear();
    for (auto &Arg : function->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    m_body->CodeGen();
    /* 此处因为在tutorial3中，每个函数block必须返回，因此如果失败则删除该函数
    if (llvm::Value *RetVal = m_body->CodeGen()) {
        // RetVal->print(llvm::errs(), false);
        // Finish off the function.
        Builder->CreateRet(RetVal);
        // Validate the generated code, checking for consistency.
        // verifyFunction(*function);

        std::cerr<< "485\n";
        return function;
    }
    */

    // Error reading body, remove function.
    // function->eraseFromParent();
    return function;
}

llvm::Value *CompileUnit::CodeGen() {
    for (auto& child : m_children) {
        if (child.index() == 0) {
            auto &decl = std::get<std::unique_ptr<Declaration>>(child);
            decl->CodeGen();
        } else {
            auto &func = std::get<std::unique_ptr<Function>>(child);
            func->CodeGen(); // ->print(llvm::errs(), false);
        }
    }
    return nullptr;
}
llvm::Value *Declaration::CodeGen() {
    if (nowFunction) {
        // 在函数头部使用alloca分配空间
        llvm::IRBuilder<> entryBuilder(
                &nowFunction->getEntryBlock(),
                nowFunction->getEntryBlock().begin()
        );

        // 生成局部变量
        llvm::AllocaInst *alloca = entryBuilder.CreateAlloca(
                TypeSystem::get(dynamic_cast<ScalarType*>(type().get())),
                nullptr,
                m_ident.name()
        );

        // 将局部变量插入符号表
        symbolTable.insert(ident().name(), alloca);

        // 初始化  在helper类，先初始化为0
        // if (def->initVal) {
        //     dynamicInitValCodeGen(alloca, def->initVal);
        // } 
    } else {
        // 全局变量
        auto var = new llvm::GlobalVariable(
                *TheModule,
                TypeSystem::get(dynamic_cast<ScalarType*>(type().get())),
                false,
                llvm::GlobalValue::LinkageTypes::InternalLinkage,
                nullptr,
                m_ident.name()
        );

        // 将全局变量插入符号表
        symbolTable.insert(m_ident.name(), var);

        // 初始化
        // if (def->initVal) {
        //     llvm::Constant *initVal = constantInitValConvert(
        //             def->initVal,
        //             TypeSystem::get(type, convertArraySize(def->size))
        //     );
        //     var->setInitializer(initVal);
        // } else {
            // 未初始化的全局变量默认初始化为0
            var->setInitializer(llvm::Constant::getNullValue(
                    TypeSystem::get(dynamic_cast<ScalarType*>(type().get()))
            ));
        // }        
    }
    return nullptr;
}
llvm::Value *Block::CodeGen() {
    symbolTable.push();
    for (auto& child : m_children) {
        if (child.index() == 0) {
            auto &decl = std::get<std::unique_ptr<Declaration>>(child);
            decl->CodeGen();
        } else {
            auto &stat = std::get<std::unique_ptr<Statement>>(child);
            stat->CodeGen();
        }
    }
    symbolTable.pop();
    
    return nullptr;
}
llvm::Value *ExprStmt::CodeGen() {
    auto t = m_expr->CodeGen();
    // if (t) {
    //     t->print(llvm::errs(), false);
    // }
    return t;
}


llvm::Value *Call::CodeGen() {
    llvm::Function *function = TheModule->getFunction(m_func.name());

    if (!function) {
        throw std::runtime_error("function " + m_func.name() + " not found");
    }
    if (function->arg_size() != m_args.size()) {
        throw std::runtime_error("invalid number of params for function " + m_func.name());
    }
    std::vector<llvm::Value *> values;
    for (auto arg : m_args) {
        values.emplace_back(arg->CodeGen());
    }

    return Builder.CreateCall(function, values);
}

llvm::Value *Assignment::CodeGen() {
    llvm::Value *lhs = symbolTable.get(m_lhs->ident().name());
    llvm::Value *rhs = m_rhs->CodeGen();
    // do not do type convert?
    Builder->CreateStore(rhs, lhs);

    return nullptr;
}
llvm::Value *IfElse::CodeGen() {
    /*
    llvm::Value *value = m_cond->CodeGen();

    llvm::Function *function = Builder.GetInsertBlock()->getParent();
    auto thenBB = llvm::BasicBlock::Create(TheContext, "then");
    auto elseBB = llvm::BasicBlock::Create(TheContext, "else");
    auto mergeBB = llvm::BasicBlock::Create(TheContext, "merge");
    // what is merge
    Builder.CreateCondBr(value, thenBB, elseBB);

    bool need_merge = false;
    function->GetBasicBlockList().push_back(thenBB);
    Builder.SetInsertPoint(thenBB);
    m_then->CodeGen();
    if (Builder.GetInsertBlock()) {
        need_merge = true;
        Builder.CreateBr(mergeBB);
    }

    function->GetBasicBlockList().push_back(elseBB);
    Builder.SetInsertPoint(elseBB);
    if (m_else) {
        m_else->CodeGen();
    }
    if (Builder.GetInsertBlock()) {
        need_merge = true;
        Builder.CreateBr(mergeBB);
    }

    if (need_merge) {
        function->getBasicBlockList().push_back(elseBB);
        Builder.SetInsertPoint(mergeBB);
    }

    return nullptr;
    */
}
llvm::Value *While::CodeGen() {
    // /*
    if (!Builder->GetInsertBlock()) {
        return nullptr;
    }
    llvm::Function *function = Builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *conditionBB = llvm::BasicBlock::Create(*TheContext, "cond", function);
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(*TheContext, "body", function);
    llvm::BasicBlock *continueBB = llvm::BasicBlock::Create(*TheContext, "cont", function);

    Builder->CreateBr(conditionBB);
    // function->insert(function->end(), conditionBB); // solve below bug
    // function->getBasicBlockList().push_back(conditionBB); BUG!
    Builder->SetInsertPoint(conditionBB);

    llvm::Value *value = m_cond->CodeGen();

    // type convert: int -> bool
    value = Builder->CreateICmpNE(
        value, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0)
    );
    Builder->CreateCondBr(value, bodyBB, continueBB);

    // function->getBasicBlockList().push_back(bodyBB);
    Builder->SetInsertPoint(bodyBB);

    loops.push({conditionBB, continueBB});
    m_body->CodeGen();
    loops.pop();

    if (Builder->GetInsertBlock()) {
        Builder->CreateBr(conditionBB);
    }
    // function->getBasicBlockList().push_back(continueBB);
    Builder->SetInsertPoint(continueBB);

    return nullptr;
    // */
}
llvm::Value *Break::CodeGen() {
    /*
    if (loops.empty()) {
        throw std::runtime_error("break statement outside of loop");
    }
    if (!Builder.GetInsertBlock()) {
        return nullptr;
    }
    Builder.CreateBr(loops.top().breakBB);
    Builder.ClearInsertionPoint();
    return nullptr;
    */
}
llvm::Value *Continue::CodeGen() {
    /*
    if (loops.empty()) {
        throw std::runtime_error("continue statement outside of loop");
    }
    if (!Builder.GetInsertBlock()) {
        return nullptr;
    }
    Builder.CreateBr(loops.top().continueBB);
    Builder.ClearInsertionPoint();
    return nullptr;
    */
}
llvm::Value *Return::CodeGen() {
    std::cerr<< "return codegen\n";
    
    if (!Builder->GetInsertBlock()) {
        return nullptr;
    }

    if (m_res) {
        Builder->CreateRet(m_res->CodeGen()); // ??
    } else {
        Builder->CreateRetVoid();
    }
    Builder->ClearInsertionPoint();
    return nullptr;
}
llvm::Value *AstNode::CodeGen() {
    return nullptr;
}

void test_function() {
    
    std::vector<llvm::Type*> args(0, llvm::Type::getDoubleTy(*TheContext));
    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), args, false);
    auto function = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "myfunc", TheModule.get());
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", function);    
    Builder->SetInsertPoint(BB);
    auto L = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 100);
    auto R  = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 10);
    L->print(llvm::errs(), false);
    std::cerr<< std::endl;
    R->print(llvm::errs(), false);
    std::cerr<< std::endl;
    auto ret = Builder->CreateAdd(L, R);

    Builder->CreateRet(ret);
    // ret->print(llvm::errs(), false);
    TheModule->print(llvm::errs(), nullptr);
}
