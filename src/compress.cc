/*
 * FILE             $Id: compress.cc 21 2011-03-04 08:28:25Z burlog $
 *
 * PROJECT          KHTML JavaScript compress utility
 *
 * DESCRIPTION      Compress string dump
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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string.h>
#include <errno.h>
#include <algorithm>
#include <typeinfo>

#include "util.h"
#include "compress.h"
#include "blacklist.h"
#include "kjs/nodes.h"

using namespace KJS;

namespace {

std::ostream &operator<<(std::ostream &os,
                         const CompressStream_t::IdentifierMap_t &idmap) {
    // simple dump map to stream
    for (CompressStream_t::IdentifierMap_t::const_iterator iid = idmap.begin();
            iid != idmap.end(); ++iid)
        os << iid->second << ": " << iid->first << std::endl;
    return os;
}

std::ostream &operator<<(std::ostream &os,
                         const CompressStream_t::StringSet_t &sset) {
    // simple dump set to stream
    for (CompressStream_t::StringSet_t::const_iterator isset = sset.begin();
            isset != sset.end(); ++isset)
        os << *isset << std::endl;
    return os;
}

} // namespace

/**
 * @short convert int to base62 digit
 * @param x int from <0, 62>
 * @return char represent base62 digit
 */
char Base62Int_t::toDigit(unsigned int x) const {
    if (x >= 0 && x <= 9)
        return '0' + x;

    else if (x >= 10 && x <= 35)
        return 'a' + x - 10;

    else if (x >= 36 && x <= 61)
        return 'A' + x - 36;

    return '0';
}

std::string Base62Int_t::str() const {
    unsigned int x = i;
    std::string s = "";

    // convert from dec to base62
    while (x != 0) {
        s.append(1, toDigit(x % 62));
        x /= 62;
    }

    if (s.empty())
        return "0";

    std::reverse(s.begin(), s.end());
    return s;
}

CompressStream_t &operator<<(CompressStream_t &cs, const KJS::Node *node) {
    if (node) node->streamTo(cs);
    return cs;
}

CompressStream_t &operator<<(CompressStream_t &cs,
                             const KJS::Identifier &value) {
    // dump identfier
    std::string id = cs.getIdFor(value.ustring().ascii());
    cs.os << id;

    // add origin identfier
    if (cs.comment)
        cs.os << "/*" << value.ustring().ascii() << "*/";

    return cs;
}

CompressStream_t &operator<<(CompressStream_t &cs,
                             const std::string &value) {
    cs.os << value;
    return cs;
}

CompressStream_t &operator<<(CompressStream_t &cs,
                             const char *value) {
    cs.os << value;
    return cs;
}

CompressStream_t &operator<<(CompressStream_t &cs,
                             const char &value) {
    cs.os << value;
    return cs;
}

CompressStream_t &operator<<(CompressStream_t &cs,
                             CompressStream_t::Format_t value) {
    if (cs.endl && (value == CompressStream_t::ENDL))
        cs.os << std::endl;
    return cs;
}

/**
 * @short dump node tree to stream and return stream.
 * @param node node tree to dump.
 * @param obfuscate do obfuscate.
 * @param comment write origin Identifier in comment.
 * @param ask ask user whether obfuscate identifier.
 * @param prefix dont obfuscate identfiers with prefix
 * @param blacklistDump dump blacklist to this file.
 * @param endl  write endl to buffer.
 */
CompressStream_t::CompressStream_t(const Node *node, bool obfuscate,
        bool comment, bool ask, const std::string &prefix,
        const std::string &userBlacklist, const std::string &blacklistDump,
        bool endl)
    : endl(endl), obfuscate(obfuscate), comment(comment), ask(ask),
      prefix(prefix), blacklistDump(blacklistDump), lastId(0)
{
    // read system blacklist
    std::copy(SYSTEM_BLACKLIST,
              SYSTEM_BLACKLIST + sizeof(SYSTEM_BLACKLIST) / sizeof(char *) - 1,
              std::inserter(blacklist, blacklist.begin()));

    // read user blacklist
    if (!userBlacklist.empty()) {
        std::ifstream ubl(userBlacklist.c_str());
        if (ubl) {
            std::copy(std::istream_iterator<std::string>(ubl),
                      std::istream_iterator<std::string>(),
                      std::inserter(blacklist, blacklist.begin()));
        } else {
            std::cerr << "Cannot read blacklist." << std::endl;
            std::cerr << strerror(errno) << std::endl;
        }
    }

    // dump node tree
    if (node) node->streamTo(*this);
}

/**
 * @short dump obfuscate log and destroy object.
 */
CompressStream_t::~CompressStream_t() {
    // dump obfuscate log
    if (!idmap.empty()) {
        std::ofstream fo("kjscompress.log");
        if (fo) {
            fo << idmap;
            fo.close();
        } else {
            std::cerr << "Cannot write obfuscate log." << std::endl;
            std::cerr << strerror(errno) << std::endl;
        }
    }

    // dump blacklist
    if (!blacklistDump.empty()) {
        std::ofstream fb(blacklistDump.c_str());
        if (fb) {
            fb << blacklist;
            fb.close();
        } else {
            std::cerr << "Cannot write blacklist." << std::endl;
            std::cerr << strerror(errno) << std::endl;
        }
    }
}

/**
 * @short return id for Identifier.
 * @param identifier which identifier
 * @return id for Identifier.
 */
std::string CompressStream_t::getIdFor(const std::string &id) {
    // is obfuscator allowed?
    if (!obfuscate)
        return id;

    // is identifier blacklisted by prefix?
    if (!prefix.empty() && (id.compare(0, prefix.size(), prefix) == 0))
        return id;

    // is id on blacklist?
    CompressStream_t::StringSet_t::iterator ibl = blacklist.find(id);
    if (ibl != blacklist.end())
        return id;

    // find old obfuscated identifiers
    CompressStream_t::IdentifierMap_t::iterator iid = idmap.find(id);
    if (iid != idmap.end())
        return iid->second;

    // ask user?
    if (ask) {
        // dump some bit of code
        std::string code = os.str();
        std::cerr << "--------------------------------------" << std::endl
            << code.substr((((signed)code.size() - 30) > 0)?
                    (signed)code.size() - 30: 0) << id
            << std::endl << "--------------------------------------" << std::endl
            << "Obfuscate `" << id << "' identfier? (y/n/a)" << std::endl;

        // ask user
        char ch;
        std::cin >> ch;
        if (((ch != 'Y') && (ch != 'y')) && ((ch != 'a') && (ch != 'A'))) {
            blacklist.insert(id);
            return id;
        }
        if ((ch == 'a') || (ch == 'A'))
            ask = false;
    }

    // generate new
    std::string nid = "__" + lastId.str();
    ++lastId;

    // save them
    idmap.insert(std::make_pair(id, nid));

    // return it
    return nid;
}

/*
 * Node dump rules
 */
void NullNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "NullNode" << std::endl;
#endif
    os << "null";
}

void BooleanNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "BooleanNode" << std::endl;
#endif
    os << (val ? "true" : "false");
}

void NumberNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "NumberNode" << std::endl;
#endif
    os << UString::from(val).ascii();
}

void StringNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "StringNode" << std::endl;
#endif
    os << '"' << escape(val.ascii()) << '"';
}

void RegExpNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "RegExpNode" << std::endl;
#endif
    os << "/" << pattern.ascii();
    os << "/" << flags.ascii();
}

void ThisNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ThisNode" << std::endl;
#endif
    os << "this";
}

void ResolveNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ResolveNode" << std::endl;
#endif
    os << ident;
}

void GroupNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "GroupNode" << std::endl;
#endif
    os << "(" << group << ")";
}

void ElementNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ElementNode" << std::endl;
#endif
    for (const ElementNode *n = this; n; n = n->list) {
        for (int i = 0; i < n->elision; i++)
            os << ",";
        os << n->node;
        if (n->list)
            os << ",";
    }
}

void ArrayNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ArrayNode" << std::endl;
#endif
    os << "[" << element;
    for (int i = 0; i < elision; i++)
        os << ",";
    os << "]";
}

void ObjectLiteralNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ObjectLiteralNode" << std::endl;
#endif
    if (list)
        os << "{" << list << "}";
    else
        os << "{}";
}

void PropertyValueNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "PropertyValueNode" << std::endl;
#endif
    int count = 0;
    for (const PropertyValueNode *n = this; n; n = n->list)
        os << ((count++)? ",": "")
            << "\"" << n->name << "\"" << ":" << n->assign;
}

void PropertyNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "PropertyNode" << std::endl;
#endif
    if (str.isNull())
        os << UString::from(numeric).ascii();
    else
        os << escape(str.ascii());
}

void AccessorNode1::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "AccessorNode1" << std::endl;
#endif
    os << expr1 << "[" << expr2 << "]";
}

void AccessorNode2::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "AccessorNode2" << std::endl;
#endif
    os << expr << "." << ident;
}

void ArgumentListNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ArgumentListNode" << std::endl;
#endif
    os << expr;
    for (ArgumentListNode *n = list; n; n = n->list)
        os << "," << n->expr;
}

void ArgumentsNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ArgumentsNode" << std::endl;
#endif
    os << "(" << list << ")";
}

void NewExprNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "NewExprNode" << std::endl;
#endif
    os << "new " << expr << args;
}

void FunctionCallNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "FunctionCallNode" << std::endl;
#endif
    os << expr << args;
}

void PostfixNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "PostfixNode" << std::endl;
#endif
    os << expr;
    if (oper == OpPlusPlus)
        os << "++";
    else
        os << "--";
}

void DeleteNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "DeleteNode" << std::endl;
#endif
    os << "delete " << expr;
}

void VoidNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "VoidNode" << std::endl;
#endif
    os << "void " << expr;
}

void TypeOfNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "TypeOfNode" << std::endl;
#endif
    os << "typeof " << expr;
}

void PrefixNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "PrefixNode" << std::endl;
#endif
    os << (oper == OpPlusPlus ? "++" : "--") << expr;
}

void UnaryPlusNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "UnaryPlusNode" << std::endl;
#endif
    os << "+" << expr;
}

void NegateNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "NegateNode" << std::endl;
#endif
    os << "-" << expr;
}

void BitwiseNotNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "BitwiseNotNode" << std::endl;
#endif
    os << "~" << expr;
}

void LogicalNotNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "LogicalNotNode" << std::endl;
#endif
    os << "!" << expr;
}

void MultNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "MultNode" << std::endl;
#endif
    os << term1;
    if (typeid(*term1) == typeid(KJS::PostfixNode))
        os << " ";
    os << oper;
    if (typeid(*term2) == typeid(KJS::PrefixNode))
        os << " ";
    os << term2;
}

void AddNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "AddNode" << std::endl;
#endif
    os << term1;
    if (typeid(*term1) == typeid(KJS::PostfixNode))
        os << " ";
    os << oper;
    if (typeid(*term2) == typeid(KJS::PrefixNode))
        os << " ";
    os << term2;
}

void AppendStringNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "AppendStringNode" << std::endl;
#endif
    os << term << "+" << '"' << escape(str.ascii()) << '"';
}

void ShiftNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ShiftNode" << std::endl;
#endif
    os << term1;
    if (oper == OpLShift)
        os << "<<";
    else if (oper == OpRShift)
        os << ">>";
    else
        os << ">>>";
    os << term2;
}

void RelationalNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "RelationalNode" << std::endl;
#endif
    os << expr1;
    switch (oper) {
    case OpLess:
        os << "<";
        break;
    case OpGreater:
        os << ">";
        break;
    case OpLessEq:
        os << "<=";
        break;
    case OpGreaterEq:
        os << ">=";
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

void EqualNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "EqualNode" << std::endl;
#endif
    os << expr1;
    switch (oper) {
    case OpEqEq:
        os << "==";
        break;
    case OpNotEq:
        os << "!=";
        break;
    case OpStrEq:
        os << "===";
        break;
    case OpStrNEq:
        os << "!==";
        break;
    default:
        ;
    }
    os << expr2;
}

void BitOperNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "BitOperNode" << std::endl;
#endif
    os << expr1;
    if (oper == OpBitAnd)
        os << "&";
    else if (oper == OpBitXOr)
        os << "^";
    else
        os << "|";
    os << expr2;
}

void BinaryLogicalNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "BinaryLogicalNode" << std::endl;
#endif
    os << expr1 << (oper == OpAnd ? "&&" : "||") << expr2;
}

void ConditionalNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ConditionalNode" << std::endl;
#endif
    os << logical << "?" << expr1 << ":" << expr2;
}

void AssignNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "AssignNode" << std::endl;
#endif
    os << left;
    const char *opStr;
    switch (oper) {
    case OpEqual:
        opStr = "=";
        break;
    case OpMultEq:
        opStr = "*=";
        break;
    case OpDivEq:
        opStr = "/=";
        break;
    case OpPlusEq:
        opStr = "+=";
        break;
    case OpMinusEq:
        opStr = "-=";
        break;
    case OpLShift:
        opStr = "<<=";
        break;
    case OpRShift:
        opStr = ">>=";
        break;
    case OpURShift:
        opStr = ">>=";
        break;
    case OpAndEq:
        opStr = "&=";
        break;
    case OpXOrEq:
        opStr = "^=";
        break;
    case OpOrEq:
        opStr = "|=";
        break;
    case OpModEq:
        opStr = "%=";
        break;
    default:
        opStr = "?=";
    }
    os << opStr << expr;
}

void CommaNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "CommaNode" << std::endl;
#endif
    os << expr1 << "," << expr2;
}

void StatListNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "StatListNode" << std::endl;
#endif
    for (const StatListNode *n = this; n; n = n->list)
        os << n->statement;
}

void AssignExprNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "AssignExprNode" << std::endl;
#endif
    os << "=" << expr;
}

void VarDeclNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "VarDeclNode" << std::endl;
#endif
    os << ident << init;
}

void VarDeclListNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "VarDeclListNode" << std::endl;
#endif
    os << var;
    for (VarDeclListNode *n = list; n; n = n->list)
        os << "," << n->var;
}

void VarStatementNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "VarStatementNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "var " << list << ";";
}

void BlockNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "BlockNode" << std::endl;
#endif
      os << CompressStream_t::ENDL << "{" << source
          << CompressStream_t::ENDL << "}";
}

void EmptyStatementNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "EmptyStatementNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << ";";
}

void ExprStatementNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ExprStatementNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << expr << ";";
}

void IfNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "IfNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "if(" << expr << ")" << statement1;
    if (statement2) {
        os << CompressStream_t::ENDL << "else";
        if (!dynamic_cast<BlockNode *>(statement2))
            os << "{" << statement2 << "}";
        else
            os << statement2;
    }
}

void DoWhileNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "DoWhileNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "do";
    if (!dynamic_cast<BlockNode *>(statement))
        os << "{" << statement << "}";
    else
        os << statement;
    os << CompressStream_t::ENDL << "while(" << expr << ");";
}

void WhileNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "WhileNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "while(" << expr << ")" << statement;
}

void ForNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ForNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "for(" << ((var)?"var ": "")
        << expr1 << ";" << expr2 << ";" << expr3 << ")" << statement;
}

void ForInNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ForInNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "for(";
    if (varDecl)
        os << "var " << varDecl;
    else
        os << lexpr;
    if (init)
        os << "=" << init;
    os << " in " << expr << ")" << statement;
}

void ContinueNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ContinueNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "continue";
    if (!ident.isNull())
        os << " " << ident;
    os << ";";
}

void BreakNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "BreakNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "break";
    if (!ident.isNull())
        os << " " << ident;
    os << ";";
}

void ReturnNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ReturnNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "return";
    if (value)
        os << " " << value;
    os << ";";
}

void WithNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "WithNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "with(" << expr << ")" << statement;
}

void CaseClauseNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "CaseClauseNode" << std::endl;
#endif
    os << CompressStream_t::ENDL;
    if (expr)
        os << "case " << expr;
    else
        os << "default";
    os << ":";
    if (list)
        os << list;
}

void ClauseListNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ClauseListNode" << std::endl;
#endif
    for (const ClauseListNode *n = this; n; n = n->next())
        os << n->clause();
}

void CaseBlockNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "CaseBlockNode" << std::endl;
#endif
    for (const ClauseListNode *n = list1; n; n = n->next())
        os << n->clause();
    if (def)
        os << def;
    for (const ClauseListNode *n = list2; n; n = n->next())
        os << n->clause();
}

void SwitchNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "SwitchNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "switch(" << expr << "){" << block
        << CompressStream_t::ENDL << "}";
}

void LabelNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "LabelNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << label << ":" << statement;
}

void ThrowNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ThrowNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "throw " << expr << ";";
}

void CatchNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "CatchNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "catch(" << ident << ")" << block;
}

void FinallyNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "FinallyNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "finally" << block;
}

void TryNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "TryNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "try" << block << _catch << _final;
}

void ParameterNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "ParameterNode" << std::endl;
#endif
    os << id;
    for (ParameterNode *n = next; n; n = n->next)
        os << "," << n->id;
}

void FuncDeclNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "FuncDeclNode" << std::endl;
#endif
    os << CompressStream_t::ENDL << "function " << ident << "(";
    if (param)
        os << param;
    os << ")" << body;
}

void FuncExprNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "FuncExprNode" << std::endl;
#endif
    os << "function" << "(" << param << ")" << body;
}

void SourceElementsNode::streamTo(CompressStream_t &os) const {
#ifdef DEBUG
    std::cerr << "SourceElementsNode" << std::endl;
#endif
    for (const SourceElementsNode *n = this; n; n = n->elements)
        os << n->element;
}

