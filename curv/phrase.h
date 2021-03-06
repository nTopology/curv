// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_PHRASE_H
#define CURV_PHRASE_H

#include <vector>
#include <memory>
#include <curv/shared.h>
#include <curv/location.h>
#include <curv/atom.h>
#include <curv/tail_array.h>

namespace curv {

struct Definition;
struct Meaning;
struct Operation;
struct Segment;
struct String_Expr_Base;
using String_Expr = Tail_Array<String_Expr_Base>;
struct Environ;

/// Base class for a node in a syntax tree created by the parser.
///
/// Goals for the syntax tree data structure:
/// * Cheap to construct (small footprint, no semantic analysis that could be
///   delayed until JIT compile time).
/// * Can reconstruct its Location in the source code, for error reporting.
///   This doesn't need to be fast.
/// * Preserves the original source code in full, which means preserving all of
///   the original tokens and white space. So a syntax tree can be used for any
///   purpose, including upgrading source code from an earlier version of the
///   language to a newer version.
struct Phrase : public Shared_Base
{
    virtual ~Phrase() {}
    virtual Location location() const = 0;
    virtual Shared<Definition> as_definition(Environ&);
    virtual Shared<Meaning> analyse(Environ&) const = 0;
};

/// Abstract implementation base class for Phrase classes
/// that encapsulate a single token.
struct Token_Phrase : public Phrase
{
    Location loc_;
    Token_Phrase(const Script& s, Token tok) : loc_(s, std::move(tok)) {}
    virtual Location location() const override { return loc_; }
};
struct Identifier final : public Token_Phrase
{
    Atom atom_;

    Identifier(const Script& s, Token tok)
    :
        Token_Phrase(s, std::move(tok)),
        atom_{loc_.range()}
    {}

    virtual Shared<Meaning> analyse(Environ&) const override;
};
struct Numeral final : public Token_Phrase
{
    using Token_Phrase::Token_Phrase;
    virtual Shared<Meaning> analyse(Environ&) const override;
};

struct Segment_Phrase : public Shared_Base
{
    virtual Location location() const = 0;
    virtual Shared<Segment> analyse(Environ&) const = 0;
};
struct String_Segment_Phrase : public Segment_Phrase
{
    Location loc_;
    String_Segment_Phrase(const Script& s, Token tok)
    : loc_(s, std::move(tok)) {}
    virtual Location location() const override { return loc_; }
    virtual Shared<Segment> analyse(Environ&) const override;
};
struct Char_Escape_Phrase : public Segment_Phrase
{
    Location loc_;
    Char_Escape_Phrase(const Script& s, Token tok)
    : loc_(s, std::move(tok)) {}
    virtual Location location() const override { return loc_; }
    virtual Shared<Segment> analyse(Environ&) const override;
};
struct Paren_Segment_Phrase : public Segment_Phrase
{
    Shared<const Phrase> expr_;
    Paren_Segment_Phrase(Shared<const Phrase> expr)
    : expr_(std::move(expr)) {}
    virtual Location location() const override { return expr_->location(); }
    virtual Shared<Segment> analyse(Environ&) const override;
};
struct Bracket_Segment_Phrase : public Segment_Phrase
{
    Shared<const Phrase> expr_;
    Bracket_Segment_Phrase(Shared<const Phrase> expr)
    : expr_(std::move(expr)) {}
    virtual Location location() const override { return expr_->location(); }
    virtual Shared<Segment> analyse(Environ&) const override;
};
struct Brace_Segment_Phrase : public Segment_Phrase
{
    Shared<const Phrase> expr_;
    Brace_Segment_Phrase(Shared<const Phrase> expr)
    : expr_(std::move(expr)) {}
    virtual Location location() const override { return expr_->location(); }
    virtual Shared<Segment> analyse(Environ&) const override;
};
struct String_Phrase_Base : public Phrase
{
    Location begin_;
    Token end_;

    String_Phrase_Base(const Script& s, Token begin, Token end)
    :
        begin_{s, std::move(begin)},
        end_(std::move(end))
    {}

    virtual Location location() const override
    {
        return begin_.ending_at(end_);
    }
    virtual Shared<Meaning> analyse(Environ&) const override;
    Shared<String_Expr> analyse_string(Environ&) const;

    TAIL_ARRAY_MEMBERS(Shared<const Segment_Phrase>)
};
using String_Phrase = Tail_Array<String_Phrase_Base>;

struct Unary_Phrase : public Phrase
{
    Unary_Phrase(Token op, Shared<Phrase> arg)
    : op_(op), arg_(std::move(arg))
    {}
    Token op_;
    Shared<Phrase> arg_;
    virtual Location location() const override
    {
        return arg_->location().starting_at(op_);
    }
    virtual Shared<Definition> as_definition(Environ&) override;
    virtual Shared<Meaning> analyse(Environ&) const override;
};

struct Binary_Phrase : public Phrase
{
    Binary_Phrase(
        Shared<Phrase> left,
        Token op,
        Shared<Phrase> right)
    :
        left_(std::move(left)),
        op_(op),
        right_(std::move(right))
    {}
    Shared<Phrase> left_;
    Token op_;
    Shared<Phrase> right_;
    virtual Location location() const override
    {
        return left_->location().ending_at(right_->location().token());
    }
    virtual Shared<Meaning> analyse(Environ&) const override;
};

struct Lambda_Phrase : public Binary_Phrase
{
    /// Normally false, this is set to true prior to analysis if the lambda
    /// occurs in the definiens position of a definition. All of the function
    /// definitions in the same block or module share a single nonlocals list.
    bool shared_nonlocals_ = false;

    using Binary_Phrase::Binary_Phrase;

    virtual Shared<Meaning> analyse(Environ&) const override;
};

struct Recursive_Definition_Phrase : public Phrase
{
    Recursive_Definition_Phrase(
        Shared<Phrase> left,
        Token op,
        Shared<Phrase> right)
    :
        left_(left), op_(op), right_(right)
    {}
    Shared<Phrase> left_;
    Token op_;
    Shared<Phrase> right_;
    virtual Location location() const override
    {
        return left_->location().ending_at(right_->location().token());
    }
    virtual Shared<Definition> as_definition(Environ&) override;
    virtual Shared<Meaning> analyse(Environ&) const override;
};
struct Sequential_Definition_Phrase : public Phrase
{
    Sequential_Definition_Phrase(
        Token var,
        Shared<Phrase> left,
        Token op,
        Shared<Phrase> right)
    :
        var_(var), left_(left), op_(op), right_(right)
    {}
    Token var_;
    Shared<Phrase> left_;
    Token op_;
    Shared<Phrase> right_;
    virtual Location location() const override
    {
        return right_->location().starting_at(var_);
    }
    virtual Shared<Definition> as_definition(Environ&) override;
    virtual Shared<Meaning> analyse(Environ&) const override;
};
struct Assignment_Phrase : public Phrase
{
    Assignment_Phrase(
        Shared<Phrase> left,
        Token op,
        Shared<Phrase> right)
    :
        left_(left), op_(op), right_(right)
    {}
    Shared<Phrase> left_;
    Token op_;
    Shared<Phrase> right_;
    virtual Location location() const override
    {
        return left_->location().ending_at(right_->location().token());
    }
    virtual Shared<Meaning> analyse(Environ&) const override;
};

struct Empty_Phrase : public Phrase
{
    Location begin_;    // zero length location at start of phrase

    Empty_Phrase(Location begin) : begin_(std::move(begin)) {}

    virtual Location location() const override
    {
        return begin_;
    }
    virtual Shared<Meaning> analyse(Environ&) const override;
};

// common implementation for Comma_Phrase and Semicolon_Phrase
struct Separator_Phrase : public Phrase
{
    // an expression followed by an optional separator
    struct Arg
    {
        Shared<Phrase> expr_;
        Token separator_;

        Arg(Shared<Phrase> expr, Token separator)
        : expr_(std::move(expr)), separator_(separator)
        {}
    };

    std::vector<Arg> args_;

    Separator_Phrase() {}

    virtual Location location() const override
    {
        const Arg& last = args_.back();
        return args_.front().expr_->location().ending_at(
            last.separator_.kind_ == Token::k_missing
            ? last.expr_->location().token()
            : last.separator_);
    }
};

/// `a,b,c` -- One or more blocks, separated by commas, with optional trailing
/// comma. Guaranteed to contain at least one comma token.
struct Comma_Phrase : public Separator_Phrase
{
    using Separator_Phrase::Separator_Phrase;
    virtual Shared<Meaning> analyse(Environ&) const override;
};

/// `a;b;c` -- One or more items, separated by semicolons, with optional
/// trailing semicolon. Guaranteed to contain at least one semicolon token.
struct Semicolon_Phrase : public Separator_Phrase
{
    using Separator_Phrase::Separator_Phrase;
    virtual Shared<Meaning> analyse(Environ&) const override;
    virtual Shared<Definition> as_definition(Environ&) override;
};

/// common implementation for `(...)`, `[...]` and `{...}` phrases.
struct Delimited_Phrase : public Phrase
{
    Token lparen_;
    Shared<Phrase> body_;
    Token rparen_;

    Delimited_Phrase(Token lparen, Shared<Phrase> body, Token rparen)
    : lparen_(lparen), body_(std::move(body)), rparen_(rparen)
    {}

    virtual Location location() const override
    {
        return body_->location().starting_at(lparen_).ending_at(rparen_);
    }
};

struct Paren_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyse(Environ&) const override;
    virtual Shared<Definition> as_definition(Environ&) override;
};

struct Bracket_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyse(Environ&) const override;
};

struct Brace_Phrase : public Delimited_Phrase
{
    using Delimited_Phrase::Delimited_Phrase;
    virtual Shared<Meaning> analyse(Environ&) const override;
};

struct Program_Phrase : public Phrase
{
    Shared<Phrase> body_;
    Token end_;

    Program_Phrase(Shared<Phrase> body, Token end)
    : body_(std::move(body)), end_(end)
    {}

    virtual Location location() const override
    {
        return body_->location().ending_at(end_);
    }

    virtual Shared<Meaning> analyse(Environ&) const override;
    virtual Shared<Definition> as_definition(Environ&) override;
};

/// A function call. Call_Phrase is the union of all syntaxes that denote
/// function calls. op_.kind_ specifies which syntax was used.
struct Call_Phrase : public Phrase
{
    Shared<Phrase> function_;
    Token op_;  ///! k_missing or k_left_call or k_right_call or k_backtick
    Token op2_; ///! k_missing or k_backtick
    Shared<Phrase> arg_;

    Call_Phrase(
        Shared<Phrase> function,
        Shared<Phrase> arg,
        Token op = {},
        Token op2 = {})
    :
        function_(std::move(function)),
        op_(op),
        arg_(std::move(arg))
    {}

    virtual Location location() const override
    {
        if (op_.kind_ == Token::k_backtick)
            return arg_->location();
        else if (op_.kind_ == Token::k_right_call)
            return arg_->location().ending_at(function_->location().token());
        else
            return function_->location().ending_at(arg_->location().token());
    }
    virtual Shared<Meaning> analyse(Environ&) const override;
    std::vector<Shared<Operation>> analyse_args(Environ& env) const;
};

/// Iterate over each argument in a function call argument list (Call_Phrase)
/// or each parameter in a function formal parameter list (Lambda_Phrase).
inline void
each_argument(const Phrase& args, std::function<void(const Phrase&)> func)
{
    if (auto parens = dynamic_cast<const Paren_Phrase*>(&args)) {
        if (dynamic_cast<const Empty_Phrase*>(&*parens->body_))
            return;
        if (auto commas = dynamic_cast<const Comma_Phrase*>(&*parens->body_)) {
            for (auto a : commas->args_)
                func(*a.expr_);
        } else {
            func(*parens->body_);
        }
    } else
        func(args);
}

struct If_Phrase : public Phrase
{
    Token if_;
    Shared<Phrase> condition_;
    Shared<Phrase> then_expr_;
    Token else_;
    Shared<Phrase> else_expr_;

    If_Phrase(
        Token t_if,
        Shared<Phrase> condition,
        Shared<Phrase> then_expr,
        Token t_else,
        Shared<Phrase> else_expr)
    :
        if_(t_if),
        condition_(condition),
        then_expr_(then_expr),
        else_(t_else),
        else_expr_(else_expr)
    {}

    virtual Location location() const override
    {
        if (else_expr_ == nullptr)
            return then_expr_->location().starting_at(if_);
        else
            return else_expr_->location().starting_at(if_);
    }
    virtual Shared<Meaning> analyse(Environ&) const override;
};

/// This is a generic syntax scheme: keyword (args) body,
/// used by many OpenSCAD primitives, and adopted by Curv.
struct Control_Phrase : public Phrase
{
    Token keyword_;
    Shared<Paren_Phrase> args_;
    Shared<Phrase> body_;

    Control_Phrase(
        Token keyword,
        Shared<Paren_Phrase> args,
        Shared<Phrase> body)
    :
        keyword_(keyword),
        args_(args),
        body_(body)
    {}

    virtual Location location() const override
    {
        return body_->location().starting_at(keyword_);
    }
};

struct For_Phrase : public Phrase
{
    Token keyword_;
    Token lparen_;
    Shared<const Phrase> pattern_;
    Token in_;
    Shared<const Phrase> listexpr_;
    Token rparen_;
    Shared<const Phrase> body_;

    For_Phrase(
        Token keyword,
        Token lparen,
        Shared<const Phrase> pattern,
        Token in,
        Shared<const Phrase> listexpr,
        Token rparen,
        Shared<const Phrase> body)
    :
        keyword_(keyword),
        lparen_(lparen),
        pattern_(pattern),
        in_(in),
        listexpr_(listexpr),
        rparen_(rparen),
        body_(body)
    {}

    virtual Shared<Meaning> analyse(Environ&) const override;
    virtual Location location() const override
    {
        return body_->location().starting_at(keyword_);
    }
};

struct While_Phrase : public Control_Phrase
{
    using Control_Phrase::Control_Phrase;
    virtual Shared<Meaning> analyse(Environ&) const override;
};

struct Range_Phrase : public Phrase
{
    Range_Phrase(
        Shared<Phrase> first,
        Token op1,
        Shared<Phrase> last,
        Token op2,
        Shared<Phrase> step)
    :
        first_(std::move(first)),
        op1_(op1),
        last_(std::move(last)),
        op2_(op2),
        step_(std::move(step))
    {}
    Shared<Phrase> first_;
    Token op1_;
    Shared<Phrase> last_;
    Token op2_;
    Shared<Phrase> step_;
    virtual Location location() const override
    {
        if (step_ != nullptr) {
            return first_->location().ending_at(step_->location().token());
        } else {
            return first_->location().ending_at(last_->location().token());
        }
    }
    virtual Shared<Meaning> analyse(Environ&) const override;
};

struct Let_Phrase : public Phrase
{
    Token let_;
    Shared<Phrase> bindings_;
    Token in_;
    Shared<const Phrase> body_;

    Let_Phrase(
        Token let,
        Shared<Phrase> bindings,
        Token in,
        Shared<const Phrase> body)
    :
        let_(let),
        bindings_(bindings),
        in_(in),
        body_(body)
    {}

    virtual Shared<Meaning> analyse(Environ&) const override;
    virtual Location location() const override
    {
        return body_->location().starting_at(let_);
    }
};

/// In the grammar, a <list> phrase is zero or more constituent phrases
/// separated by commas or semicolons.
/// This function iterates over each constituent phrase.
void each_item(Phrase& phrase, std::function<void(Phrase&)> func);

} // namespace curv
#endif // header guard
