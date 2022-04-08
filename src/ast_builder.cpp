#include "ast_builder.h"
#include "token.h"
#include "token_parser.h"
#include "AST.h"
#include <map>
#include <stack>

namespace compiler {

    /**
     * @brief 
     * bnf : ("{"expr"}"|NUMBER|IDENTIFIER|STRING) {postfix}
     *  value
     *  1234
     * "hello"
     * 1+2+value
     * func(a,b,c)
     * 
     * @param tokenParser 
     * @return MatchResult 
     */
    MatchResult matchPrimary(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        auto startToken = *token;
        ASTNode* operand = nullptr;
        if (token->type() == TokenType::Integer) {
            tokenParser->peek();
            operand = new ASTLeaf(ASTNodeType::Integer, *token);
        } else if (token->type() == TokenType::Identifier) {
            tokenParser->peek();
            operand = new ASTLeaf(ASTNodeType::Identifier, *token);
        }
        else if (token->type() == TokenType::LeftParen) {
            tokenParser->peek();
            auto expr = matchExpression(tokenParser);
            if(!expr) {
                return expr; // match expression failed
            }
            auto nextToken = tokenParser->nextToken();
            if(nextToken->type() == TokenType::RightParen) {
                tokenParser->peek();
                operand = expr.node;
            } else {
                delete expr.node;
                MatchResult rst = { nullptr, ASTParseError::RightParenExpected, nextToken->line(), nextToken->column() };
                return rst;
            }
        }
        if(!operand) {
            return { nullptr, ASTParseError::PrimaryMismatch, startToken.line(), startToken.column() };
        }
        ASTNode* postfix = nullptr;
        auto expr = matchPostfix(tokenParser);
        postfix = expr.node;
        ASTPrimary* rst = new ASTPrimary(operand, postfix);
        return { rst, ASTParseError::None, startToken.line(), startToken.column() };
    }

    MatchResult matchFactor(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        auto startToken = *token;
        if (token->type() == TokenType::Minus) {
            tokenParser->peek();
            auto primNode = matchPrimary(tokenParser);
            if(primNode) {
                MatchResult rst =  { 
                    new ASTNegativeExpression(primNode.node), 
                    ASTParseError::None,
                    startToken.line(),
                    startToken.column()
                };
                return rst;
            } else {
                return primNode; // failed
            }
        }
        return matchPrimary(tokenParser);
    }

    MatchResult matchExpression(TokenParser* tokenParser) {
        auto startToken = *tokenParser->nextToken();
        std::stack<ASTNode*> factorStack;
        std::stack<TokenType> opStack;
        auto factor = matchFactor(tokenParser);
        if(factor) {
            factorStack.push(factor.node);
            while(true) {
                Token const* nextToken = tokenParser->nextToken();
                switch(nextToken->type()) {
                    case TokenType::Minus:
                    case TokenType::Slash:
                    case TokenType::Plus:
                    case TokenType::Star:
                    case TokenType::Equal:
                    case TokenType::Modulus:
                    case TokenType::Less:
                    case TokenType::LessEqual:
                    case TokenType::Greater:
                    case TokenType::GreaterEqual:
                    case TokenType::EqualEqual:
                        break;
                    default:
                        // expr pattern end
                        while(!opStack.empty()) {
                            auto op = opStack.top();
                            opStack.pop();
                            auto right = factorStack.top();
                            factorStack.pop();
                            auto left = factorStack.top();
                            factorStack.pop();
                            auto binExpr = new ASTBinaryOpExpr(left, right, op);
                            factorStack.push(binExpr);
                        }
                        auto rst = factorStack.top();
                        assert(factorStack.size() == 1);
                        return { rst, ASTParseError::None, startToken.line(), startToken.column() };
                }
                tokenParser->peek();
                TokenType op = nextToken->type();
                if(opStack.size()) {
                    if(op>=opStack.top()) {
                        auto factor1 = factorStack.top(); factorStack.pop();
                        auto factor2 = factorStack.top(); factorStack.pop();
                        auto binExpr = new ASTBinaryOpExpr(factor1, factor2, opStack.top());
                        opStack.pop();
                        factorStack.push(binExpr);
                    }
                }
                opStack.push(op);
                factor = matchFactor(tokenParser);
                if(factor) {
                    factorStack.push(factor.node);
                } else { 
                    // clean up status & throw the exception
                    while(!factorStack.empty()) {
                        auto factor = factorStack.top();
                        delete factor;
                        factorStack.pop();
                    }
                    auto lastToken = tokenParser->nextToken();
                    MatchResult rst = { nullptr, ASTParseError::NeedRightFactor, lastToken->line(), lastToken->column() };
                    return rst;
                }
            }
        }
        return factor;
    }

    MatchResult matchBlock(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        if(token->type() == TokenType::LeftBrace) {
            tokenParser->peek();
            auto rst = new ASTMultiExpr(ASTNodeType::Block);
            auto statement = matchStatement(tokenParser);
            if(statement) {
                rst->addExpr(statement.node);
            }
            while(true) {
                token = tokenParser->nextToken();
                if(token->type() == TokenType::Semicolon || token->type() == TokenType::Eol) {
                    tokenParser->peek();
                } else if(token->type() == TokenType::RightBrace) {
                    tokenParser->peek();
                    return { rst, ASTParseError::None, token->line(), token->column() };
                } else {
                    statement = matchStatement(tokenParser);
                    if(statement) {
                        rst->addExpr(statement.node);
                    } else {
                        delete rst;
                        return statement;
                    }
                }
            }
            return { rst, ASTParseError::None, token->line(), token->column() };
        }
        return { nullptr, ASTParseError::BlockMismatch, token->line(), token->column() };
    }

    MatchResult matchStatement(TokenParser* tokenParser) { 
        auto& keywords = tokenParser->keywords();
        auto token = tokenParser->nextToken();
        if(token->stringLiteral() == keywords._if) {
            auto IfToken = *token;
            ASTIfStatement* if_stmt = new ASTIfStatement();
            tokenParser->peek();
            auto expr = matchExpression(tokenParser);
            if(!expr) {
                return expr;
            }
            if_stmt->setCondition(expr.node);
            auto block = matchBlock(tokenParser);
            if(!block) {
                delete if_stmt;
                return block;
            }
            if_stmt->setThenBranch(block.node);
            token = tokenParser->nextToken();
            if(token->stringLiteral() == keywords._else) {
                tokenParser->peek();
                auto block = matchBlock(tokenParser);
                if(!block) {
                    delete if_stmt;
                    return block;
                }
                if_stmt->setElseBranch(block.node);
            }
            return { if_stmt, ASTParseError::None, IfToken.line(), IfToken.column() };
        } else if(token->stringLiteral() == keywords._while) {
            auto WhileToken = *token;
            tokenParser->peek();
            auto expr = matchExpression(tokenParser);
            if(!expr) {
                return expr;
            }
            auto block = matchBlock(tokenParser);
            if(!block) {
                delete expr.node;
                return block;
            }
            auto while_stmt = new ASTWhileStatement();
            while_stmt->setBody(block.node);
            while_stmt->setCondition(expr.node);
            return { while_stmt, ASTParseError::None, WhileToken.line(), WhileToken.column() };
        } else {
            auto expr = matchExpression(tokenParser);
            return expr;
        }
    }

    MatchResult matchProgram(TokenParser* tokenParser) {
        auto program = new ASTMultiExpr(ASTNodeType::Program);
        while(true) {
            auto pos = tokenParser->pos();
            auto stmt = matchStatement(tokenParser);
            if(!stmt) {
                stmt = matchFunctionDef(tokenParser);
            }
            if(stmt) {
                program->addExpr(stmt.node);
            } else {
                tokenParser->peek();
                tokenParser->nextToken();
            }
            if(pos != tokenParser->pos()) {
                continue;
            } else {
                break;
            }
        }
        return { program, ASTParseError::None, 0, 0 };
    }

    /**
     * @brief 
     *  bnf : params : param {, param}
     * @param tokenParser 
     * @return MatchResult 
     */
    MatchResult matchParams(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        Token startToken = *token;
        if(token->type() != TokenType::Identifier) {
            return MatchResult { nullptr, ASTParseError::ParamsListMismatch, token->line(), token->column() };
        }
        tokenParser->peek();
        auto params = new ASTMultiExpr(ASTNodeType::Params);
        auto param = new ASTLeaf(ASTNodeType::Identifier, *token);
        params->addExpr(param);
        while(true) {
            token = tokenParser->nextToken();
            if(token->type() == TokenType::Comma) {
                tokenParser->peek();
                token = tokenParser->nextToken();
                if(token->type() != TokenType::Identifier) {
                    delete params;
                    return MatchResult { nullptr, ASTParseError::ShouldFollowIdentifier, token->line(), token->column() };
                }
                param = new ASTLeaf(ASTNodeType::Identifier, *token);
                tokenParser->peek();
                params->addExpr(param);
            } else {
                break;
            }
        }
        return { params, ASTParseError::None, startToken.line(), startToken.column() };
    }

    /**
     * @brief 
     *  bnf : paramlist : "(" params ")"
     * @param tokenParser 
     * @return MatchResult 
     *  if no params, return nullptr
     */
    MatchResult matchParamList(TokenParser* tokenParser) {
        auto currToken = tokenParser->nextToken();
        auto startToken = *currToken;
        if(currToken->type() == TokenType::LeftParen) {
            tokenParser->peek();
            auto params = matchParams(tokenParser); // maybe no params
            currToken = tokenParser->nextToken();
            if(currToken->type() == TokenType::RightParen) {
                tokenParser->peek();
                params.error = ASTParseError::None; // if has error, set as no error!
                return params; // success, maybe no params
            } else {
                delete params.node; // clean up
                return { nullptr, ASTParseError::RightParenExpected, currToken->line(), currToken->column() }; // failed
            }
        }
        return {nullptr, ASTParseError::ParamsListMismatch, startToken.line(), startToken.column()}; // failed
    }

    /**
     * @brief 
     * bnf : function : "func" ident "(" paramlist ")" block
     * 
     * @param tokenParser 
     * @return MatchResult 
     */
    MatchResult matchFunctionDef(TokenParser* tokenParser) {
        auto const& keywords = tokenParser->keywords();
        auto token = tokenParser->nextToken();
        Token startToken = *token;
        if(token->stringLiteral() != keywords._func) { // "func"
            return MatchResult { nullptr, ASTParseError::FunctionMismatch, token->line(), token->column() };
        }
        tokenParser->peek();
        token = tokenParser->nextToken();
        if(token->type() != TokenType::Identifier) { // ident
            return MatchResult { nullptr, ASTParseError::MissFunctionName, token->line(), token->column() };
        }
        tokenParser->peek();
        auto func = new ASTFunction();
        func->setName(token->stringLiteral());
        auto paramList = matchParamList(tokenParser); // paramlist
        if(!paramList) {
            delete func;
            return paramList; // failed
        }
        func->setParams(paramList.node);
        auto block = matchBlock(tokenParser); // body
        if(!block) {
            delete func;
            return block; // failed
        }
        func->setBody(block.node);
        return { func, ASTParseError::None, startToken.line(), startToken.column() };
    }


    /**
     * @brief 
     * bnf : expr {, expr}
     * 
     * @param tokenParser 
     * @return MatchResult 
     */
    MatchResult matchArgs(TokenParser* tokenParser) {
        auto beginToken = *tokenParser->nextToken();
        auto multiExpr = new ASTMultiExpr(ASTNodeType::Args);
        auto expr = matchExpression(tokenParser);
        if(!expr) {
            return expr;
        }
        multiExpr->addExpr(expr.node);
        while(true) {
            auto token = tokenParser->nextToken();
            if(token->type() == TokenType::Comma) {
                tokenParser->peek();
                expr = matchExpression(tokenParser);
                if(!expr) {
                    delete multiExpr;
                    return expr;
                }
            } else {
                break;
            }
        }
        return { multiExpr, ASTParseError::None, beginToken.line(), beginToken.column() };
    }
    
    /**
     * @brief 
     *  bnf : "(" [args] ")"
     * 
     * @param tokenParser 
     * @return MatchResult 
     */
    MatchResult matchPostfix(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        Token startToken = *token;
        if(token->type() != TokenType::LeftParen) {
            return MatchResult { nullptr, ASTParseError::LeftParenExpected, token->line(), token->column() };
        }
        tokenParser->peek();
        auto args = matchArgs(tokenParser);
        token = tokenParser->nextToken();
        if(token->type() != TokenType::RightParen) {
            delete args.node;
            return MatchResult { nullptr, ASTParseError::RightParenExpected, token->line(), token->column() };
        }
        tokenParser->peek();
        return { args.node, ASTParseError::None, startToken.line(), startToken.column() };
    }
}