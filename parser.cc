// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/parse.h>
#include <curv/scanner.h>
#include <curv/exception.h>

using namespace std;
using namespace curv;
using namespace aux;

namespace curv {

Shared_Ptr<Syntax> parse_sum(Scanner&);
Shared_Ptr<Syntax> parse_product(Scanner&);
Shared_Ptr<Syntax> parse_unary(Scanner&);
Shared_Ptr<Syntax> parse_atom(Scanner&);

// Parse a script, return a syntax tree.
// It's a recursive descent parser.
/*
sum : product | sum + product | sum - product
product : unary | product * unary | product / unary
unary : atom | - unary | + unary
atom : numeral | ( sum )
*/
Shared_Ptr<Syntax>
parse(const Script& script)
{
    Scanner scanner(script);
    auto expr = parse_sum(scanner);
    auto tok = scanner.get_token();
    if (tok.kind != Token::k_end)
        throw SyntaxError(tok, "unexpected token at end of script");
    return expr;
}

// sum : product | sum + product | sum - product
Shared_Ptr<Syntax>
parse_sum(Scanner& scanner)
{
    auto left = parse_product(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind) {
        case Token::k_plus:
        case Token::k_minus:
            left = aux::make_shared<BinaryExpr>(
                tok, std::move(left), parse_product(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// product : unary | product * unary | product / unary
Shared_Ptr<Syntax>
parse_product(Scanner& scanner)
{
    auto left = parse_unary(scanner);
    for (;;) {
        auto tok = scanner.get_token();
        switch (tok.kind) {
        case Token::k_times:
        case Token::k_over:
            left = aux::make_shared<BinaryExpr>(
                tok, std::move(left), parse_unary(scanner));
            continue;
        default:
            scanner.push_token(tok);
            return left;
        }
    }
}

// unary : atom | - unary | + unary
Shared_Ptr<Syntax>
parse_unary(Scanner& scanner)
{
    auto tok = scanner.get_token();
    switch (tok.kind) {
    case Token::k_plus:
    case Token::k_minus:
        return aux::make_shared<UnaryExpr>(tok, parse_unary(scanner));
    default:
        scanner.push_token(tok);
        return parse_atom(scanner);
    }
}

// atom : numeral | ( sum )
Shared_Ptr<Syntax>
parse_atom(Scanner& scanner)
{
    auto tok = scanner.get_token();
    if (tok.kind == Token::k_num) {
        return aux::make_shared<NumExpr>(tok);
    }
    if (tok.kind == Token::k_ident) {
        return aux::make_shared<IdentExpr>(tok);
    }
    if (tok.kind == Token::k_lparen) {
        auto expr = parse_sum(scanner);
        auto tok2 = scanner.get_token();
        if (tok2.kind == Token::k_rparen)
            return aux::make_shared<ParenExpr>(tok,std::move(expr),tok2);
        throw SyntaxError(tok2, "unexpected token when expecting ')'");
    }
    throw SyntaxError(tok, "unexpected token when expecting atom");
}

}
