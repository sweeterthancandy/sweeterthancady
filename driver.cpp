#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>

#include <boost/optional.hpp>
#include <boost/variant.hpp>


#if 0
struct SymbolTable{
private:
        std::unordered_map<std::string, double> m_;
};
#endif

/* 
                                Ideas
                                =====
        1) We could do something where look for common subtrees with a metric
           induced by assigning weights to nodes, IE div is 50, plus is 1, 
           Phi is 1000. Then we could count all subtrees
                        root = external-root
                        for(;;){
                                compute a statistic of counts of common subtrees
                                take the largest neaive saving, with is ( count -1 ) * metric,
                                if the subtree is not above a threshold
                                        break
                                and replace all instanace
                        }
*/

enum OperatorKind{
        OPKind_UnaryOperator,
        OPKind_BinaryOperator,
        OPKind_Constant,
        OPKind_ExogenousSymbol,
        OPKind_EndgenousSymbol,
        OPKind_Other,
};

namespace std{
        template< class T, class U > 
        std::shared_ptr<T> reinterpret_pointer_cast( const std::shared_ptr<U>& r ) noexcept
        {
                    auto p = reinterpret_cast<typename std::shared_ptr<T>::element_type*>(r.get());
                        return std::shared_ptr<T>(r, p);
        }
} // end namespace std

struct EndgenousSymbol;
using  EndgenousSymbolSet = std::unordered_set<std::shared_ptr<EndgenousSymbol const> >;
struct Operator : std::enable_shared_from_this<Operator>{

        explicit Operator(std::string const& name, OperatorKind kind = OPKind_Other)
                : name_{name}
                , kind_{kind}
        {}
        virtual ~Operator(){}

        OperatorKind Kind()const{ return kind_; }




        #if 0
        virtual double Eval(SymbolTable const& ST)const=0;
        #endif
        virtual std::shared_ptr<Operator> Diff(std::string const& symbol)const=0;
        virtual void EmitCode(std::ostream& ss)const=0;
        
        size_t Arity()const{ return children_.size(); }
        std::shared_ptr<Operator> At(size_t idx)const{
                if( Arity() < idx ){
                        throw std::domain_error("getting child that doesn't exist");
                }
                return children_.at(idx);
        }
        void Rebind(size_t idx, std::shared_ptr<Operator> const& ptr){
                if( Arity() < idx ){
                        throw std::domain_error("getting child that doesn't exist");
                }
                children_.at(idx) = ptr;
        }
        auto&       Children()     { return children_; }
        auto const& Children()const{ return children_; }

        bool IsTerminal()const{ return Arity() == 0; }
        bool IsNonTerminal()const{ return Arity() > 0; }

        std::string const& Name()const{ return name_; }

        virtual std::vector<std::string> HiddenArguments()const{ return {}; }

        inline void Display(std::ostream& ostr = std::cout)const;

        EndgenousSymbolSet EndgenousDependencies()const{
                EndgenousSymbolSet mem;
                EndgenousDependenciesCollect(mem);
                return mem;
        }
        virtual void EndgenousDependenciesCollect(EndgenousSymbolSet& mem)const{
                std::vector<std::shared_ptr<Operator const> > stack{shared_from_this()};
                for(;stack.size();){
                        auto head = stack.back();
                        stack.pop_back();
                        for(auto const& ptr : head->children_){
                                if( ptr->Kind() == OPKind_EndgenousSymbol){
                                        mem.insert(std::reinterpret_pointer_cast<EndgenousSymbol>(ptr));
                                } else {
                                        for(auto const& sub : ptr->Children() ){
                                                stack.push_back(sub);
                                        }
                                }
                        }
                }
        }
protected:
        size_t Push(std::shared_ptr<Operator> const& ptr){
                size_t slot = children_.size();
                children_.push_back(ptr);
                return slot;
        }
private:
        std::string name_;
        std::vector<std::shared_ptr<Operator> > children_;

        OperatorKind kind_;

};


struct Constant : Operator{
        Constant(double value)
                :Operator{"Constant", OPKind_Constant}
                ,value_(value)
        {}
        #if 0
        virtual double Eval(SymbolTable const& ST)const{
                return value_;
        }
        #endif
        virtual std::shared_ptr<Operator> Diff(std::string const& symbol)const{
                return Constant::Make(0.0);
        }
        virtual void EmitCode(std::ostream& ss)const{
                ss << value_;
        }

        static std::shared_ptr<Operator> Make(double value){
                return std::make_shared<Constant>(value);
        }
        virtual std::vector<std::string> HiddenArguments()const{ return { std::to_string(value_) }; }
        double Value()const{ return value_; }
private:
        double value_;
};

struct ExogenousSymbol : Operator{
        ExogenousSymbol(std::string const& name)
                :Operator{"ExogenousSymbol", OPKind_ExogenousSymbol}
                ,name_(name)
        {}
        virtual std::vector<std::string> HiddenArguments()const{ return {name_}; }
        #if 0
        virtual double Eval(SymbolTable const& ST)const{
                return ST.Lookup(name_);
        }
        #endif
        virtual std::shared_ptr<Operator> Diff(std::string const& symbol)const{
                if( symbol == name_ ){
                        return Constant::Make(1.0);
                }
                return Constant::Make(0.0);
        }
        virtual void EmitCode(std::ostream& ss)const{
                ss << name_;
        }
        std::string const& Name()const{ return name_; }
        
        static std::shared_ptr<ExogenousSymbol> Make(std::string const& symbol){
                return std::make_shared<ExogenousSymbol>(symbol);
        }
private:
        std::string name_;
};
/*
        I want a mechinism for follow statements.
        there are two cases, a symbol which represents
        something exogenous, and the case we we are
        splitting a 

 */
struct EndgenousSymbol : Operator{
        EndgenousSymbol(std::string const& name,
                        std::shared_ptr<Operator> const& expr)
                :Operator{"EndgenousSymbol", OPKind_EndgenousSymbol}
                ,name_{name}
        {
                Push(expr);
        }
        virtual std::vector<std::string> HiddenArguments()const{ return {name_, "<expr>"}; }
        virtual std::shared_ptr<Operator> Diff(std::string const& symbol)const{
                if( symbol == name_ ){
                        return Constant::Make(1.0);
                }
                return Constant::Make(0.0);
        }
        virtual void EmitCode(std::ostream& ss)const{
                ss << name_;
        }
        std::string const& Name()const{ return name_; }
        
        static std::shared_ptr<EndgenousSymbol> Make(std::string const& symbol, std::shared_ptr<Operator> const& expr){
                return std::make_shared<EndgenousSymbol>(symbol, expr);
        }

        std::shared_ptr<Operator> Expr()const{ return At(0); }
        std::shared_ptr<Operator> as_operator_()const{ return At(0); }
        
        #if 0
        virtual void EndgenousDependenciesCollect(EndgenousSymbolSet& mem)const{
                mem.insert(std::reinterpret_pointer_cast<EndgenousSymbol const>(shared_from_this()));
        }
        #endif
private:
        std::string name_;
};
        
void Operator::Display(std::ostream& ostr)const{

        std::cout << "this->Name() => " << this->Name() << "\n"; // __CandyPrint__(cxx-print-scalar,this->Name())
        for(auto const& ptr : EndgenousDependencies() ){
                std::cout << "    : " << ptr->Name() << "\n";
        }

        using ptr_t = std::shared_ptr<Operator const>;
        std::vector<std::vector<ptr_t> > stack{{shared_from_this()}};

        auto indent = [&](int extra = 0){
                return std::string((stack.size()+extra)*2,' ');
        };

        for(size_t ttl=1000;stack.size() && ttl;--ttl){

                #if 0
                std::cout << "{";
                for(size_t j=0;j!=stack.size();++j){
                        if( j!=0)
                                std::cout << ",";
                        std::cout << stack[j].size();
                }
                std::cout << "}\n";
                #endif



                auto& head = stack.back();

                if( head.empty() ){
                        stack.pop_back();
                        if( stack.size() == 0 )
                                break;
                        ostr << indent() << "}\n";
                        continue;
                }

                auto ptr = head.back();
                head.pop_back();



                auto hidden = ptr->HiddenArguments();
                if( ptr->IsTerminal() ){
                        #if 1
                        if( hidden.size() == 0 ){
                                ostr << indent() << ptr->Name() << "{}\n";
                        } else if( hidden.size() == 1 ){
                                ostr << indent() << ptr->Name() << "{" << hidden[0] << "}\n";
                        } else {
                                ostr << indent() << ptr->Name() << "{\n";
                                for(auto const& s : hidden ){
                                        ostr << indent(1) << s << "\n";
                                }
                                ostr << indent() << "}\n";
                        }
                        #endif
                } else {
                        ostr << indent() << ptr->Name() << "{\n";
                        for(auto const& s : hidden ){
                                ostr << indent(1) << s << "\n";
                        }
                        auto children = ptr->Children();
                        #if 0
                        std::cout << "stack.size() => " << stack.size() << "\n"; // __CandyPrint__(cxx-print-scalar,stack.size())
                        stack.emplace_back();
                        for(auto iter = children.rbegin(), end = children.rend();iter!=end;++iter){
                                stack.back().push_back(*iter);
                        }
                        std::cout << "stack.size() => " << stack.size() << "\n"; // __CandyPrint__(cxx-print-scalar,stack.size())
                        #endif
                        stack.emplace_back(children.rbegin(), children.rend());
                }
        }
}

enum UnaryOperatorKind{
        UOP_USUB,
};

struct UnaryOperator : Operator{
        UnaryOperator(UnaryOperatorKind op, std::shared_ptr<Operator> arg)
                :Operator{"UnaryOperator", OPKind_UnaryOperator}
                ,op_(op)
        {
                Push(arg);
        }

        UnaryOperatorKind OpKind()const{ return op_; }
        virtual std::shared_ptr<Operator> Diff(std::string const& symbol)const override
        {
                return UnaryMinus(At(0)->Diff(symbol));
        }

        static std::shared_ptr<Operator> UnaryMinus(std::shared_ptr<Operator> const& arg){
                return std::make_shared<UnaryOperator>(UOP_USUB, arg);
        }




        virtual void EmitCode(std::ostream& ss)const override{
                ss << "(-(";
                At(0)->EmitCode(ss);
                ss << "))";
        }


private:
        UnaryOperatorKind op_;
};














enum BinaryOperatorKind{
        OP_ADD,
        OP_SUB,
        OP_MUL,
        OP_DIV,
        OP_POW,
};

struct BinaryOperator : Operator{
        BinaryOperator(BinaryOperatorKind op, std::shared_ptr<Operator> left, std::shared_ptr<Operator> right)
                :Operator{"BinaryOperator", OPKind_BinaryOperator}
                ,op_(op)
        {
                Push(left);
                Push(right);
        }

        BinaryOperatorKind OpKind()const{ return op_; }
        virtual std::vector<std::string> HiddenArguments()const override{
                switch(op_){
                case OP_ADD: return {"ADD"};
                case OP_SUB: return {"SUB"};
                case OP_MUL: return {"MUL"};
                case OP_DIV: return {"DIV"};
                case OP_POW: return {"POW"};
                }
                return {"unknown_"};
        }
        std::shared_ptr<Operator> LParam()const{ return At(0); }
        std::shared_ptr<Operator> RParam()const{ return At(1); }
        #if 0
        virtual double Eval(SymbolTable const& ST)const{
                switch(op_)
                {
                case OP_ADD:
                        {
                                return left_->Eval(ST) + RParam()->Eval(ST);
                        }
                case OP_SUB:
                        {
                                return left_->Eval(ST) - RParam()->Eval(ST);
                        }
                case OP_MUL:
                        {
                                return left_->Eval(ST) * RParam()->Eval(ST);
                        }
                case OP_DIV:
                        {
                                return left_->Eval(ST) / RParam()->Eval(ST);
                        }
                }
        }
        #endif
        virtual std::shared_ptr<Operator> Diff(std::string const& symbol)const override
        {
                switch(op_)
                {
                        case OP_ADD:
                        {
                                return Add(
                                        LParam()->Diff(symbol),
                                        RParam()->Diff(symbol)
                                );
                        }
                        case OP_SUB:
                        {
                                return Sub(
                                        LParam()->Diff(symbol),
                                        RParam()->Diff(symbol)
                                );
                        }
                        case OP_MUL:
                        {
                                return Add(
                                        Mul( LParam()->Diff(symbol), RParam()),
                                        Mul( LParam(), RParam()->Diff(symbol))
                                );
                        }
                        case OP_DIV:
                        {
                                return Div(
                                        Sub(
                                                Mul(
                                                        LParam()->Diff(symbol),
                                                        RParam()
                                                ),
                                                Mul(
                                                        LParam(),
                                                        RParam()->Diff(symbol)
                                                )
                                        ),
                                        Pow(
                                                RParam(),
                                                Constant::Make(2.0)
                                        )
                                );

                        }
                        case OP_POW:
                        {
                                // lets assume that the exponent is indpedent
                                // of the deriviative for now
                                //
                                // f(x)^C = C * f(x)*(C-1) * f'(x)
                                // ~ left ^ right
                                //
                                return Mul(
                                        RParam(),
                                        Mul(
                                                Pow(
                                                        LParam(),
                                                        Sub(
                                                                RParam(),
                                                                Constant::Make(1.0)
                                                        )
                                                ),
                                                LParam()->Diff(symbol)
                                        )
                                );
                        }
                }
        }


        virtual void EmitCode(std::ostream& ss)const override{
                if( op_ == OP_POW ){
                        ss << "std::pow(";
                        LParam()->EmitCode(ss);
                        ss << ", ";
                        RParam()->EmitCode(ss);
                        ss << ")";
                } else {
                        ss << "(";
                        ss << "(";
                        LParam()->EmitCode(ss);
                        ss << ")";
                        switch(op_){
                        case OP_ADD: ss << "+"; break;
                        case OP_SUB: ss << "-"; break;
                        case OP_MUL: ss << "*"; break;
                        case OP_DIV: ss << "/"; break;
                        }
                        ss << "(";
                        RParam()->EmitCode(ss);
                        ss << ")";
                        ss << ")";
                }
        }


        static std::shared_ptr<Operator> Add(std::shared_ptr<Operator> const& left,
                                             std::shared_ptr<Operator> const& right)
        {
                return std::make_shared<BinaryOperator>(OP_ADD, left, right);
        }
        static std::shared_ptr<Operator> Sub(std::shared_ptr<Operator> const& left,
                                             std::shared_ptr<Operator> const& right)
        {
                return std::make_shared<BinaryOperator>(OP_SUB, left, right);
        }
        static std::shared_ptr<Operator> Mul(std::shared_ptr<Operator> const& left,
                                             std::shared_ptr<Operator> const& right)
        {
                return std::make_shared<BinaryOperator>(OP_MUL, left, right);
        }
        static std::shared_ptr<Operator> Div(std::shared_ptr<Operator> const& left,
                                             std::shared_ptr<Operator> const& right)
        {
                return std::make_shared<BinaryOperator>(OP_DIV, left, right);
        }
        static std::shared_ptr<Operator> Pow(std::shared_ptr<Operator> const& left,
                                             std::shared_ptr<Operator> const& right)
        {
                return std::make_shared<BinaryOperator>(OP_POW, left, right);
        }

private:
        BinaryOperatorKind op_;
};

struct Exp : Operator{
        Exp(std::shared_ptr<Operator> arg)
                :Operator{"Exp"}
        {
                Push(arg);
        }
        virtual std::shared_ptr<Operator> Diff(std::string const& symbol)const{
                return BinaryOperator::Mul(
                        std::make_shared<Exp>(At(0)),
                        At(0)->Diff(symbol));
        }
        virtual void EmitCode(std::ostream& ss)const{
                ss << "std::exp(";
                At(0)->EmitCode(ss);
                ss << ")";
        }
        static std::shared_ptr<Exp> Make(std::shared_ptr<Operator> const& arg){
                return std::make_shared<Exp>(arg);
        }
};

struct Log : Operator{
        Log(std::shared_ptr<Operator> arg)
                :Operator{"Log"}
        {
                Push(arg);
        }
        virtual std::shared_ptr<Operator> Diff(std::string const& symbol)const{
                return BinaryOperator::Div(
                        At(0)->Diff(symbol),
                        At(0));
        }
        virtual void EmitCode(std::ostream& ss)const{
                ss << "std::log(";
                At(0)->EmitCode(ss);
                ss << ")";
        }
        static std::shared_ptr<Log> Make(std::shared_ptr<Operator> const& arg){
                return std::make_shared<Log>(arg);
        }
};


// normal distribution CFS
struct Phi : Operator{
        Phi(std::shared_ptr<Operator> arg)
                :Operator{"Phi"}
        {
                Push(arg);
        }
        virtual std::shared_ptr<Operator> Diff(std::string const& symbol)const{
                // f(x) = 1/\sqrt{2 \pi} \exp{-\frac{1}{2}x^2}

                return 
                        BinaryOperator::Mul(
                                BinaryOperator::Div(
                                        Exp::Make(
                                                BinaryOperator::Sub(
                                                        Constant::Make(0.0),
                                                        BinaryOperator::Mul(
                                                                Constant::Make(0.5),
                                                                BinaryOperator::Pow(
                                                                        At(0),
                                                                        Constant::Make(2.0)
                                                                )
                                                        )
                                                )
                                        ),
                                        Constant::Make(2.506628274631000502415765284811045253006986740609938316629)
                                ),
                                At(0)->Diff(symbol)
                        );

        }
        virtual void EmitCode(std::ostream& ss)const{
                // std::erfc(-x/std::sqrt(2))/2
                ss << "std::erfc(-(";
                At(0)->EmitCode(ss);
                ss << ")/std::sqrt(2))/2";
        }

        static std::shared_ptr<Operator> Make(std::shared_ptr<Operator> const& arg){
                return std::make_shared<Phi>(arg);
        }
};

#if 0
struct Statement : EndgenousSymbol{
        Statement(std::string const& name, std::shared_ptr<Operator> expr)
                :ExogenousSymbol(name),
                expr_(expr)
        {}
        std::shared_ptr<Operator> Expr()const{ return expr_; }
        std::shared_ptr<Operator> as_operator_()const{ return expr_; }
private:
        std::string name_;
        std::shared_ptr<Operator> expr_;
};
#endif

struct TemporaryAllocator{
        std::string Allocate(){
                std::stringstream ss;
                ss << prefix_ << index_;
                ++index_;
                return ss.str();
        }
private:
        std::string prefix_{"__temp_"};
        size_t index_{0};
};

struct Function{
        explicit Function(std::string const& name):name_{name}{}
        void AddArgument(std::string const& symbol){
                args_.push_back(symbol);
        }
        auto AddStatement(std::shared_ptr<EndgenousSymbol> stmt){
                stmts_.push_back(stmt);
                return stmt;
        }
        auto const& Arguments()const{ return args_; }
        auto const& Statements()const{ return stmts_; }
        std::string const& Name()const{ return name_; }

private:
        std::string name_;
        std::vector<std::string> args_;
        std::vector<std::shared_ptr<EndgenousSymbol> > stmts_;
};

struct ConstantDescription{
        ConstantDescription(std::shared_ptr<Operator> root){
                if( root->Kind() != OPKind_Constant )
                        return;
                auto constant = static_cast<Constant*>(root.get());
                opt_value_ = constant->Value();
        }
        bool IsZero()const{ return opt_value_ && (*opt_value_ == 0.0 || *opt_value_ == -0.0); }
        bool IsOne()const{ return opt_value_ && *opt_value_ == 1.0; }
        bool IsConstantValue()const{ return !! opt_value_; }
        double ValueOrThrow()const{
                if( ! opt_value_ )
                        throw std::domain_error("have no value");
                return opt_value_.get();
        }
private:
        boost::optional<double> opt_value_;
};

struct FoldZero{

        std::shared_ptr<Operator> Fold(std::shared_ptr<Operator> root){
                if( root->Kind() == OPKind_BinaryOperator ){
                        auto bin_op = static_cast<BinaryOperator*>(root.get());

                        auto left_folded = this->Fold(bin_op->At(0));
                        auto right_folded = this->Fold(bin_op->At(1));

                        auto left_desc  = ConstantDescription{left_folded};
                        auto right_desc = ConstantDescription{right_folded};


                        switch(bin_op->OpKind())
                        {
                                case OP_ADD:
                                {
                                        if( left_desc.IsConstantValue() && 
                                            right_desc.IsConstantValue() ){
                                                double sum = 0.0;
                                                sum += left_desc.ValueOrThrow();
                                                sum += right_desc.ValueOrThrow();
                                                return Constant::Make(sum);
                                        }
                                        if( left_desc.IsZero() )
                                                return right_folded;
                                        if( right_desc.IsZero() )
                                                return left_folded;


                                        break;
                                }
                                case OP_SUB:
                                {
                                        if( left_desc.IsConstantValue() && 
                                            right_desc.IsConstantValue() ){
                                                double sum = 0.0;
                                                sum += left_desc.ValueOrThrow();
                                                sum -= right_desc.ValueOrThrow();
                                                return Constant::Make(sum);
                                        }

                                        if( right_desc.IsZero())
                                                return left_folded;

                                        if( left_desc.IsZero() ){
                                                return UnaryOperator::UnaryMinus(right_folded);
                                        }
                                        break;
                                }
                                case OP_MUL:
                                {
                                        if( left_desc.IsZero() || right_desc.IsZero() ){
                                                return Constant::Make(0.0);
                                        }
                                        if( left_desc.IsOne() )
                                                return right_folded;
                                        if( right_desc.IsOne() )
                                                return left_folded;
                                        break;
                                }
                                case OP_DIV:
                                {
                                        if( ! left_desc.IsZero() ){
                                                if( right_desc.IsZero() ){
                                                        throw std::domain_error("have divide by zero");
                                                }
                                        } else {
                                                return Constant::Make(0.0);
                                        }
                                        break;
                                }
                                case OP_POW:
                                {
                                        if( left_desc.IsConstantValue() && 
                                            right_desc.IsConstantValue() ){
                                                return Constant::Make(
                                                        std::pow(
                                                                left_desc.ValueOrThrow()
                                                                ,right_desc.ValueOrThrow()
                                                        )
                                                );
                                        }

                                        if( left_desc.IsZero() )
                                                return Constant::Make(0.0);

                                        if( right_desc.IsOne() ){
                                                return left_folded;
                                        }
                                        if( right_desc.IsZero() ){
                                                return Constant::Make(1.0);
                                        }
                                        break;
                                }
                        }
                        return std::make_shared<BinaryOperator>(
                                bin_op->OpKind(),
                                left_folded,
                                right_folded);
                }
                
                if( root->Kind() == OPKind_UnaryOperator ){
                        auto unary_op = static_cast<UnaryOperator*>(root.get());

                        auto folded_arg = this->Fold(unary_op->At(0));

                        auto arg_desc  = ConstantDescription{folded_arg};

                        if( arg_desc.IsZero() ){
                                return Constant::Make(0.0);
                        }

                        return UnaryOperator::UnaryMinus(folded_arg);
                }

                if( root->IsNonTerminal() ){
                        for(size_t idx=0;idx!=root->Arity();++idx){
                                auto folded = this->Fold(root->At(idx));
                                root->Rebind(idx, folded);
                        }
                }
                return root;
        }
};


struct StringCodeGenerator{
        void Emit(std::ostream& ss, Function const& f)const{

                FoldZero folder;

                // we have a vector [ x1, x2, ... ] which are the function 
                // parameters. 

                struct VariableInfo{
                        VariableInfo(std::string const& name)
                                : name_{name}
                        {}
                        std::string const& Name()const{ return name_; }
                        boost::optional<std::shared_ptr<Operator> > GetDiffLexical(std::string const& symbol)const{
                                auto iter = diff_map_.find(symbol);
                                if( iter == diff_map_.end() )
                                        return {};
                                return iter->second;
                        }
                        void MapDiff(std::string const& symbol,
                                    std::shared_ptr<Operator> const& value){
                                diff_map_[symbol] = value;
                        }
                private:
                        std::string name_;
                        std::unordered_map<std::string, std::shared_ptr<Operator> > diff_map_;
                };


                auto to_diff = f.Arguments();


                std::vector<std::shared_ptr<VariableInfo> > deps;
                for( auto const& arg : f.Arguments() ){
                        auto ptr = std::make_shared<VariableInfo>(arg);
                        for( auto const& inner_arg : to_diff ){
                                if( arg == inner_arg ){
                                        ptr->MapDiff(inner_arg, Constant::Make(1.0));
                                } else {
                                        ptr->MapDiff(inner_arg, Constant::Make(0.0));
                                }
                        }
                        deps.push_back(ptr);
                }



                ss << "double " << f.Name() << "(";
                for(size_t idx=0;idx!=deps.size();++idx){
                        if( idx != 0 ) 
                                ss << ", ";
                        ss << "double " << deps[idx]->Name();
                        ss << ", double* " << "d_" + deps[idx]->Name();
                }

                ss << ")\n";
                ss << "{\n";

                std::string indent = "    ";

                TemporaryAllocator temp_alloc;

                for(size_t idx=0;idx!=f.Statements().size();++idx){
                        // for each statement we need to add two calculations to the
                        // infomation
                        //      statement = expr
                        //      for each X in to-diff:
                        //        d_statement_X = D[X](expr)
                        //      
                        //    

                        //

                        auto const& stmt = f.Statements()[idx];
                        auto const& expr = stmt->Expr();
                        
                        auto stmt_dep = std::make_shared<VariableInfo>(stmt->Name());
                        
                        #if 0
                        ss << indent << "/* expr\n";
                        expr->Display(ss);
                        ss << indent << "*/\n";
                        #endif
                        ss << indent << "double " << stmt_dep->Name() << " = ";
                        expr->EmitCode(ss);
                        ss << ";\n";


                        for( auto const& d_symbol : to_diff ){
                                std::vector<std::string> subs;

                                for( auto const& info : deps ){

                                        auto temp_name = temp_alloc.Allocate();

                                         ;

                                        // \partial stmt / \partial symbol d symbol
                                        auto sub_diff = BinaryOperator::Mul(
                                                expr->Diff( info->Name() ),
                                                *info->GetDiffLexical(d_symbol));

                                        sub_diff = folder.Fold(sub_diff);

                                        if( sub_diff->Kind() == OPKind_Constant  ){
                                                auto constant = reinterpret_cast<Constant*>(sub_diff.get());
                                                if( constant->Value() == 0.0 )
                                                        continue;
                                        }


                                        #if 0
                                        ss << indent << "// \\partial " << stmt->Name() << " / \\partial " << info->Name() << " d " << info->Name() << "\n";
                                        #endif
                                        #if 0
                                        ss << indent << "/* expr\n";
                                        sub_diff->Display(ss);
                                        ss << indent << "*/\n";
                                        #endif
                                        ss << indent << "double " << temp_name << " = ";
                                        sub_diff->EmitCode(ss);
                                        ss << ";\n";

                                        subs.push_back(temp_name);
                                }


                                std::string token = "__diff_" + stmt->Name() + "_" + d_symbol;
                                stmt_dep->MapDiff( d_symbol, ExogenousSymbol::Make(token));


                                ss << indent << "double " << token << " = ";
                                if( subs.size() ){
                                        for(size_t idx=0;idx!=subs.size();++idx){
                                                if( idx != 0 )
                                                        ss << " + ";
                                                ss << subs[idx];
                                        }
                                } else {
                                        ss << "0.0";
                                }
                                ss << ";\n";
                        }
                        ss << "\n\n\n";
                        deps.emplace_back(stmt_dep);

                }
                        
                for( auto const& d_symbol : to_diff ){
                        ss << indent << "*d_" + d_symbol << " = " << reinterpret_cast<ExogenousSymbol*>(deps.back()->GetDiffLexical(d_symbol).get().get())->Name() << ";\n";
                }

                ss << indent << "return " << deps.back()->Name() << ";\n";
                ss << "}\n";

        }
};

void example_0(){


        Function f("f");
        f.AddArgument("x");
        f.AddArgument("y");

        //auto expr_0 = BinaryOperator::Mul(Log::Make(BinaryOperator::Mul(ExogenousSymbol::Make("x"),ExogenousSymbol::Make("x"))),  Exp::Make(ExogenousSymbol::Make("y")));
        //auto expr_0 = BinaryOperator::Pow(ExogenousSymbol::Make("x"), Constant::Make(2));
        auto expr_0 = Phi::Make(BinaryOperator::Pow(ExogenousSymbol::Make("x"), Constant::Make(3)));

        auto stmt_0 = std::make_shared<EndgenousSymbol>("stmt0", expr_0);

        f.AddStatement(stmt_0);

        std::ofstream fstr("prog.cxx");
        fstr << R"(
#include <cstdio>
#include <cmath>
)";

        StringCodeGenerator cg;
        cg.Emit(fstr, f);
        fstr << R"(

int main(){
        double x_min = 0.1;
        double x_max = +2.0;
        double y_min = -2.0;
        double y_max = +2.0;

        double epsilon = 1e-10;
        double increment = 0.05;

        

        for(double x =x_min; x <= x_max + increment /2; x += increment ){
                for(double y =y_min; y <= y_max + increment /2; y += increment ){
                        double d_x = 0.0;
                        double d_y = 0.0;

                        double value = f(x, &d_x, y, &d_y);

                        double dummy;
                        double x_lower = f(x - epsilon /2 , &dummy, y, &dummy);
                        double x_upper = f(x + epsilon /2 , &dummy, y, &dummy);
                        double x_finite_diff = ( x_upper - x_lower ) / epsilon;
                        double x_residue = d_x - x_finite_diff;
                        
                        double y_lower = f(x, &dummy, y - epsilon /2 , &dummy);
                        double y_upper = f(x, &dummy, y + epsilon /2 , &dummy);
                        double y_finite_diff = ( y_upper - y_lower ) / epsilon;
                        double y_residue = d_y - y_finite_diff;
                        
                        //printf("%f,%f,%f,%f,%f,%f\n", x, y, d_x, d_y, x_finite_diff, x_residue);
                        printf("%f,%f,%f => %f,%f,%f => %f,%f,%f\n", x, y,value, d_x, x_finite_diff,x_residue, d_y, y_finite_diff,y_residue);
                }


        }

}
)";
}

namespace Frontend{

        struct ImbueWith{};

        struct WithOperators{
                WithOperators(std::shared_ptr<Operator> impl)
                        :impl_(impl)
                {}
                std::shared_ptr<Operator> as_operator_()const{
                        return impl_;
                }
        private:
                std::shared_ptr<Operator> impl_;
        };

        namespace Detail{

                template<size_t Precedence>
                struct PrecedenceDevice : PrecedenceDevice<Precedence-1>{};
                template<>
                struct PrecedenceDevice<0>{};

                template<
                        class T,
                        class = std::__void_t<
                                typename std::enable_if<
                                        std::is_same<const char*, typename std::decay<T>::type>::value ||
                                        std::is_same<char*      , typename std::decay<T>::type>::value ||
                                        std::is_same<std::string, typename std::decay<T>::type>::value
                                >::type
                        >
                >
                std::shared_ptr<Operator> AsOperatorImpl(T&& t, PrecedenceDevice<7>&&){
                        return ExogenousSymbol::Make(t);
                }
                template<
                        class T,
                        class = std::__void_t<
                                typename std::enable_if<
                                        std::is_floating_point<typename std::decay<T>::type>::value ||
                                        std::is_integral<typename std::decay<T>::type>::value
                                >::type
                        >
                >
                std::shared_ptr<Operator> AsOperatorImpl(T&& t, PrecedenceDevice<8>&&){
                        return Constant::Make(t);
                }
                template<
                        class T,
                        class = std::__void_t<decltype(std::declval<T>()->as_operator_())>
                >
                std::shared_ptr<Operator> AsOperatorImpl(T&& t, PrecedenceDevice<9>&&){
                        return t->as_operator_();
                }
                template<
                        class T,
                        class = std::__void_t<decltype(std::declval<T>().as_operator_())>
                >
                std::shared_ptr<Operator> AsOperatorImpl(T&& t, PrecedenceDevice<10>&&){
                        return t.as_operator_();
                }
                template<
                        class T,
                        class = std::__void_t<
                                typename std::enable_if<
                                        std::is_same<
                                                std::shared_ptr<Operator>,
                                                typename std::decay<T>::type
                                        >::value
                                >::type
                        >
                >
                std::shared_ptr<Operator> AsOperatorImpl(T&& t, PrecedenceDevice<20>&&){
                        return t;
                }

        }  // end namespace Detail
        
        template<class T>
        std::shared_ptr<Operator> AsOperator(T&& t){
                return Detail::AsOperatorImpl(
                        std::forward<T>(t),
                        Detail::PrecedenceDevice<100>{}
                );
        }

        #define FRONTEND_DEFINE_OPERATOR(LEXICAL_TOKEN, MAPPED_FUNCTION) \
        template<                                                        \
                class L,                                                 \
                class R,                                                 \
                class = std::__void_t<                                   \
                        decltype( AsOperator(std::declval<L>()) ),       \
                        decltype( AsOperator(std::declval<R>()) )        \
                >                                                        \
        >                                                                \
        auto operator LEXICAL_TOKEN(L&& l, R&& r){                       \
                return WithOperators{                                    \
                        BinaryOperator::MAPPED_FUNCTION(                 \
                                 AsOperator(l),                          \
                                 AsOperator(r)                           \
                        )                                                \
                };                                                       \
        }
        FRONTEND_DEFINE_OPERATOR(+, Add)
        FRONTEND_DEFINE_OPERATOR(-, Sub)
        FRONTEND_DEFINE_OPERATOR(*, Mul)
        FRONTEND_DEFINE_OPERATOR(/, Div)
        FRONTEND_DEFINE_OPERATOR(^, Pow)

        template<
                class Arg,
                class = std::__void_t<
                        decltype( AsOperator(std::declval<Arg>()) )
                >
        >
        auto operator -(Arg&& arg){
                return WithOperators{
                        UnaryOperator::UnaryMinus(
                                 AsOperator(arg)
                        )
                };
        }

        inline auto Var(std::string const& name){
                return WithOperators{ExogenousSymbol::Make(name)};
        }
        template<class T>
        inline auto Log(T&& arg){
                return WithOperators{ Log::Make( AsOperator(arg) ) };
        }
        template<class T>
        inline auto Exp(T&& arg){
                return WithOperators{ Exp::Make( AsOperator(arg) ) };
        }
        template<class T>
        inline auto Phi(T&& arg){
                return WithOperators{ Phi::Make( AsOperator(arg) ) };
        }


        template<class L, class R>
        inline auto Pow(L&& l, R&& r){
                return WithOperators{ 
                        BinaryOperator::Pow( AsOperator(l), AsOperator(r) )
                };
        }

        template<class Arg>
        inline auto Stmt(std::string const& name, Arg&& arg){
                auto ptr = std::make_shared<EndgenousSymbol>(name, AsOperator(arg));
                return ptr;
        }

        struct Double : WithOperators{
                template<class Expr>
                // non-explicit so we can assign more matural
                //      Double d = ...;
                Double(Expr&& expr)
                        : WithOperators{AsOperator(std::forward<Expr>(expr))}
                {}
        };
        #if 0
        struct Var{
                Var(std::string const& name):
                        impl_{ExogenousSymbol::Make(name)}
                {}
                std::shared_ptr<Operator> as_operator_()const{
                        return impl_;
                }
        private:
                std::shared_ptr<Operator> impl_;
        };
        #endif


} // end namespace Frontend

void example_1(){


        Function f("f");
        f.AddArgument("a");
        f.AddArgument("b");
        f.AddArgument("x");


        auto expr_0 = BinaryOperator::Mul( ExogenousSymbol::Make("a"), BinaryOperator::Mul(ExogenousSymbol::Make("x"),  ExogenousSymbol::Make("x")));


        auto stmt_0 = std::make_shared<EndgenousSymbol>("stmt0", expr_0);

        auto expr_1 = BinaryOperator::Add( ExogenousSymbol::Make(stmt_0->Name()), ExogenousSymbol::Make("b"));

        auto stmt_1 = std::make_shared<EndgenousSymbol>("stmt1", expr_1);
        f.AddStatement(stmt_0);
        f.AddStatement(stmt_1);

        std::ofstream fstr("prog.c");
        StringCodeGenerator cg;
        cg.Emit(fstr, f);
        fstr << R"(
#include <stdio.h>
int main(){
        double a = 2.0;
        double b = 3.0;

        double epsilon = 1e-10;
        double increment = 0.05;

        

        for(double x =0.0; x <= 2.0 + increment /2; x += increment ){
                double d_a = 0.0;
                double d_b = 0.0;
                double d_x = 0.0;

                double y = f(a, &d_a, b, &d_b, x, &d_x);

                double dummy;
                double lower = f(a, &dummy, b, &dummy, x - epsilon/2, &dummy);
                double upper = f(a, &dummy, b, &dummy, x + epsilon/2, &dummy);
                double finite_diff = ( upper - lower ) / epsilon;
                double residue = d_x - finite_diff;
                
                printf("%f,%f,%f,%f,%f,%f,%f\n", x, y, d_a, d_b, d_x, finite_diff, residue);


        }

}
)";
}

void black_scholes(){


        Function f("black");
        f.AddArgument("t");
        f.AddArgument("T");
        f.AddArgument("r");
        f.AddArgument("S");
        f.AddArgument("K");
        f.AddArgument("vol");

        auto time_to_expiry = BinaryOperator::Sub(
                ExogenousSymbol::Make("T"),
                ExogenousSymbol::Make("t")
        );

        auto deno = BinaryOperator::Div( 
                Constant::Make(1.0),
                BinaryOperator::Mul(
                        ExogenousSymbol::Make("vol"),
                        BinaryOperator::Pow(
                                time_to_expiry,
                                Constant::Make(0.5)
                        )
                )
        );

        auto d1 = BinaryOperator::Mul(
                deno,
                BinaryOperator::Add(
                        Log::Make(
                                BinaryOperator::Div(
                                        ExogenousSymbol::Make("S"),
                                        ExogenousSymbol::Make("K")
                                )
                        ),
                        BinaryOperator::Mul(
                                BinaryOperator::Add(
                                        ExogenousSymbol::Make("r"),
                                        BinaryOperator::Div(
                                                BinaryOperator::Pow(
                                                        ExogenousSymbol::Make("vol"),
                                                        Constant::Make(2.0)
                                                ),
                                                Constant::Make(2.0)
                                        )
                                ),
                                time_to_expiry
                        )
                )
        );

        auto stmt_0 = std::make_shared<EndgenousSymbol>("stmt0", d1);

        
        auto d2 = BinaryOperator::Sub(
                ExogenousSymbol::Make(stmt_0->Name()),
                BinaryOperator::Mul(
                        ExogenousSymbol::Make("vol"),
                        time_to_expiry
                )
        );


        auto stmt_1 = std::make_shared<EndgenousSymbol>("stmt1", d2);
        
        auto pv = BinaryOperator::Mul(
                ExogenousSymbol::Make("K"),
                Exp::Make(
                        BinaryOperator::Mul(
                                BinaryOperator::Sub(
                                        Constant::Make(0.0),
                                        ExogenousSymbol::Make("r")
                                ),
                                time_to_expiry
                        )
                )
        );
        
        auto stmt_2 = std::make_shared<EndgenousSymbol>("stmt2", pv);

        auto black = BinaryOperator::Sub(
                BinaryOperator::Mul(
                        Phi::Make(stmt_0),
                        ExogenousSymbol::Make("S")
                ),
                BinaryOperator::Mul(
                        Phi::Make(stmt_1),
                        stmt_2
                )
        );

        auto stmt_3 = std::make_shared<EndgenousSymbol>("stmt3", black);


        f.AddStatement(stmt_0);
        f.AddStatement(stmt_1);
        f.AddStatement(stmt_2);
        f.AddStatement(stmt_3);

        std::ofstream fstr("prog.cxx");
        fstr << R"(
#include <cstdio>
#include <cmath>
)";

        StringCodeGenerator cg;
        cg.Emit(fstr, f);
        fstr << R"(

double black_fd(double epsilon, double t, double d_t, double T, double d_T, double r, double d_r, double S, double d_S, double K, double d_K, double vol, double d_vol){
        double dummy;
        double lower = black( t - d_t*epsilon/2 , &dummy, T - d_T*epsilon/2  , &dummy, r - d_r*epsilon/2  , &dummy, S - d_S*epsilon/2  , &dummy, K - d_K*epsilon/2  , &dummy, vol - d_vol*epsilon/2, &dummy);
        double upper = black( t + d_t*epsilon/2 , &dummy, T + d_T*epsilon/2  , &dummy, r + d_r*epsilon/2  , &dummy, S + d_S*epsilon/2  , &dummy, K + d_K*epsilon/2  , &dummy, vol + d_vol*epsilon/2, &dummy);
        double finite_diff = ( upper - lower ) / epsilon;
        return finite_diff;
}
int main(){
        double t   = 0.0;
        double T   = 10.0;
        double r   = 0.04;
        double S   = 50;
        double K   = 60;
        double vol = 0.2;

        double epsilon = 1e-10;

        double d_t = 0.0;
        double d_T = 0.0;
        double d_r = 0.0;
        double d_S = 0.0;
        double d_K = 0.0;
        double d_vol = 0.0;
        double value = black( t  , &d_t, T  , &d_T, r  , &d_r, S  , &d_S, K  , &d_K, vol, &d_vol);

        double d1 = 1/ ( vol * std::sqrt(T - t)) *  ( std::log(S/K) + ( r + vol*vol/2)*(T-t));

        double dummy;
        double lower = black( t - epsilon/2 , &dummy, T  , &dummy, r  , &dummy, S  , &dummy, K  , &dummy, vol, &dummy);
        double upper = black( t + epsilon/2 , &dummy, T  , &dummy, r  , &dummy, S  , &dummy, K  , &dummy, vol, &dummy);
        double finite_diff = ( upper - lower ) / epsilon;
        double residue = d_t - finite_diff;

        printf("%f,%f,%f,%f,%f,%f => %f,%f => %f,%f,%f\n", t, T, r, S, K, vol, value, d1, d_t, finite_diff, residue);

        printf("d[t]  ,%f,%f\n", d_t  ,  black_fd(epsilon, t, 1, T  , 0, r  , 0, S  , 0, K  , 0, vol, 0));
        printf("d[T]  ,%f,%f\n", d_T  ,  black_fd(epsilon, t, 0, T  , 1, r  , 0, S  , 0, K  , 0, vol, 0));
        printf("d[r]  ,%f,%f\n", d_r  ,  black_fd(epsilon, t, 0, T  , 0, r  , 1, S  , 0, K  , 0, vol, 0));
        printf("d[S]  ,%f,%f\n", d_S  ,  black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 1, K  , 0, vol, 0));
        printf("d[K]  ,%f,%f\n", d_K  ,  black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 0, K  , 1, vol, 0));
        printf("d[vol],%f,%f\n", d_vol,  black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 0, K  , 0, vol, 1));
        

}
)";
}
















void black_scholes_frontend(){

        using namespace Frontend;
        using Frontend::Log;
        using Frontend::Exp;
        using Frontend::Phi;


        Function f("black");
        f.AddArgument("t");
        f.AddArgument("T");
        f.AddArgument("r");
        f.AddArgument("S");
        f.AddArgument("K");
        f.AddArgument("vol");

        auto d1    = f.AddStatement(Stmt("d1"   , (1.0 / ( Var("vol") * ((Var("T") - Var("t")) ^ 0.5) )) * ( Log(Var("S") / "K") +   ("r" + ( Var("vol") ^ 2.0 ) / 2 ) * (Var("T") - Var("t")) )));
        auto d2    = f.AddStatement(Stmt("d2"   , d1 - "vol" * (Var("T") - Var("t"))));
        auto pv    = f.AddStatement(Stmt("pv"   , "K" * Exp( -Var("r") * ( Var("T") - Var("t") ) )));
        auto black = f.AddStatement(Stmt("black", Phi(d1) * "S" - Phi(d2) * pv));

        std::ofstream fstr("prog.cxx");
        fstr << R"(
#include <cstdio>
#include <cmath>
)";

        StringCodeGenerator cg;
        cg.Emit(fstr, f);
        fstr << R"(

double black_fd(double epsilon, double t, double d_t, double T, double d_T, double r, double d_r, double S, double d_S, double K, double d_K, double vol, double d_vol){
        double dummy;
        double lower = black( t - d_t*epsilon/2 , &dummy, T - d_T*epsilon/2  , &dummy, r - d_r*epsilon/2  , &dummy, S - d_S*epsilon/2  , &dummy, K - d_K*epsilon/2  , &dummy, vol - d_vol*epsilon/2, &dummy);
        double upper = black( t + d_t*epsilon/2 , &dummy, T + d_T*epsilon/2  , &dummy, r + d_r*epsilon/2  , &dummy, S + d_S*epsilon/2  , &dummy, K + d_K*epsilon/2  , &dummy, vol + d_vol*epsilon/2, &dummy);
        double finite_diff = ( upper - lower ) / epsilon;
        return finite_diff;
}
int main(){
        double t   = 0.0;
        double T   = 10.0;
        double r   = 0.04;
        double S   = 50;
        double K   = 60;
        double vol = 0.2;

        double epsilon = 1e-10;

        double d_t = 0.0;
        double d_T = 0.0;
        double d_r = 0.0;
        double d_S = 0.0;
        double d_K = 0.0;
        double d_vol = 0.0;
        double value = black( t  , &d_t, T  , &d_T, r  , &d_r, S  , &d_S, K  , &d_K, vol, &d_vol);

        double d1 = 1/ ( vol * std::sqrt(T - t)) *  ( std::log(S/K) + ( r + vol*vol/2)*(T-t));

        double dummy;
        double lower = black( t - epsilon/2 , &dummy, T  , &dummy, r  , &dummy, S  , &dummy, K  , &dummy, vol, &dummy);
        double upper = black( t + epsilon/2 , &dummy, T  , &dummy, r  , &dummy, S  , &dummy, K  , &dummy, vol, &dummy);
        double finite_diff = ( upper - lower ) / epsilon;
        double residue = d_t - finite_diff;

        printf("%f,%f,%f,%f,%f,%f => %f,%f => %f,%f,%f\n", t, T, r, S, K, vol, value, d1, d_t, finite_diff, residue);

        printf("d[t]  ,%f,%f\n", d_t  ,  black_fd(epsilon, t, 1, T  , 0, r  , 0, S  , 0, K  , 0, vol, 0));
        printf("d[T]  ,%f,%f\n", d_T  ,  black_fd(epsilon, t, 0, T  , 1, r  , 0, S  , 0, K  , 0, vol, 0));
        printf("d[r]  ,%f,%f\n", d_r  ,  black_fd(epsilon, t, 0, T  , 0, r  , 1, S  , 0, K  , 0, vol, 0));
        printf("d[S]  ,%f,%f\n", d_S  ,  black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 1, K  , 0, vol, 0));
        printf("d[K]  ,%f,%f\n", d_K  ,  black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 0, K  , 1, vol, 0));
        printf("d[vol],%f,%f\n", d_vol,  black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 0, K  , 0, vol, 1));
        

}
)";
}

namespace MathFunctions{

        inline double Phi(double x){
                return std::erfc(-x/std::sqrt(2))/2;
        }
        inline double Exp(double x){
                return std::exp(x);
        }
        inline double Pow(double x, double y){
                return std::pow(x,y);
        }
        inline double Log(double x){
                return std::log(x);
        }

        using Frontend::Phi;
        using Frontend::Exp;
        using Frontend::Pow;
        using Frontend::Log;

} // end namespace MathFunctions

struct BlackScholesCallOption{
        template<class Double>
        struct Build{
                Double Evaluate(
                        Double t,
                        Double T,
                        Double r,
                        Double S,
                        Double K,
                        Double vol )const
                {
                        using MathFunctions::Phi;
                        using MathFunctions::Exp;
                        using MathFunctions::Pow;
                        using MathFunctions::Log;

                        Double d1 = ((1.0 / ( vol * Pow((T - t),0.5) )) * ( Log(S / K) +   (r + ( Pow(vol,2.0) ) / 2 ) * (T - t) ));
                        Double d2 = d1 - vol * (T - t);
                        Double pv = K * Exp( -r * ( T - t ) );
                        Double black = Phi(d1) * S - Phi(d2) * pv;
                        return black;
                }
        };
};


/*
        Here we want to make a massive distincition between lvalue and rvalue

        each lvalue is single assignment, and maps to statements in the code,
        the idea is that we might have something like this

                a = f(x,y,z)
                b = a * ( a - 1 ) * (a -2 ) * ( a -3 )

        mathematically, the above is nothing more than the expanded expression,
        however we want to allow the creation of statements. 
 */
struct DoubleKernelImpl{
        virtual ~DoubleKernelImpl()=default;
        virtual std::shared_ptr<Operator> as_operator_()const=0;
};
struct DoubleKernelOperator : DoubleKernelImpl{
        DoubleKernelOperator(std::shared_ptr<Operator> ptr)
                :operator_(ptr)
        {}
        virtual std::shared_ptr<Operator> as_operator_()const{ return operator_; }
private:
        std::shared_ptr<Operator> operator_;
};
struct DoubleKernel : Frontend::ImbueWith{
        static std::string Tag(){
                static size_t counter = 0;
                std::stringstream ss;
                ss << "__statement_" << counter;
                ++counter;
                return ss.str();
        }
        template< class Expr >
        DoubleKernel(Expr&& expr)
                : impl_{std::make_shared<DoubleKernelOperator>(EndgenousSymbol::Make(Tag(), Frontend::AsOperator(expr)))}
        {}
        struct Dispatch_Exo{};
        DoubleKernel( Dispatch_Exo&&, std::shared_ptr<Operator> const& op)
                : impl_{std::make_shared<DoubleKernelOperator>(op)}
        {}
        static DoubleKernel BuildFromExo(std::shared_ptr<Operator> const& op){
                return DoubleKernel(Dispatch_Exo{}, op); 
        }
        std::shared_ptr<Operator> as_operator_()const{
                return impl_->as_operator_();
        }
private:
        std::shared_ptr<DoubleKernelImpl> impl_;
};

void black_scholes_template(){
        auto black_eval = BlackScholesCallOption::Build<double>{};

        double t   = 0.0;
        double T   = 10.0;
        double r   = 0.04;
        double S   = 50;
        double K   = 60;
        double vol = 0.2;

        std::cout << "black_eval(t,T,r,S,K,vol) => " << black_eval.Evaluate(t,T,r,S,K,vol) << "\n"; // __CandyPrint__(cxx-print-scalar,black_eval(t,T,r,S,K,vol))


        
        auto ad_kernel = BlackScholesCallOption::Build<DoubleKernel>{};

        auto as_black = ad_kernel.Evaluate( 
                Frontend::Double("t"),
                Frontend::Double("T"),
                Frontend::Double("r"),
                Frontend::Double("S"),
                Frontend::Double("K"),
                Frontend::Double("vol")
        );

        std::cerr << __FILE__ << ":" << __LINE__ << ":A\n"; // __CandyTag__A
        as_black.as_operator_()->Display();
        std::cerr << __FILE__ << ":" << __LINE__ << ":B\n"; // __CandyTag__B

        Function f("black");
        f.AddArgument("t");
        f.AddArgument("T");
        f.AddArgument("r");
        f.AddArgument("S");
        f.AddArgument("K");
        f.AddArgument("vol");

        using namespace Frontend;
        auto black = f.AddStatement(Stmt("black", as_black));
        black->Display();
        std::cout << "black->Name() => " << black->Name() << "\n"; // __CandyPrint__(cxx-print-scalar,black->Name())

        std::unordered_set< std::shared_ptr<Operator const> > seen;
        struct StackFrame{
                explicit StackFrame(std::shared_ptr<Operator const> op)
                        : Op{op}
                {
                        auto deps_set = Op->EndgenousDependencies();
                        Deps.assign(deps_set.begin(), deps_set.end());
                }
                std::shared_ptr<Operator const> Op;
                std::vector<std::shared_ptr<EndgenousSymbol const> > Deps;
        };
        std::vector<StackFrame> stack{StackFrame{as_black.as_operator_()}};
        for(size_t ttl=1000;stack.size() && ttl;--ttl){
                auto& frame = stack.back();
                if( frame.Deps.size() == 0 ){
                        if( seen.count(frame.Op) == 0 ){
                                seen.insert(frame.Op);
                                std::cout << "----------TERMINAL--------------\n";
                                frame.Op->Display();
                        }
                        stack.pop_back();
                        continue;
                }
                auto dep = frame.Deps.back();
                frame.Deps.pop_back();

                stack.push_back(StackFrame{dep});

        }


        std::ofstream fstr("prog.cxx");
        fstr << R"(
#include <cstdio>
#include <cmath>
)";

        StringCodeGenerator cg;
        cg.Emit(fstr, f);
        fstr << R"(

double black_fd(double epsilon, double t, double d_t, double T, double d_T, double r, double d_r, double S, double d_S, double K, double d_K, double vol, double d_vol){
        double dummy;
        double lower = black( t - d_t*epsilon/2 , &dummy, T - d_T*epsilon/2  , &dummy, r - d_r*epsilon/2  , &dummy, S - d_S*epsilon/2  , &dummy, K - d_K*epsilon/2  , &dummy, vol - d_vol*epsilon/2, &dummy);
        double upper = black( t + d_t*epsilon/2 , &dummy, T + d_T*epsilon/2  , &dummy, r + d_r*epsilon/2  , &dummy, S + d_S*epsilon/2  , &dummy, K + d_K*epsilon/2  , &dummy, vol + d_vol*epsilon/2, &dummy);
        double finite_diff = ( upper - lower ) / epsilon;
        return finite_diff;
}
int main(){
        double t   = 0.0;
        double T   = 10.0;
        double r   = 0.04;
        double S   = 50;
        double K   = 60;
        double vol = 0.2;

        double epsilon = 1e-10;

        double d_t = 0.0;
        double d_T = 0.0;
        double d_r = 0.0;
        double d_S = 0.0;
        double d_K = 0.0;
        double d_vol = 0.0;
        double value = black( t  , &d_t, T  , &d_T, r  , &d_r, S  , &d_S, K  , &d_K, vol, &d_vol);

        double d1 = 1/ ( vol * std::sqrt(T - t)) *  ( std::log(S/K) + ( r + vol*vol/2)*(T-t));

        double dummy;
        double lower = black( t - epsilon/2 , &dummy, T  , &dummy, r  , &dummy, S  , &dummy, K  , &dummy, vol, &dummy);
        double upper = black( t + epsilon/2 , &dummy, T  , &dummy, r  , &dummy, S  , &dummy, K  , &dummy, vol, &dummy);
        double finite_diff = ( upper - lower ) / epsilon;
        double residue = d_t - finite_diff;

        printf("%f,%f,%f,%f,%f,%f => %f,%f => %f,%f,%f\n", t, T, r, S, K, vol, value, d1, d_t, finite_diff, residue);

        printf("d[t]  ,%f,%f\n", d_t  ,  black_fd(epsilon, t, 1, T  , 0, r  , 0, S  , 0, K  , 0, vol, 0));
        printf("d[T]  ,%f,%f\n", d_T  ,  black_fd(epsilon, t, 0, T  , 1, r  , 0, S  , 0, K  , 0, vol, 0));
        printf("d[r]  ,%f,%f\n", d_r  ,  black_fd(epsilon, t, 0, T  , 0, r  , 1, S  , 0, K  , 0, vol, 0));
        printf("d[S]  ,%f,%f\n", d_S  ,  black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 1, K  , 0, vol, 0));
        printf("d[K]  ,%f,%f\n", d_K  ,  black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 0, K  , 1, vol, 0));
        printf("d[vol],%f,%f\n", d_vol,  black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 0, K  , 0, vol, 1));
        

}
)";
}



int main(){
        //black_scholes();
        //black_scholes_frontend();
        black_scholes_template();

}
