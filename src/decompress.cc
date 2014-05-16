/*
 * FILE             $Id: decompress.cc 9 2008-05-30 14:56:21Z burlog $
 *
 * PROJECT          KHTML JavaScript decompress utility
 *
 * DESCRIPTION      DeCompress string dump
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * LICENSE          see COPYING
 *
 * Copyright (C) Seznam.cz a.s. 2007
 * All Rights Reserved
 *
 * HISTORY
 *       2007-03-09 (bukovsky)
 *                  First draft.
 */

#include <iostream>

#include "util.h"
#include "decompress.h"
#include "kjs/nodes.h"

using namespace KJS;

DeCompressStream_t &operator<<(DeCompressStream_t &cs, const KJS::Node *node) {
    if (node) node->streamTo(cs);
    return cs;
}

DeCompressStream_t &operator<<(DeCompressStream_t &cs,
                               const KJS::Identifier &value) {
    cs.os << value.ustring().ascii();
    return cs;
}

DeCompressStream_t &operator<<(DeCompressStream_t &cs,
                               const std::string &value) {
    cs.os << value;
    return cs;
}

DeCompressStream_t &operator<<(DeCompressStream_t &cs,
        const char *value) {
    cs.os << value;
    return cs;
}

DeCompressStream_t &operator<<(DeCompressStream_t &cs,
        const char &value) {
    cs.os << value;
    return cs;
}

DeCompressStream_t &operator<<(DeCompressStream_t &cs,
                               DeCompressStream_t::Format_t value) {
    if (value == DeCompressStream_t::ENDL)
        cs.os << std::endl << cs.indent;
    else if (value == DeCompressStream_t::INDENT)
        cs.indent += "    ";
    else if (value == DeCompressStream_t::UNINDENT)
        cs.indent.resize(cs.indent.size() - 4);
    return cs;
}

/**
 * @short dump node tree to stream and return stream.
 * @param node node tree to dump.
 */
DeCompressStream_t::DeCompressStream_t(const Node *node) {
    if (node) node->streamTo(*this);
}

/*
 * Node dump rules
 */
void NullNode::streamTo(DeCompressStream_t &os) const {
    os << "null";
}

void BooleanNode::streamTo(DeCompressStream_t &os) const {
    os << (val ? "true" : "false");
}

void NumberNode::streamTo(DeCompressStream_t &os) const {
    os << UString::from(val).ascii();
}

void StringNode::streamTo(DeCompressStream_t &os) const {
    os << '"' << escape(val.ascii()) << '"';
}

void RegExpNode::streamTo(DeCompressStream_t &os) const {
    os << "/" << pattern.ascii();
    os << "/" << flags.ascii();
}

void ThisNode::streamTo(DeCompressStream_t &os) const {
    os << "this";
}

void ResolveNode::streamTo(DeCompressStream_t &os) const {
    os << ident;
}

void GroupNode::streamTo(DeCompressStream_t &os) const {
    os << "(" << group << ")";
}

void ElementNode::streamTo(DeCompressStream_t &os) const {
    for (const ElementNode *n = this; n; n = n->list) {
        for (int i = 0; i < n->elision; i++)
            os << ", ";
        os << n->node;
        if (n->list)
            os << ", ";
    }
}

void ArrayNode::streamTo(DeCompressStream_t &os) const {
    os << "[" << element;
    for (int i = 0; i < elision; i++)
        os << ", ";
    os << "]";
}

void ObjectLiteralNode::streamTo(DeCompressStream_t &os) const {
    if (list)
        os << "{" << list << "}";
    else
        os << "{}";
}

void PropertyValueNode::streamTo(DeCompressStream_t &os) const {
    int count = 0;
    for (const PropertyValueNode *n = this; n; n = n->list)
        os << ((count++)? ", ": "")
            << "\"" << n->name << "\"" << ": " << n->assign;
}

void PropertyNode::streamTo(DeCompressStream_t &os) const {
    if (str.isNull())
        os << UString::from(numeric).ascii();
    else
        os << escape(str.ascii());
}

void AccessorNode1::streamTo(DeCompressStream_t &os) const {
    os << expr1 << "[" << expr2 << "]";
}

void AccessorNode2::streamTo(DeCompressStream_t &os) const {
    os << expr << "." << ident;
}

void ArgumentListNode::streamTo(DeCompressStream_t &os) const {
    os << expr;
    for (ArgumentListNode *n = list; n; n = n->list)
        os << ", " << n->expr;
}

void ArgumentsNode::streamTo(DeCompressStream_t &os) const {
    os << "(" << list << ")";
}

void NewExprNode::streamTo(DeCompressStream_t &os) const {
    os << "new " << expr << args;
}

void FunctionCallNode::streamTo(DeCompressStream_t &os) const {
    os << expr << args;
}

void PostfixNode::streamTo(DeCompressStream_t &os) const {
    os << expr;
    if (oper == OpPlusPlus)
        os << "++";
    else
        os << "--";
}

void DeleteNode::streamTo(DeCompressStream_t &os) const {
    os << "delete " << expr;
}

void VoidNode::streamTo(DeCompressStream_t &os) const {
    os << "void " << expr;
}

void TypeOfNode::streamTo(DeCompressStream_t &os) const {
    os << "typeof " << expr;
}

void PrefixNode::streamTo(DeCompressStream_t &os) const {
    os << (oper == OpPlusPlus ? "++" : "--") << expr;
}

void UnaryPlusNode::streamTo(DeCompressStream_t &os) const {
    os << " + " << expr;
}

void NegateNode::streamTo(DeCompressStream_t &os) const {
    os << " - " << expr;
}

void BitwiseNotNode::streamTo(DeCompressStream_t &os) const {
    os << " ~ " << expr;
}

void LogicalNotNode::streamTo(DeCompressStream_t &os) const {
    os << "! " << expr;
}

void MultNode::streamTo(DeCompressStream_t &os) const {
    os << term1 << " " << oper << " " << term2;
}

void AddNode::streamTo(DeCompressStream_t &os) const {
    os << term1 << " " << oper << " " << term2;
}

void AppendStringNode::streamTo(DeCompressStream_t &os) const {
    os << term << " + " << '"' << escape(str.ascii()) << '"';
}

void ShiftNode::streamTo(DeCompressStream_t &os) const {
    os << term1;
    if (oper == OpLShift)
        os << " << ";
    else if (oper == OpRShift)
        os << " >> ";
    else
        os << " >>> ";
    os << term2;
}

void RelationalNode::streamTo(DeCompressStream_t &os) const {
    os << expr1;
    switch (oper) {
    case OpLess:
        os << " < ";
        break;
    case OpGreater:
        os << " > ";
        break;
    case OpLessEq:
        os << " <= ";
        break;
    case OpGreaterEq:
        os << " >= ";
        break;
    case OpInstanceOf:
        os << " instanceof ";
        break;
    case OpIn:
        os << " in ";
        break;
    default:
        ;
    }
    os << expr2;
}

void EqualNode::streamTo(DeCompressStream_t &os) const {
    os << expr1;
    switch (oper) {
    case OpEqEq:
        os << " == ";
        break;
    case OpNotEq:
        os << " != ";
        break;
    case OpStrEq:
        os << " === ";
        break;
    case OpStrNEq:
        os << " !== ";
        break;
    default:
        ;
    }
    os << expr2;
}

void BitOperNode::streamTo(DeCompressStream_t &os) const {
    os << expr1;
    if (oper == OpBitAnd)
        os << " & ";
    else if (oper == OpBitXOr)
        os << " ^ ";
    else
        os << " | ";
    os << expr2;
}

void BinaryLogicalNode::streamTo(DeCompressStream_t &os) const {
    os << expr1 << (oper == OpAnd ? " && " : " || ") << expr2;
}

void ConditionalNode::streamTo(DeCompressStream_t &os) const {
    os << logical << "? " << expr1 << ": " << expr2;
}

void AssignNode::streamTo(DeCompressStream_t &os) const {
    os << left;
    const char *opStr;
    switch (oper) {
    case OpEqual:
        opStr = " = ";
        break;
    case OpMultEq:
        opStr = " *= ";
        break;
    case OpDivEq:
        opStr = " /= ";
        break;
    case OpPlusEq:
        opStr = " += ";
        break;
    case OpMinusEq:
        opStr = " -= ";
        break;
    case OpLShift:
        opStr = " << =";
        break;
    case OpRShift:
        opStr = " >> =";
        break;
    case OpURShift:
        opStr = " >> =";
        break;
    case OpAndEq:
        opStr = " &= ";
        break;
    case OpXOrEq:
        opStr = " ^= ";
        break;
    case OpOrEq:
        opStr = " |= ";
        break;
    case OpModEq:
        opStr = " %= ";
        break;
    default:
        opStr = " ?= ";
    }
    os << opStr << expr;
}

void CommaNode::streamTo(DeCompressStream_t &os) const {
    os << expr1 << ", " << expr2;
}

void StatListNode::streamTo(DeCompressStream_t &os) const {
    for (const StatListNode *n = this; n; n = n->list)
        os << n->statement;
}

void AssignExprNode::streamTo(DeCompressStream_t &os) const {
    os << " = " << expr;
}

void VarDeclNode::streamTo(DeCompressStream_t &os) const {
    os << ident << init;
}

void VarDeclListNode::streamTo(DeCompressStream_t &os) const {
    os << var;
    for (VarDeclListNode *n = list; n; n = n->list)
        os << ", " << n->var;
}

void VarStatementNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "var " << list << ";";
}

void BlockNode::streamTo(DeCompressStream_t &os) const {
      os << DeCompressStream_t::ENDL << "{"
          << DeCompressStream_t::INDENT << source
          << DeCompressStream_t::UNINDENT
          << DeCompressStream_t::ENDL << "}";
}

void EmptyStatementNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << ";";
}

void ExprStatementNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << expr << ";";
}

void IfNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "if (" << expr << ")"
        << DeCompressStream_t::INDENT << statement1
        << DeCompressStream_t::UNINDENT;

    if (statement2) {
        os << DeCompressStream_t::ENDL << "else"
            << DeCompressStream_t::INDENT;

        // do block
        if (!dynamic_cast<BlockNode *>(statement2))
            os << DeCompressStream_t::ENDL << "{" << DeCompressStream_t::INDENT
                << statement2 << DeCompressStream_t::UNINDENT
                << DeCompressStream_t::ENDL << "}"
                << DeCompressStream_t::UNINDENT;
        else
            os << statement2 << DeCompressStream_t::UNINDENT;
    }
}

void DoWhileNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "do" << DeCompressStream_t::INDENT;
    if (!dynamic_cast<BlockNode *>(statement))
        os << DeCompressStream_t::ENDL << "{" << DeCompressStream_t::INDENT
            << statement << DeCompressStream_t::UNINDENT
            << DeCompressStream_t::ENDL << "}";
    else
        os << statement;
    os << DeCompressStream_t::UNINDENT
        << DeCompressStream_t::ENDL << "while (" << expr << ");";
}

void WhileNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "while (" << expr << ")"
        << DeCompressStream_t::INDENT << statement
        << DeCompressStream_t::UNINDENT;
}

void ForNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "for (" << ((var)?"var ": "")
        << expr1 << "; " << expr2 << "; " << expr3 << ")"
        << DeCompressStream_t::INDENT << statement
        << DeCompressStream_t::UNINDENT;
}

void ForInNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "for (";
    if (varDecl)
        os << "var " << varDecl;
    else
        os << lexpr;
    if (init)
        os << "=" << init;
    os << " in " << expr << ")" << DeCompressStream_t::INDENT
        << statement << DeCompressStream_t::UNINDENT;
}

void ContinueNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "continue";
    if (!ident.isNull())
        os << " " << ident;
    os << ";";
}

void BreakNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "break";
    if (!ident.isNull())
        os << " " << ident;
    os << ";";
}

void ReturnNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "return";
    if (value)
        os << " " << value;
    os << ";";
}

void WithNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "with(" << expr << ")" << statement;
}

void CaseClauseNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL;
    if (expr)
        os << "case " << expr;
    else
        os << "default";
    os << ":" << DeCompressStream_t::INDENT;
    if (list)
        os << list;
    os << DeCompressStream_t::UNINDENT;
}

void ClauseListNode::streamTo(DeCompressStream_t &os) const {
    for (const ClauseListNode *n = this; n; n = n->next())
        os << n->clause();
}

void CaseBlockNode::streamTo(DeCompressStream_t &os) const {
    for (const ClauseListNode *n = list1; n; n = n->next())
        os << n->clause();
    if (def)
        os << def;
    for (const ClauseListNode *n = list2; n; n = n->next())
        os << n->clause();
}

void SwitchNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "switch(" << expr << ") {"
        << DeCompressStream_t::INDENT << block << DeCompressStream_t::UNINDENT
        << DeCompressStream_t::ENDL << "}";
}

void LabelNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << label << ":"
        << DeCompressStream_t::INDENT << statement
        << DeCompressStream_t::UNINDENT;
}

void ThrowNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "throw " << expr << ";";
}

void CatchNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "catch(" << ident << ")" << block;
}

void FinallyNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "finally " << block;
}

void TryNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "try " << block << _catch << _final;
}

void ParameterNode::streamTo(DeCompressStream_t &os) const {
    os << id;
    for (ParameterNode *n = next; n; n = n->next)
        os << ", " << n->id;
}

void FuncDeclNode::streamTo(DeCompressStream_t &os) const {
    os << DeCompressStream_t::ENDL << "function " << ident << "(";
    if (param)
        os << param;
    os << ")" << body;
}

void FuncExprNode::streamTo(DeCompressStream_t &os) const {
    os << "function" << "(" << param << ")" << body;
}

void SourceElementsNode::streamTo(DeCompressStream_t &os) const {
    for (const SourceElementsNode *n = this; n; n = n->elements)
        os << n->element;
}

