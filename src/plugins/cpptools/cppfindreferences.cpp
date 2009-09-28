/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "cppfindreferences.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"

#include <texteditor/basetexteditor.h>
#include <find/searchresultwindow.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/filesearch.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/icore.h>

#include <ASTVisitor.h>
#include <AST.h>
#include <Control.h>
#include <Literals.h>
#include <TranslationUnit.h>
#include <Symbols.h>
#include <Names.h>
#include <Scope.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/Overview.h>

#include <QtCore/QTime>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QDir>

#include <qtconcurrent/runextensions.h>

using namespace CppTools::Internal;
using namespace CPlusPlus;

namespace {

struct Process: protected ASTVisitor
{
public:
    Process(QFutureInterface<Core::Utils::FileSearchResult> &future,
            Document::Ptr doc, const Snapshot &snapshot)
            : ASTVisitor(doc->control()),
              _future(future),
              _doc(doc),
              _snapshot(snapshot),
              _source(_doc->source()),
              _sem(doc->control())
    {
        _snapshot.insert(_doc);
    }

    void operator()(Symbol *symbol, Identifier *id, AST *ast)
    {
        _declSymbol = symbol;
        _id = id;
        _exprDoc = Document::create("<references>");
        accept(ast);
    }

protected:
    using ASTVisitor::visit;

    QString matchingLine(const Token &tk) const
    {
        const char *beg = _source.constData();
        const char *cp = beg + tk.offset;
        for (; cp != beg - 1; --cp) {
            if (*cp == '\n')
                break;
        }

        ++cp;

        const char *lineEnd = cp + 1;
        for (; *lineEnd; ++lineEnd) {
            if (*lineEnd == '\n')
                break;
        }

        const QString matchingLine = QString::fromUtf8(cp, lineEnd - cp);
        return matchingLine;

    }

    void reportResult(unsigned tokenIndex)
    {
        const Token &tk = tokenAt(tokenIndex);
        const QString lineText = matchingLine(tk);

        unsigned line, col;
        getTokenStartPosition(tokenIndex, &line, &col);

        if (col)
            --col;  // adjust the column position.

        int len = tk.f.length;

        _future.reportResult(Core::Utils::FileSearchResult(QDir::toNativeSeparators(_doc->fileName()),
                                                           line, lineText, col, len));
    }

    bool checkCandidates(const QList<Symbol *> &candidates) const
    {
        // ### FIXME return isDeclSymbol(LookupContext::canonicalSymbol(candidates));
        return true;
    }

    bool isDeclSymbol(Symbol *symbol) const
    {
        if (! symbol)
            return false;

        else if (symbol == _declSymbol)
            return true;

        else if (symbol->line() == _declSymbol->line() && symbol->column() == _declSymbol->column()) {
            if (! qstrcmp(symbol->fileName(), _declSymbol->fileName()))
                return true;
        }

        return false;
    }

    LookupContext currentContext(AST *ast) const
    {
        unsigned line, column;
        getTokenStartPosition(ast->firstToken(), &line, &column);
        Symbol *lastVisibleSymbol = _doc->findSymbolAt(line, column);
        return LookupContext(lastVisibleSymbol, _exprDoc, _doc, _snapshot);
    }

    virtual bool visit(QualifiedNameAST *ast)
    {
        if (! ast->name) {
            //qWarning() << "invalid AST at" << _doc->fileName() << line << column;
            ast->name = _sem.check(ast, /*scope */ static_cast<Scope *>(0));
        }

        Q_ASSERT(ast->name != 0);
        Identifier *id = ast->name->identifier();
        if (id == _id && ast->unqualified_name) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(ast->name);
            if (checkCandidates(candidates))
                reportResult(ast->unqualified_name->firstToken());
        }

        return false;
    }

    virtual bool visit(SimpleNameAST *ast)
    {
        Identifier *id = identifier(ast->identifier_token);
        if (id == _id) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(ast->name);
            if (checkCandidates(candidates))
                reportResult(ast->identifier_token);
        }

        return false;
    }

    virtual bool visit(TemplateIdAST *ast)
    {
        Identifier *id = identifier(ast->identifier_token);
        if (id == _id) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(ast->name);
            if (checkCandidates(candidates))
                reportResult(ast->identifier_token);
        }

        return false;
    }

private:
    QFutureInterface<Core::Utils::FileSearchResult> &_future;
    Identifier *_id; // ### remove me
    Symbol *_declSymbol;
    Document::Ptr _doc;
    Snapshot _snapshot;
    QByteArray _source;
    Document::Ptr _exprDoc;
    Semantic _sem;
};

} // end of anonymous namespace

CppFindReferences::CppFindReferences(CppModelManager *modelManager)
    : _modelManager(modelManager),
      _resultWindow(ExtensionSystem::PluginManager::instance()->getObject<Find::SearchResultWindow>())
{
    m_watcher.setPendingResultsLimit(1);
    connect(&m_watcher, SIGNAL(resultReadyAt(int)), this, SLOT(displayResult(int)));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
}

CppFindReferences::~CppFindReferences()
{
}

static void find_helper(QFutureInterface<Core::Utils::FileSearchResult> &future,
                        Snapshot snapshot,
                        Symbol *symbol)
{
    QTime tm;
    tm.start();

    Identifier *symbolId = symbol->identifier();
    Q_ASSERT(symbolId != 0);

    const QString fileName = QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());

    QStringList files(fileName);
    files += snapshot.dependsOn(fileName);
    qDebug() << "done in:" << tm.elapsed() << "number of files to parse:" << files.size();

    future.setProgressRange(0, files.size());

    tm.start();
    for (int i = 0; i < files.size(); ++i) {
        const QString &fn = files.at(i);
        future.setProgressValueAndText(i, QFileInfo(fn).fileName());

        Document::Ptr previousDoc = snapshot.value(fn);
        if (previousDoc) {
            Control *control = previousDoc->control();
            Identifier *id = control->findIdentifier(symbolId->chars(), symbolId->size());
            if (! id)
                continue; // skip this document, it's not using symbolId.
        }

        QFile f(fn);
        if (! f.open(QFile::ReadOnly))
            continue;

        const QString source = QTextStream(&f).readAll(); // ### FIXME
        const QByteArray preprocessedCode = snapshot.preprocessedCode(source, fn);
        Document::Ptr doc = snapshot.documentFromSource(preprocessedCode, fn);
        doc->tokenize();

        Control *control = doc->control();
        if (Identifier *id = control->findIdentifier(symbolId->chars(), symbolId->size())) {
            doc->check();
            TranslationUnit *unit = doc->translationUnit();
            Process process(future, doc, snapshot);
            process(symbol, id, unit->ast());
        }
    }
    future.setProgressValue(files.size());
}

void CppFindReferences::findAll(const Snapshot &snapshot, Symbol *symbol)
{
    _resultWindow->clearContents();
    _resultWindow->popup(true);

    Core::ProgressManager *progressManager = Core::ICore::instance()->progressManager();

    QFuture<Core::Utils::FileSearchResult> result = QtConcurrent::run(&find_helper, snapshot, symbol);
    m_watcher.setFuture(result);

    Core::FutureProgress *progress = progressManager->addTask(result, tr("Searching..."),
                                                              CppTools::Constants::TASK_SEARCH,
                                                              Core::ProgressManager::CloseOnSuccess);

    connect(progress, SIGNAL(clicked()), _resultWindow, SLOT(popup()));
}

void CppFindReferences::displayResult(int index)
{
    Core::Utils::FileSearchResult result = m_watcher.future().resultAt(index);
    Find::ResultWindowItem *item = _resultWindow->addResult(result.fileName,
                                                            result.lineNumber,
                                                            result.matchingLine,
                                                            result.matchStart,
                                                            result.matchLength);
    if (item)
        connect(item, SIGNAL(activated(const QString&,int,int)),
                this, SLOT(openEditor(const QString&,int,int)));
}

void CppFindReferences::searchFinished()
{
    emit changed();
}

void CppFindReferences::openEditor(const QString &fileName, int line, int column)
{
    TextEditor::BaseTextEditor::openEditorAt(fileName, line, column);
}

