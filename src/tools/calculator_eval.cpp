// src/tools/calculator_eval.cpp — YOU IMPLEMENT (spec §4.6).
//
// calc_evaluate: evaluate a well-formed token list with two stacks
// (one for numbers, one for operators and '('). See spec §4.6 and the
// worked trace in Appendix C. std::stack or std::vector are both fine.

#include "tools/calculator.h"
#include <stack>

// how important is this operator, bigger number = goes first
static int precedence(CalcToken::Kind k) {
    if (k == CalcToken::Kind::Times || k == CalcToken::Kind::Divide) {
        return 2;
    }
    return 1; // plus or minus
}

// pop the top operator and do the math on the top two values returns false if something goes wrong (like divide by zero or empty stack)
static bool apply_op(std::stack<double>* values, std::stack<CalcToken::Kind>* ops, std::string* error) {
    if (ops->empty() || values->size() < 2) {
        *error = "bad expression";
        return false;
    }

    CalcToken::Kind op = ops->top();
    ops->pop();

    // right comes off first, then left
    double right = values->top();
    values->pop();
    double left = values->top();
    values->pop();

    double result = 0.0;

    if (op == CalcToken::Kind::Plus) {
        result = left + right;
    } else if (op == CalcToken::Kind::Minus) {
        result = left - right;
    } else if (op == CalcToken::Kind::Times) {
        result = left * right;
    } else if (op == CalcToken::Kind::Divide) {
        if (right == 0.0) {
            *error = "division by zero";
            return false;
        }
        result = left / right;
    }

    values->push(result);
    return true;
}

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out, std::string* error) {
    std::stack<double> values;
    std::stack<CalcToken::Kind> ops;

    for (const CalcToken& t : tokens) {
        if (t.kind == CalcToken::Kind::Number) {
            values.push(t.value);
        } else if (t.kind == CalcToken::Kind::LParen) {
            ops.push(t.kind);
        } else if (t.kind == CalcToken::Kind::RParen) {
            // apply stuff until we hit the matching (
            while (!ops.empty() && ops.top() != CalcToken::Kind::LParen) {
                if (!apply_op(&values, &ops, error)) {
                    return false;
                }
            }
            if (ops.empty()) {
                *error = "mismatched parens";
                return false;
            }
            ops.pop(); // pop the (
        } else {
            // its one of the basics apply anything on top that has >= precedence first
            while (!ops.empty() && ops.top() != CalcToken::Kind::LParen && precedence(ops.top()) >= precedence(t.kind)) {
                if (!apply_op(&values, &ops, error)) {
                    return false;
                }
            }
            ops.push(t.kind);
        }
    }

    // apply whatever is left
    while (!ops.empty()) {
        if (!apply_op(&values, &ops, error)) {
            return false;
        }
    }

    if (values.size() != 1) {
        *error = "bad expression";
        return false;
    }

    *out = values.top();
    return true;
}