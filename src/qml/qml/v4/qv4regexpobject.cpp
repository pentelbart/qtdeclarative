/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4regexpobject_p.h"
#include "qv4jsir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto_p.h"
#include "qv4stringobject_p.h"
#include "qv4mm_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>
#include "private/qlocale_tools_p.h"

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <QtCore/qregexp.h>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include "qv4alloca_p.h"

Q_CORE_EXPORT QString qt_regexp_toCanonical(const QString &, QRegExp::PatternSyntax);

using namespace QV4;

DEFINE_MANAGED_VTABLE(RegExpObject);

RegExpObject::RegExpObject(ExecutionEngine *engine, RegExp* value, bool global)
    : Object(engine)
    , value(value)
    , global(global)
{
    vtbl = &static_vtbl;
    type = Type_RegExpObject;

    Property *lastIndexProperty = insertMember(engine->newIdentifier(QStringLiteral("lastIndex")),
                                                         Attr_NotEnumerable|Attr_NotConfigurable);
    lastIndexProperty->value = Value::fromInt32(0);
    if (!this->value)
        return;

    QString p = this->value->pattern();
    if (p.isEmpty()) {
        p = QStringLiteral("(?:)");
    } else {
        // escape certain parts, see ch. 15.10.4
        p.replace('/', QLatin1String("\\/"));
    }

    defineReadonlyProperty(engine->newIdentifier(QStringLiteral("source")), Value::fromString(engine->newString(p)));
    defineReadonlyProperty(engine->newIdentifier(QStringLiteral("global")), Value::fromBoolean(global));
    defineReadonlyProperty(engine->newIdentifier(QStringLiteral("ignoreCase")), Value::fromBoolean(this->value->ignoreCase()));
    defineReadonlyProperty(engine->newIdentifier(QStringLiteral("multiline")), Value::fromBoolean(this->value->multiLine()));
}

// Converts a QRegExp to a JS RegExp.
// The conversion is not 100% exact since ECMA regexp and QRegExp
// have different semantics/flags, but we try to do our best.
RegExpObject::RegExpObject(ExecutionEngine *engine, const QRegExp &re)
    : Object(engine)
    , value(0)
    , global(false)
{
    vtbl = &static_vtbl;
    type = Type_RegExpObject;

    // Convert the pattern to a ECMAScript pattern.
    QString pattern = qt_regexp_toCanonical(re.pattern(), re.patternSyntax());
    if (re.isMinimal()) {
        QString ecmaPattern;
        int len = pattern.length();
        ecmaPattern.reserve(len);
        int i = 0;
        const QChar *wc = pattern.unicode();
        bool inBracket = false;
        while (i < len) {
            QChar c = wc[i++];
            ecmaPattern += c;
            switch (c.unicode()) {
            case '?':
            case '+':
            case '*':
            case '}':
                if (!inBracket)
                    ecmaPattern += QLatin1Char('?');
                break;
            case '\\':
                if (i < len)
                    ecmaPattern += wc[i++];
                break;
            case '[':
                inBracket = true;
                break;
            case ']':
                inBracket = false;
                break;
            default:
                break;
            }
        }
        pattern = ecmaPattern;
    }

    value = RegExp::create(engine, pattern, re.caseSensitivity() == Qt::CaseInsensitive, false);
}

void RegExpObject::destroy(Managed *that)
{
    static_cast<RegExpObject *>(that)->~RegExpObject();
}

void RegExpObject::markObjects(Managed *that)
{
    RegExpObject *re = static_cast<RegExpObject*>(that);
    if (re->value)
        re->value->mark();
    Object::markObjects(that);
}

Property *RegExpObject::lastIndexProperty(ExecutionContext *ctx)
{
    assert(0 == internalClass->find(ctx->engine->newIdentifier(QStringLiteral("lastIndex"))));
    return &memberData[0];
}

// Converts a JS RegExp to a QRegExp.
// The conversion is not 100% exact since ECMA regexp and QRegExp
// have different semantics/flags, but we try to do our best.
QRegExp RegExpObject::toQRegExp() const
{
    Qt::CaseSensitivity caseSensitivity = value->ignoreCase() ? Qt::CaseInsensitive : Qt::CaseSensitive;
    return QRegExp(value->pattern(), caseSensitivity, QRegExp::RegExp2);
}

Value RegExpPrototype::ctor_method_construct(Managed *, ExecutionContext *ctx, Value *argv, int argc)
{
    Value r = argc > 0 ? argv[0] : Value::undefinedValue();
    Value f = argc > 1 ? argv[1] : Value::undefinedValue();
    if (RegExpObject *re = r.asRegExpObject()) {
        if (!f.isUndefined())
            ctx->throwTypeError();

        RegExpObject *o = ctx->engine->newRegExpObject(re->value, re->global);
        return Value::fromObject(o);
    }

    QString pattern;
    if (!r.isUndefined())
        pattern = r.toString(ctx)->toQString();

    bool global = false;
    bool ignoreCase = false;
    bool multiLine = false;
    if (!f.isUndefined()) {
        f = __qmljs_to_string(f, ctx);
        QString str = f.stringValue()->toQString();
        for (int i = 0; i < str.length(); ++i) {
            if (str.at(i) == QChar('g') && !global) {
                global = true;
            } else if (str.at(i) == QChar('i') && !ignoreCase) {
                ignoreCase = true;
            } else if (str.at(i) == QChar('m') && !multiLine) {
                multiLine = true;
            } else {
                ctx->throwSyntaxError(0);
            }
        }
    }

    RegExp* re = RegExp::create(ctx->engine, pattern, ignoreCase, multiLine);
    if (!re->isValid())
        ctx->throwSyntaxError(0);

    RegExpObject *o = ctx->engine->newRegExpObject(re, global);
    return Value::fromObject(o);
}

Value RegExpPrototype::ctor_method_call(Managed *that, ExecutionContext *ctx, const Value &thisObject, Value *argv, int argc)
{
    if (argc > 0 && argv[0].asRegExpObject()) {
        if (argc == 1 || argv[1].isUndefined())
            return argv[0];
    }

    return ctor_method_construct(that, ctx, argv, argc);
}

Value RegExpPrototype::method_exec(SimpleCallContext *ctx)
{
    RegExpObject *r = ctx->thisObject.asRegExpObject();
    if (!r)
        ctx->throwTypeError();

    Value arg = ctx->argument(0);
    arg = __qmljs_to_string(arg, ctx);
    QString s = arg.stringValue()->toQString();

    int offset = r->global ? r->lastIndexProperty(ctx)->value.toInt32() : 0;
    if (offset < 0 || offset > s.length()) {
        r->lastIndexProperty(ctx)->value = Value::fromInt32(0);
        return Value::nullValue();
    }

    uint* matchOffsets = (uint*)alloca(r->value->captureCount() * 2 * sizeof(uint));
    int result = r->value->match(s, offset, matchOffsets);
    if (result == -1) {
        r->lastIndexProperty(ctx)->value = Value::fromInt32(0);
        return Value::nullValue();
    }

    // fill in result data
    ArrayObject *array = ctx->engine->newArrayObject();
    for (int i = 0; i < r->value->captureCount(); ++i) {
        int start = matchOffsets[i * 2];
        int end = matchOffsets[i * 2 + 1];
        Value entry = Value::undefinedValue();
        if (start != -1 && end != -1)
            entry = Value::fromString(ctx, s.mid(start, end - start));
        array->push_back(entry);
    }

    array->put(ctx, QLatin1String("index"), Value::fromInt32(result));
    array->put(ctx, QLatin1String("input"), arg);

    if (r->global)
        r->lastIndexProperty(ctx)->value = Value::fromInt32(matchOffsets[1]);

    return Value::fromObject(array);
}

Value RegExpPrototype::method_test(SimpleCallContext *ctx)
{
    Value r = method_exec(ctx);
    return Value::fromBoolean(!r.isNull());
}

Value RegExpPrototype::method_toString(SimpleCallContext *ctx)
{
    RegExpObject *r = ctx->thisObject.asRegExpObject();
    if (!r)
        ctx->throwTypeError();

    QString result = QChar('/') + r->get(ctx, ctx->engine->newIdentifier(QStringLiteral("source"))).stringValue()->toQString();
    result += QChar('/');
    if (r->global)
        result += QChar('g');
    if (r->value->ignoreCase())
        result += QChar('i');
    if (r->value->multiLine())
        result += QChar('m');
    return Value::fromString(ctx, result);
}

Value RegExpPrototype::method_compile(SimpleCallContext *ctx)
{
    RegExpObject *r = ctx->thisObject.asRegExpObject();
    if (!r)
        ctx->throwTypeError();

    RegExpObject *re = ctx->engine->regExpCtor.asFunctionObject()->construct(ctx, ctx->arguments, ctx->argumentCount).asRegExpObject();

    r->value = re->value;
    r->global = re->global;
    return Value::undefinedValue();
}

#include "qv4regexpobject_p_jsclass.cpp"
