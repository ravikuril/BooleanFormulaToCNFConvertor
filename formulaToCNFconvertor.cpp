#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <string>
//#include "simplifier.h"               // this is boolean fromula to cnf convertor
using namespace std;


#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <vector>
#include <typeinfo>
namespace qi = boost::spirit::qi;

// Abstract data type

struct op_or {};
struct op_and {};
struct op_imp {};
struct op_iff {};
struct op_not {};
string output;
namespace ast
{
    typedef std::string var;
    template <typename tag> struct binop;
    template <typename tag> struct unop;

    enum class expr_type { var = 0, not_, and_, or_, imp, iff };
    typedef boost::variant<var,
        boost::recursive_wrapper<unop <op_not> >,
        boost::recursive_wrapper<binop<op_and> >,
        boost::recursive_wrapper<binop<op_or> >,
        boost::recursive_wrapper<binop<op_imp> >,
        boost::recursive_wrapper<binop<op_iff> >
    > expr;

    expr_type get_expr_type(const expr& expression)
    {
        return static_cast<expr_type>(expression.which());
    }

    template <typename tag> struct binop
    {
        expr oper1, oper2;
    };

    template <typename tag> struct unop
    {
        expr oper1;
    };

    struct printer : boost::static_visitor<void>
    {
        printer(std::ostream& os) : _os(os) {}
        std::ostream& _os;
        mutable bool first{ true };

        //
        void operator()(const ast::var& v) const { _os << v; output+=v; }


        void operator()(const ast::binop<op_and>& b) const {print(" and ", b.oper1, b.oper2); }
        void operator()(const ast::binop<op_or>& b) const { print(" or ", b.oper1, b.oper2); }
        void operator()(const ast::binop<op_iff>& b) const {print(" iff ", b.oper1, b.oper2); }
        void operator()(const ast::binop<op_imp>& b) const { print(" imp ", b.oper1, b.oper2); }

        void print(const std::string& op, const ast::expr& l, const ast::expr& r) const
        {
            _os << "(";
            output+="(";
            boost::apply_visitor(*this, l);
            _os << op;
            output+=op;
            boost::apply_visitor(*this, r);
            _os << ")";
            output+=")";
        }

        void operator()(const ast::unop<op_not>& u) const
        {
            _os << "not(";
            output+="not(";
            boost::apply_visitor(*this, u.oper1);
            _os << ")";
            output+=")";
        }
    };

    std::ostream& operator<<(std::ostream& os, const expr& e)
    {
        boost::apply_visitor(printer(os), e); return os;
    }

    expr operator!(const expr& e)
    {
        return unop<op_not>{e};
    }

    expr operator||(const expr& l, const expr& r)
    {
        return binop<op_or>{l, r};
    }

    expr operator&&(const expr& l, const expr& r)
    {
        return binop<op_and>{l, r};
    }

}

BOOST_FUSION_ADAPT_TPL_STRUCT(
    (Tag),
    (ast::binop) (Tag),
    (ast::expr, oper1)
    (ast::expr, oper2)
)

BOOST_FUSION_ADAPT_TPL_STRUCT(
    (Tag),
    (ast::unop) (Tag),
    (ast::expr, oper1)
)


// Grammar rules

template <typename It, typename Skipper = qi::space_type>
struct parser : qi::grammar<It, ast::expr(), Skipper>
{
    parser() : parser::base_type(expr_)
    {
        using namespace qi;
        const as<ast::binop<op_iff> > as_iff = {};
        const as<ast::binop<op_imp> > as_imp = {};
        const as<ast::binop<op_or> > as_or = {};
        const as<ast::binop<op_and> > as_and = {};
        const as<ast::unop<op_not> > as_not = {};

        expr_ = iff_.alias();

        iff_ = as_iff[imp_ >> "iff" >> iff_] | imp_;
        imp_ = as_imp[or_ >> "imp" >> imp_] | or_;
        or_ = as_or[and_ >> "or" >> or_] | and_;
        and_ = as_and[not_ >> "and" >> and_] | not_;
        not_ = as_not["not" > simple] | simple;

        simple = (('(' > expr_ > ')') | var_);
        var_ = qi::lexeme[+alpha];

        BOOST_SPIRIT_DEBUG_NODE(expr_);
        BOOST_SPIRIT_DEBUG_NODE(iff_);
        BOOST_SPIRIT_DEBUG_NODE(imp_);
        BOOST_SPIRIT_DEBUG_NODE(or_);
        BOOST_SPIRIT_DEBUG_NODE(and_);
        BOOST_SPIRIT_DEBUG_NODE(not_);
        BOOST_SPIRIT_DEBUG_NODE(simple);
        BOOST_SPIRIT_DEBUG_NODE(var_);
    }

private:
    qi::rule<It, ast::var(), Skipper> var_;
    qi::rule<It, ast::expr(), Skipper> not_, and_, or_, imp_, iff_, simple, expr_;
};

template <typename Transform>
struct ast_helper : boost::static_visitor<ast::expr>
{

    template <typename Tag>
    ast::expr pass_through(const ast::binop<Tag>& op) const
    {
        return ast::binop<Tag>{recurse(op.oper1), recurse(op.oper2)};
    }

    template <typename Tag>
    ast::expr pass_through(const ast::unop<Tag>& op) const
    {
        return ast::unop<Tag>{recurse(op.oper1)};
    }

    ast::expr pass_through(const ast::var& variable) const
    {
        return variable;
    }

    ast::expr recurse(const ast::expr& expression) const
    {
        return boost::apply_visitor(Transform{}, expression);
    }

    struct left_getter:boost::static_visitor<ast::expr>
    {
        template< template<class> class Op,typename Tag>
        ast::expr operator()(const Op<Tag>& op) const
        {
            return op.oper1;
        }

        ast::expr operator()(const ast::var&) const
        {
            return{};//throw something?
        }
    };


    ast::expr left(const ast::expr& expression) const
    {
        return boost::apply_visitor(left_getter{}, expression);
    }

    ast::expr child0(const ast::expr& expression) const
    {
        return left(expression);
    }

    struct right_getter :boost::static_visitor<ast::expr>
    {
        template<typename Tag>
        ast::expr operator()(const ast::binop<Tag>& op) const
        {
            return op.oper2;
        }

        template<typename Expr>
        ast::expr operator()(const Expr&) const
        {
            return{};//throw something?
        }
    };

    ast::expr right(const ast::expr& expression) const
    {
        return boost::apply_visitor(right_getter{}, expression);
    }

};

struct eliminate_imp : ast_helper<eliminate_imp>
{
    template <typename Op>
    ast::expr operator()(const Op& op) const
    {
        return pass_through(op);
    }

    ast::expr operator()(const ast::binop<op_imp>& imp) const
    {
        return !recurse(imp.oper1) || recurse(imp.oper2);
    }

    ast::expr operator()(const ast::expr& expression) const
    {
        return recurse(expression);
    }
};

struct eliminate_iff : ast_helper<eliminate_iff>
{
    template <typename Op>
    ast::expr operator()(const Op& op) const
    {
        return pass_through(op);
    }

    ast::expr operator()(const ast::binop<op_iff>& imp) const
    {
        return (recurse(imp.oper1) || !recurse(imp.oper2)) && (!recurse(imp.oper1) || recurse(imp.oper2));
    }

    ast::expr operator()(const ast::expr& expression) const
    {
        return recurse(expression);
    }
};

struct distribute_nots : ast_helper<distribute_nots>
{
    template <typename Op>
    ast::expr operator()(const Op& op) const
    {
        return pass_through(op);
    }

    ast::expr operator()(const ast::unop<op_not>& not_) const
    {
        switch (ast::get_expr_type(not_.oper1)) //There is probably a better solution
        {
        case ast::expr_type::and_:
            return recurse(!recurse(left(not_.oper1))) || recurse(!recurse(right(not_.oper1)));

        case ast::expr_type::or_:
            return recurse(!recurse(left(not_.oper1))) && recurse(!recurse(right(not_.oper1)));

        case ast::expr_type::not_:
            return recurse(child0(not_.oper1));
        default:
            return pass_through(not_);
        }
    }

    ast::expr operator()(const ast::expr& expression) const
    {
        return recurse(expression);
    }
};

struct any_and_inside : boost::static_visitor<bool>
{
    any_and_inside(const ast::expr& expression) :expression(expression) {}
    const ast::expr& expression;

    bool operator()(const ast::var&) const
    {
        return false;
    }

    template <typename Tag>
    bool operator()(const ast::binop<Tag>& op) const
    {
        return boost::apply_visitor(*this, op.oper1) || boost::apply_visitor(*this, op.oper2);
    }

    bool operator()(const ast::binop<op_and>&) const
    {
        return true;
    }

    template<typename Tag>
    bool operator()(const ast::unop<Tag>& op) const
    {
        return boost::apply_visitor(*this, op.oper1);
    }


    explicit operator bool() const
    {
        return boost::apply_visitor(*this, expression);
    }

};

struct distribute_ors : ast_helper<distribute_ors>
{
    template <typename Op>
    ast::expr operator()(const Op& op) const
    {
        return pass_through(op);
    }

    ast::expr operator()(const ast::binop<op_or>& or_) const
    {
        if (ast::get_expr_type(or_.oper1) == ast::expr_type::and_)
        {
            return recurse(recurse(left(or_.oper1)) || recurse(or_.oper2)) 
                && recurse(recurse(right(or_.oper1)) || recurse(or_.oper2));
        }
        else if (ast::get_expr_type(or_.oper2) == ast::expr_type::and_)
        {
            return recurse(recurse(or_.oper1) || recurse(left(or_.oper2)))
                && recurse(recurse(or_.oper1) || recurse(right(or_.oper2)));
        }
        else if (any_and_inside( or_ ))
        {
            return recurse(recurse(or_.oper1) || recurse(or_.oper2));
        }
        else
        {
            return pass_through(or_);
        }
    }

    ast::expr operator()(const ast::expr& expression) const
    {
        return recurse(expression);
    }

};

ast::expr to_CNF(const ast::expr& expression)
{
    return distribute_ors()(distribute_nots()(eliminate_iff()(eliminate_imp()(expression))));
}




// Test some examples in main and check the order of precedence

int CNF_convertor(std::string s)
{
    //std::string s="p imp q imp r;";
    std::list<std::string> pre;
    pre.push_back(s);
    //auto& input=pre.begin();
    for (auto& input :pre)
    

    {
        auto f(std::begin(input)), l(std::end(input));
        parser<decltype(f)> p;

        try
        {
            ast::expr result;
            bool ok = qi::phrase_parse(f, l, p > ';', qi::space, result);

            if (!ok)
               std::cerr << "invalid input\n";
            else
            {
               // std::cout << "originall: " << result << "\n";
        ast::expr s1=to_CNF(result);
                std::cout << "CNF: " <<s1<< "\n";
            }

        }
        catch (const qi::expectation_failure<decltype(f)>& e)
        {
            std::cerr << "expectation_failure at '" << std::string(e.first, e.last) << "'\n";
        }

        if (f != l) std::cerr << "unparsed: '" << std::string(f, l) << "'\n";
    }

    return 0;
}
