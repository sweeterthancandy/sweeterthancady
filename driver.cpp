#include "Cady/Cady.h"
#include "Cady/Frontend.h"
#include "Cady/CodeGen.h"

#include <map>
#include <iomanip>

using namespace Cady;
using namespace Cady::CodeGen;

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
                DoubleKernel::BuildFromExo("t"),
                DoubleKernel::BuildFromExo("T"),
                DoubleKernel::BuildFromExo("r"),
                DoubleKernel::BuildFromExo("S"),
                DoubleKernel::BuildFromExo("K"),
                DoubleKernel::BuildFromExo("vol")
        );


        Function f("black");
        f.AddArgument("t");
        f.AddArgument("T");
        f.AddArgument("r");
        f.AddArgument("S");
        f.AddArgument("K");
        f.AddArgument("vol");

        using namespace Frontend;

        std::unordered_set< std::shared_ptr<Operator > > seen;
        struct StackFrame{
                explicit StackFrame(std::shared_ptr<EndgenousSymbol > op)
                        : Op{op}
                {
                        auto deps_set = Op->EndgenousDependencies();
                        Deps.assign(deps_set.begin(), deps_set.end());
                }
                std::shared_ptr<EndgenousSymbol > Op;
                std::vector<std::shared_ptr<EndgenousSymbol > > Deps;
        };
        std::vector<StackFrame> stack{StackFrame{std::reinterpret_pointer_cast<EndgenousSymbol>(as_black.as_operator_())}};
        for(size_t ttl=1000;stack.size() && ttl;--ttl){
                auto& frame = stack.back();
                if( frame.Deps.size() == 0 ){
                        if( seen.count(frame.Op) == 0 ){
                                seen.insert(frame.Op);
                                auto black = f.AddStatement(frame.Op);
                                #if 0
                                std::cout << "----------TERMINAL--------------\n";
                                frame.Op->Display();
                                #endif
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
#include <iostream>
#include <boost/timer/timer.hpp>
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
        
        // time profile
        for(volatile size_t N = 100;;N*=2){
                boost::timer::cpu_timer timer;
                for(volatile size_t idx=0;idx!=N;++idx){
                        double value = black( t  , &d_t, T  , &d_T, r  , &d_r, S  , &d_S, K  , &d_K, vol, &d_vol);
                }
                std::string ad_time = timer.format(4, "%w");
                timer.start();
                for(volatile size_t idx=0;idx!=N;++idx){
                        black_fd(epsilon, t, 1, T  , 0, r  , 0, S  , 0, K  , 0, vol, 0);
                        black_fd(epsilon, t, 0, T  , 1, r  , 0, S  , 0, K  , 0, vol, 0);
                        black_fd(epsilon, t, 0, T  , 0, r  , 1, S  , 0, K  , 0, vol, 0);
                        black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 1, K  , 0, vol, 0);
                        black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 0, K  , 1, vol, 0);
                        black_fd(epsilon, t, 0, T  , 0, r  , 0, S  , 0, K  , 0, vol, 1);
                }
                std::string fd_time = timer.format(4, "%w");
                std::cout << N << "," << fd_time << "," << ad_time << "\n";
        }

}
)";
}


struct RemoveEndgenousFolder{
        std::shared_ptr<Operator> Fold(std::shared_ptr<Operator> root){
                if( root->Kind() == OPKind_EndgenousSymbol ){
                        auto as_endgenous = std::reinterpret_pointer_cast<EndgenousSymbol>(root);
                        return this->Fold(as_endgenous->Expr());
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

struct RemoveEndo : OperatorTransform{
        virtual std::shared_ptr<Operator> Apply(std::shared_ptr<Operator> const& ptr){
                auto candidate = ptr->Clone(shared_from_this());
                if( candidate->Kind() == OPKind_EndgenousSymbol ){
                        if( auto typed = std::dynamic_pointer_cast<EndgenousSymbol>(candidate)){
                                return typed->Expr();
                        }
                }
                return candidate;
        }
};
struct RemapUnique : OperatorTransform{
        explicit RemapUnique(std::string const& prefix = "__symbol_")
                : prefix_{prefix}
        {
                std::cerr << " RemapUnique()\n";
        }
        ~RemapUnique(){
                std::cerr << "~RemapUnique()\n";
        }
        void mutate_prefix(std::string const& prefix){
                prefix_ = prefix;
        }
        virtual std::shared_ptr<Operator> Apply(std::shared_ptr<Operator> const& ptr){

                auto candidate = ptr->Clone(shared_from_this());

                auto key = std::make_tuple(
                        candidate->NameInvariantOfChildren(),
                        candidate->Children()
                        );

                auto iter = ops_.find(key);
                if( iter != ops_.end() )
                        return iter->second;

                if( 
                    candidate->Kind() != OPKind_EndgenousSymbol &&
                    #if 0
                    candidate->Kind() != OPKind_ExogenousSymbol &&
                    #endif
                    candidate->Kind() != OPKind_Constant )
                {

                        std::stringstream ss;
                        ss << prefix_ << (ops_.size()+1);
                        auto endogous_sym = EndgenousSymbol::Make(ss.str(), candidate); 
                        
                        ops_[key] = endogous_sym;
                        return endogous_sym;
                } else {
                        ops_[key] = candidate;
                        return candidate;
                }
        }
private:
        std::string prefix_;
        std::map<
                std::tuple<
                        std::string,
                        std::vector<std::shared_ptr<Operator> > 
                >,
                std::shared_ptr<Operator>
        > ops_;
};




void black_scholes_template_opt(){

        
        auto ad_kernel = BlackScholesCallOption::Build<DoubleKernel>{};

        auto as_black = ad_kernel.Evaluate( 
                DoubleKernel::BuildFromExo("t"),
                DoubleKernel::BuildFromExo("T"),
                DoubleKernel::BuildFromExo("r"),
                DoubleKernel::BuildFromExo("S"),
                DoubleKernel::BuildFromExo("K"),
                DoubleKernel::BuildFromExo("vol")
        );

        SymbolTable ST;
        ST("t"  , 0.0);
        ST("T"  , 10.0);
        ST("r"  , 0.04);
        ST("S"  , 50);
        ST("K"  , 60);
        ST("vol", 0.2);

        RemoveEndgenousFolder remove_endogous;
        Transform::FoldZero constant_fold;

        auto black_expr = as_black.as_operator_();

        std::cout << "--------- black_expr -----------\n";
        //black_expr->Display();
        std::cout << "black_expr->Eval(ST) => " << black_expr->Eval(ST) << "\n"; // __CandyPrint__(cxx-print-scalar,black_expr->Eval(ST))

        auto removed_endo = remove_endogous.Fold(black_expr);

        auto params = std::vector<std::string>{ "t", "T", "r", "S", "K", "vol" };

        std::vector<std::shared_ptr<Operator> > ticker;
        for(auto const& s : params){
                auto raw_diff = removed_endo->Diff(s);
                ticker.push_back(raw_diff);
        }

        auto unique_mapper = std::make_shared<RemapUnique>();

        std::ofstream out("black_better.h");

        out << "double black(";
        for(size_t idx=0;idx!=params.size();++idx){
                if( idx != 0 ){
                        out << ", ";
                }
                out << "double " << params[idx] << ", double* d_" << params[idx];
        }
        out << "){\n";

        std::unordered_set<std::shared_ptr<Operator> > seen;
        std::shared_ptr<EndgenousSymbol> return_;
        for(size_t idx=0;idx!=ticker.size();++idx){
                auto constant_folded = constant_fold.Fold(ticker[idx]);
                
                auto unique          = constant_folded->Clone(unique_mapper);

                auto dependents = unique->DepthFirstAnySymbolicDependency();

                for(auto const& dep : dependents.DepthFirst){
                        // first emit all the expressions we need
                        if( seen.count(dep) > 0 )
                                continue;
                        seen.insert(dep);

                        out << "    double " << std::left << std::setw(15) << dep->Name() << " = ";
                        dep->Expr()->EmitCode(out);
                        out << ";\n";
                }

                out << "    *d_" << params[idx] << " = ";
                unique->EmitCode(out);
                out << ";\n";



                //unique->Display();
        }

        auto constant_folded = constant_fold.Fold(removed_endo);
        
        auto unique          = constant_folded->Clone(unique_mapper);

        auto dependents = unique->DepthFirstAnySymbolicDependency();

        for(auto const& dep : dependents.DepthFirst){
                // first emit all the expressions we need
                if( seen.count(dep) > 0 )
                        continue;
                seen.insert(dep);

                out << "    double " << std::left << std::setw(15) << dep->Name() << " = ";
                dep->Expr()->EmitCode(out);
                out << ";\n";
        }
        out << "    return ";
        unique->EmitCode(out);
        out << ";\n";
        out << "}\n";


}







struct DataFlow;

struct DotCompiler{
        DotCompiler(){
                order.emplace_back();
        }
        std::stringstream nodes;
        std::stringstream edges;
        std::vector<std::vector<std::string> > order;
};

struct DataFlowGraph{
        void Add(std::shared_ptr<DataFlow> const& flow);
        void EmitDot(std::ostream& out)const;
        void EmitCppCode(std::ostream& out)const;
        void CollectADFlow(std::vector<std::shared_ptr<EndgenousSymbol> > & computation);
        void Display(std::ostream& out = std::cout)const;
        void EmitInstructions(InstructionBlock& B)const;
        void EmitADInstructions(Operator::DependentsProfile& information,
                                std::shared_ptr<RemapUnique> RU,
                                InstructionBlock& B)const;
private:
        std::vector<std::shared_ptr<DataFlow> > rank_;
        std::unordered_map<std::string,std::shared_ptr<DataFlow> > index_; 
};

struct DataFlow{
        DataFlow(std::shared_ptr<EndgenousSymbol> sym)
                :sym_(sym)
        {}
        auto Expr()const{ return sym_; }
        auto Name()const{ return sym_->Name(); }
        void AddParent(DataFlow* ptr){
                parents_.push_back(ptr);
        }
        void AddChild(DataFlow* ptr){
                children_.push_back(ptr);
        }
        #if 0
        void EmitADInstructionsImpl(Operator::DependentsProfile& information, std::shared_ptr<RemapUnique> RU, InstructionBlock& B)const
        {
                auto remapped = sym_->Expr()->Clone(RU);

                auto dependents = remapped->DepthFirstAnySymbolicDependency();
                for(auto const& dep : dependents.DepthFirst ){
                        if( information.Set.count(dep) == 1 )
                                continue;
                        information.Set.insert(dep);
                        B.Add(std::make_shared<InstructionDeclareVariable>(
                                        dep->Name(),
                                        dep->Expr()));
                }

                auto exo = std::reinterpret_pointer_cast<EndgenousSymbol>(remapped);

                B.Add(std::make_shared<InstructionDeclareVariable>(
                                exo->Name(),
                                exo->Expr()));
                if( children_.empty() ){
                        std::vector<std::string> deps;
                        auto dependents = sym_->DepthFirstAnySymbolicDependencyAndThis();
                        for(auto const& ptr : dependents.DepthFirst ){
                                deps.push_back(ptr->Name());
                        }
                        B.Add(std::make_shared<InstructionComment>(deps));
                        B.Add(std::make_shared<InstructionReturn>(sym_->Name()));
                }
        }
        #endif
        void EmitInstructionsImpl(InstructionBlock& B)const{
                B.Add(std::make_shared<InstructionDeclareVariable>(
                                sym_->Name(),
                                sym_->Expr()));
                if( children_.empty() ){
                        std::vector<std::string> deps;
                        auto dependents = sym_->DepthFirstAnySymbolicDependencyAndThis();
                        for(auto const& ptr : dependents.DepthFirst ){
                                deps.push_back(ptr->Name());
                        }
                        B.Add(std::make_shared<InstructionComment>(deps));
                        B.Add(std::make_shared<InstructionReturn>(sym_->Name()));
                }
        }
        void EmitADInstructionsImpl(Operator::DependentsProfile& information, std::shared_ptr<RemapUnique> RU, InstructionBlock& B)const
        {
                auto make_ad_sym = [](auto const& name){
                        return "__rev_ad_" + name;
                };

                if( children_.size() == 0 ){
                        B.Add(std::make_shared<InstructionDeclareVariable>(
                                make_ad_sym(sym_->Name()),
                                Constant::Make(1.0)
                        ));
                } else {
                        static auto removed_end = std::make_shared<RemoveEndo>();
                        static Transform::FoldZero constant_fold;
                        // forward
                        auto make_node_back = [&](auto child){
                                return constant_fold.Fold(
                                        BinaryOperator::Mul(
                                                child->Expr()->Expr()->Diff(sym_->Name()),
                                                ExogenousSymbol::Make(make_ad_sym(child->Name()))
                                        )
                                );
                        };
                        std::shared_ptr<Operator> head = make_node_back(children_[0]);
                        for(size_t idx=1;idx<children_.size();++idx){
                                head = BinaryOperator::Add(
                                        head,
                                        make_node_back(children_[idx])
                                );
                        }
                
                        #if 0
                        auto remapped = head;
                        #else
                        std::stringstream cpp_expr;
                        head = head->Clone(removed_end);
                        head->EmitCode(cpp_expr);
                        //head = remove_endogous.Fold(head);
                        //auto folded = constant_fold.Fold(head);
                        auto remapped = head->Clone(RU);

                        B.Add(std::make_shared<InstructionComment>(std::vector<std::string>{"BEGIN " + cpp_expr.str()}));
                        auto dependents = remapped->DepthFirstAnySymbolicDependency();
                        for(auto const& dep : dependents.DepthFirst ){
                                if( information.Set.count(dep) == 1 )
                                        continue;
                                information.Set.insert(dep);
                                B.Add(std::make_shared<InstructionDeclareVariable>(
                                                dep->Name(),
                                                dep->Expr()));
                                
                        }
                        B.Add(std::make_shared<InstructionComment>(std::vector<std::string>{"END " + cpp_expr.str()}));
                        #endif

                                
                        B.Add(std::make_shared<InstructionDeclareVariable>(
                                make_ad_sym(sym_->Name()),
                                remapped));
                }

                if( parents_.size() == 0 ){
                        auto make_ptr_sym = [](auto const& name){
                                return "d_" + name;
                        };
                        auto param_name = std::dynamic_pointer_cast<ExogenousSymbol>(sym_->Expr());
                        B.Add(std::make_shared<InstructionPointerAssignment>(
                                        make_ptr_sym(param_name->Name()),
                                        make_ad_sym(sym_->Name())));
                }


#if 0

                auto make_ptr_sym = [](auto const& name){
                        return "d_" + name;
                };
                if( parents_.empty() ){
                        auto param_name = std::dynamic_pointer_cast<ExogenousSymbol>(sym_->Expr());
                        out << "*" << make_ptr_sym(param_name->Name()) << " = " << make_ad_sym(sym_->Name()) << ";\n";
                }
#endif
        }
        void EmitEvalCodeImpl(std::ostream& out)const{
                out << "double " << sym_->Name() << " = ";
                sym_->Expr()->EmitCode(out);
                out << ";\n";
                if( children_.size() == 0 ){
                        auto make_ad_sym = [](auto const& name){
                                return "__rev_ad_" + name;
                        };
                        auto make_ptr_sym = [](auto const& name){
                                return "d_" + name;
                        };
                        std::cout << sym_->Name() << "\n";
                        //sym_->Display();
                        if( auto param_name = std::dynamic_pointer_cast<ExogenousSymbol>(sym_->Expr()) ){
                                out << "*" << make_ptr_sym(param_name->Name()) << " = " << make_ad_sym(sym_->Name()) << ";\n";
                        } else {
                                out << "// unexpcted\n";
                        }
                }
                
                auto make_ad_sym = [](auto const& name){
                        return "__rev_ad_" + name;
                };
                auto make_ptr_sym = [](auto const& name){
                        return "d_" + name;
                };
                if( parents_.empty() ){
                        auto param_name = std::dynamic_pointer_cast<ExogenousSymbol>(sym_->Expr());
                        out << "*" << make_ptr_sym(param_name->Name()) << " = " << make_ad_sym(sym_->Name()) << ";\n";
                }
        }
        void EmitADFlowImpl(std::vector<std::shared_ptr<EndgenousSymbol> > & computation){
                auto make_ad_sym = [](auto const& name){
                        return "__rev_ad_" + name;
                };
                if( children_.size() > 0 ){
                        static Transform::FoldZero constant_fold;
                        // forward
                        auto make_node_back = [&](auto child){
                                return constant_fold.Fold(
                                        BinaryOperator::Mul(
                                                child->Expr()->Expr()->Diff(sym_->Name()),
                                                ExogenousSymbol::Make(make_ad_sym(child->Name()))
                                        )
                                );
                        };
                        std::shared_ptr<Operator> head = make_node_back(children_[0]);
                        for(size_t idx=1;idx<children_.size();++idx){
                                head = BinaryOperator::Add(
                                        head,
                                        make_node_back(children_[idx])
                                );
                        }


                        computation.push_back(EndgenousSymbol::Make( make_ad_sym(sym_->Name()), head));
                                
                }
        }
        void EmitReverseADCodeImpl(std::ostream& out)const{
                auto make_ad_sym = [](auto const& name){
                        return "__rev_ad_" + name;
                };
                out << "double " << make_ad_sym(sym_->Name()) << " = ";
                if( children_.size() > 0 ){
                        static Transform::FoldZero constant_fold;
                        // forward
                        auto make_node_back = [&](auto child){
                                return constant_fold.Fold(
                                        BinaryOperator::Mul(
                                                child->Expr()->Expr()->Diff(sym_->Name()),
                                                ExogenousSymbol::Make(make_ad_sym(child->Name()))
                                        )
                                );
                        };
                        std::shared_ptr<Operator> head = make_node_back(children_[0]);
                        for(size_t idx=1;idx<children_.size();++idx){
                                head = BinaryOperator::Add(
                                        head,
                                        make_node_back(children_[idx])
                                );
                        }
                                
                        head->EmitCode(out);
                } else {
                        out << "1.0";
                }
                out << ";\n";

                auto make_ptr_sym = [](auto const& name){
                        return "d_" + name;
                };
                if( parents_.empty() ){
                        auto param_name = std::dynamic_pointer_cast<ExogenousSymbol>(sym_->Expr());
                        out << "*" << make_ptr_sym(param_name->Name()) << " = " << make_ad_sym(sym_->Name()) << ";\n";
                }

        }

        void EmitDot(DotCompiler& compiler)const{
                if( parents_.size() ){
                        compiler.order.emplace_back(std::vector<std::string>{sym_->Name()});
                } else {
                        compiler.order[0].push_back(sym_->Name());
                }
                compiler.nodes << sym_->Name() << "[shape=record, label=\"<expr>";
                compiler.nodes << sym_->Name() << " = ";
                sym_->Expr()->EmitCode(compiler.nodes);

                #if 0
                for(auto const& ptr : parents_){
                        auto name = ptr->Name();
                        auto diff = sym_->Expr()->Diff(name);
                        Transform::FoldZero constant_fold;
                        auto folded = constant_fold.Fold(diff);
                        compiler.nodes << "|D[" << name << "] = ";
                        folded->EmitCode(compiler.nodes);
                }
                #endif
                Transform::FoldZero constant_fold;
                compiler.nodes << "|<diff>D[" << sym_->Name() << "]";
                if( parents_.size() > 0 ){

                        // forward
                        auto make_node_fwd = [&](auto name){
                                return constant_fold.Fold(
                                        BinaryOperator::Mul(
                                                sym_->Expr()->Diff(name),
                                                ExogenousSymbol::Make("D[" + name + "]")
                                        )
                                );
                        };
                        std::shared_ptr<Operator> head = make_node_fwd(parents_[0]->Name());
                        for(size_t idx=1;idx<parents_.size();++idx){
                                head = BinaryOperator::Add(
                                        head,
                                        make_node_fwd(parents_[idx]->Name())
                                );
                        }
                                
                        compiler.nodes << " = ";
                        head->EmitCode(compiler.nodes);


                }
                
                compiler.nodes << "|<bdiff>B[" << sym_->Name() << "]";
                if( children_.size() > 0 ){
                        // forward
                        auto make_node_back = [&](auto child){
                                return constant_fold.Fold(
                                        BinaryOperator::Mul(
                                                child->Expr()->Expr()->Diff(sym_->Name()),
                                                ExogenousSymbol::Make("B[" + child->Name() + "]")
                                        )
                                );
                        };
                        std::shared_ptr<Operator> head = make_node_back(children_[0]);
                        for(size_t idx=1;idx<children_.size();++idx){
                                head = BinaryOperator::Add(
                                        head,
                                        make_node_back(children_[idx])
                                );
                        }
                                
                        compiler.nodes << " = ";
                        head->EmitCode(compiler.nodes);
                }
                compiler.nodes << "\"];\n";
                #if 1
                for(auto const& ptr : parents_){

                        auto name = ptr->Name();

                        auto diff = sym_->Expr()->Diff(name);
                        auto folded = constant_fold.Fold(diff);

                        compiler.edges << name << ":diff -> " << sym_->Name() << ":diff [color=blue,label=\"";
                        folded->EmitCode(compiler.edges);
                        compiler.edges << "\"];\n";
                }
                #endif
                for(auto const& ptr : children_ ){

                        auto diff = ptr->Expr()->Expr()->Diff(sym_->Name());
                        auto folded = constant_fold.Fold(diff);

                        compiler.edges << ptr->Name() << ":bdiff -> " << sym_->Name() << ":bdiff [color=red,label=\"";
                        folded->EmitCode(compiler.edges);
                        compiler.edges << "\"];\n";
                }

        }
private:
        std::shared_ptr<EndgenousSymbol> sym_;
        std::vector<DataFlow*> parents_;
        std::vector<DataFlow*> children_;
};
void DataFlowGraph::CollectADFlow(std::vector<std::shared_ptr<EndgenousSymbol> > & computation){
        for(auto const& flow : rank_){
                flow->EmitADFlowImpl(computation);
        }
}
        
void DataFlowGraph::EmitDot(std::ostream& out)const{
        DotCompiler dc;
        for(auto const& flow : rank_){
                flow->EmitDot(dc);
        }
        out << "digraph{\n";
        out << dc.nodes.str();
        out << dc.edges.str();

        out << "node [shape = none];\n";
        for(size_t idx=0;idx!=dc.order.size();++idx){
                if( idx != 0 )
                        out << "->";
                out << idx;
        }
        out << "[arrowhead=none,shape=none]\n";
        for(size_t idx=0;idx!=dc.order.size();++idx){
                out << "{rank=same;" << idx;
                for(auto const& name : dc.order[idx] ){
                        out << "," << name;
                }
                out << "}\n";
        }
        out << "}\n";
}
void DataFlowGraph::Display(std::ostream& out)const{
        for(auto const& flow : rank_){
                std::cout << "--: " << flow->Name() << "\n";
        }
}
void DataFlowGraph::EmitInstructions(InstructionBlock& B)const{
        for(auto const& flow : rank_){
                flow->EmitInstructionsImpl(B);
        }
}
void DataFlowGraph::EmitADInstructions(Operator::DependentsProfile& information,
                                       std::shared_ptr<RemapUnique> RU,
                                       InstructionBlock& B)const
{
        for(size_t idx=rank_.size();idx;){
                --idx;
                rank_[idx]->EmitADInstructionsImpl(information, RU, B);
        }
}
void DataFlowGraph::EmitCppCode(std::ostream& out)const{
        out << R"(

#include <cstdio>
#include <cmath>
#include <iostream>

double black(double t, double* d_t, double T, double* d_T, double r, double* d_r, double S, double* d_S, double K, double* d_K, double vol, double* d_vol){
)";
        for(auto const& flow : rank_){
                flow->EmitEvalCodeImpl(out);
        }
#if 0
for(size_t idx=rank_.size();idx;){
        --idx;
        rank_[idx]->EmitReverseADCodeImpl(out);
}
#endif
        out << "return " << rank_.back()->Name() << ";\n";
        out << "}";
        out <<
R"(

int main(){
        double t   = 0.0;
        double T   = 10.0;
        double r   = 0.04;
        double S   = 50;
        double K   = 60;
        double vol = 0.2;

        double d_t = 0.0;
        double d_T = 0.0;
        double d_r = 0.0;
        double d_S = 0.0;
        double d_K = 0.0;
        double d_vol = 0.0;
        double value = black( t  , &d_t, T  , &d_T, r  , &d_r, S  , &d_S, K  , &d_K, vol, &d_vol);

        std::cout << "black = " << value << "\n";
        std::cout << "d_t   = " << d_t << "\n";
        std::cout << "d_T   = " << d_T << "\n";
        std::cout << "d_r   = " << d_r << "\n";
        std::cout << "d_S   = " << d_S << "\n";
        std::cout << "d_K   = " << d_K << "\n";
        std::cout << "d_vol = " << d_vol << "\n";
}
)";
}

void DataFlowGraph::Add(std::shared_ptr<DataFlow> const& flow){
        rank_.push_back(flow);
        index_[flow->Name()] = flow;
        auto dependents = flow->Expr()->DepthFirstAnySymbolicDependencyNoRecurse();

        auto link_dependency = [&](auto& child, auto& parent){
                child->AddParent(parent.get());
                parent->AddChild(child.get());
        };

        for(auto const& ptr : dependents.DepthFirst ){
                auto iter = index_.find(ptr->Name());
                if( iter == index_.end() )
                        throw std::domain_error("cant find node");
                link_dependency(flow, iter->second);
        }
}

void reverse_test(){
        using namespace Frontend;
        using Frontend::Sin;
        #if 0
        auto x1 = Var("x1");
        auto x2 = Var("x2");
        auto expr_ = Break("z", Break("y", x1 * x2 + Sin(x1)));
        auto expr = expr_.as_operator_();
        #else
        auto ad_kernel = BlackScholesCallOption::Build<DoubleKernel>{};

        auto as_black = ad_kernel.Evaluate( 
                DoubleKernel::BuildFromExo("t"),
                DoubleKernel::BuildFromExo("T"),
                DoubleKernel::BuildFromExo("r"),
                DoubleKernel::BuildFromExo("S"),
                DoubleKernel::BuildFromExo("K"),
                DoubleKernel::BuildFromExo("vol")
        );
        auto expr_ = as_black.as_operator_();
        RemoveEndgenousFolder remove_endogous;
        auto expr = remove_endogous.Fold(expr_);
        #endif

        auto unique_mapper = std::make_shared<RemapUnique>("w");
        auto unique        = Break("value", expr->Clone(unique_mapper)).as_operator_();
        //unique->Display();
                
        auto dependents = unique->DepthFirstAnySymbolicDependencyAndThis();
        std::vector<std::shared_ptr<DataFlow> > flow;
        DataFlowGraph graph;
        for(auto const& dep : dependents.DepthFirst){
                auto ptr = std::make_shared<DataFlow>(dep);
                flow.push_back(ptr);
                graph.Add(ptr);
        }
        #if 0
        for(auto const& step : flow){
                step->Expr()->Display();
        }
        #endif

        std::ofstream out("graph.dot");
        graph.EmitDot(out);
        out.close();
        std::system("dot -Tpng graph.dot -o graph.png");


        InstructionBlock B;
        graph.EmitInstructions(B);

        std::cout << "dependents.Set.size() => " << dependents.Set.size() << "\n"; // __CandyPrint__(cxx-print-scalar,dependents.Set.size())
        graph.EmitADInstructions(dependents, unique_mapper, B);



        std::ofstream code("black.cpp");
        code << R"(

#include <cstdio>
#include <cmath>
#include <iostream>

double black(double t, double* d_t, double T, double* d_T, double r, double* d_r, double S, double* d_S, double K, double* d_K, double vol, double* d_vol){
)";

        B.EmitCode(code);
        code <<
R"(
}

int main(){
        double t   = 0.0;
        double T   = 10.0;
        double r   = 0.04;
        double S   = 50;
        double K   = 60;
        double vol = 0.2;

        double d_t = 0.0;
        double d_T = 0.0;
        double d_r = 0.0;
        double d_S = 0.0;
        double d_K = 0.0;
        double d_vol = 0.0;
        double value = black( t  , &d_t, T  , &d_T, r  , &d_r, S  , &d_S, K  , &d_K, vol, &d_vol);

        std::cout << "black = " << value << "\n";
        std::cout << "d_t   = " << d_t << "\n";
        std::cout << "d_T   = " << d_T << "\n";
        std::cout << "d_r   = " << d_r << "\n";
        std::cout << "d_S   = " << d_S << "\n";
        std::cout << "d_K   = " << d_K << "\n";
        std::cout << "d_vol = " << d_vol << "\n";
}
)";


        #if 0
        std::vector<std::shared_ptr<EndgenousSymbol> > ad_computation;
        graph.Display();
        graph.CollectADFlow(ad_computation);
        std::vector<std::shared_ptr<Operator> > computation;
        computation.push_back(unique);

        unique_mapper->mutate_prefix("__rev_ad_");

        for(auto& ptr : ad_computation ){
                computation.push_back(ptr->Clone(unique_mapper));
                //computation.push_back(ptr);
                //ptr->Display();
        }

        std::cout << "ad_computation.size() => " << ad_computation.size() << "\n"; // __CandyPrint__(cxx-print-scalar,computation.size())
        std::cout << "computation.size() => " << computation.size() << "\n"; // __CandyPrint__(cxx-print-scalar,computation.size())


        do{

                DataFlowGraph ad_graph;
                Operator::DependentsProfile dependents;
                for(auto const& ptr : computation ){
                        ptr->CollectDepthFirstAnySymbolicDependency(dependents, true);
                }
                for(auto const& dep : dependents.DepthFirst){
                        auto ptr = std::make_shared<DataFlow>(dep);
                        flow.push_back(ptr);
                        ad_graph.Add(ptr);
                }
                ad_graph.Display();
                std::cerr << __FILE__ << ":" << __LINE__ << ":A\n"; // __CandyTag__A
                std::ofstream code("black.cpp");
                ad_graph.EmitCppCode(code);
                code.close();
        }while(0);
        #endif

}

int main(){
        //black_scholes();
        //black_scholes_frontend();
        //black_scholes_template();
        //black_scholes_template_opt();

        try{
                reverse_test();
        } catch ( std::exception const& e ){
                std::cerr << "Exception: " << e.what() << "\n";
        }
}
