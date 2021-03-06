/*
 * Copyright (C) 2012 Andres Pagliano, Gabriel Miretti, Gonzalo Buteler,
 * Nestor Bustamante, Pablo Perez de Angelis
 *
 * This file is part of LVK Chatbot.
 *
 * LVK Chatbot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LVK Chatbot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LVK Chatbot.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "da-clue/clueengine.h"
#include "da-clue/analyzedscript.h"
#include "da-clue/script.h"
#include "nlp-engine/enginefactory.h"
#include "nlp-engine/nlpproperties.h"

#include <QtDebug>

//--------------------------------------------------------------------------------------------------
// ClueEngine
//--------------------------------------------------------------------------------------------------

Lvk::Clue::ClueEngine::ClueEngine()
    : m_engine(Nlp::EngineFactory().createEngine())
{
}

//--------------------------------------------------------------------------------------------------

Lvk::Clue::ClueEngine::~ClueEngine()
{
    delete m_engine;
}

//--------------------------------------------------------------------------------------------------

void Lvk::Clue::ClueEngine::setRules(const Nlp::RuleList &rules)
{
    m_engine->setRules(rules);
}

//--------------------------------------------------------------------------------------------------

void Lvk::Clue::ClueEngine::setEvasive(const QString &evasive)
{
    m_evasive = evasive;
}

//--------------------------------------------------------------------------------------------------

void Lvk::Clue::ClueEngine::setCategoriesEnabled(bool enabled)
{
    m_engine->setProperty(NLP_PROP_PREFER_CUR_TOPIC, enabled);
}

//--------------------------------------------------------------------------------------------------

void Lvk::Clue::ClueEngine::clear()
{
    m_engine->clear();
}

//--------------------------------------------------------------------------------------------------

void Lvk::Clue::ClueEngine::analyze(const Clue::ScriptList &scripts,
                                    Clue::AnalyzedList &ascripts)
{
    Clue::AnalyzedScript as;

    foreach (const Clue::Script &s, scripts) {
        analyze(s, as);
        ascripts.append(as);
    }
}

//--------------------------------------------------------------------------------------------------

void Lvk::Clue::ClueEngine::analyze(const Clue::Script &script, Clue::AnalyzedScript &ascript)
{
    ascript.clear();
    ascript.filename = script.filename;
    ascript.character = script.character;
    ascript.number = script.number;

    int matches = 0;
    Nlp::Engine::MatchList ml;

    // FIXME not setting real score and outputIdx

    qDebug() << "ClueEngine: Analyzing script:"  << script.filename;

    foreach (const Clue::ScriptLine &line, script) {
        qDebug() << "ClueEngine: Getting response for question:"  << line.question;

        QString resp = m_engine->getResponse(line.question, ml);
        QString topic = m_engine->getCurrentTopic("");

        if (ml.isEmpty()) {
            qDebug() << "ClueEngine: no response!";

            Clue::AnalyzedLine aline(line);

            aline.status = Clue::NoAnswerFound;
            aline.answer = m_evasive;
            aline.topic = topic;

            ascript.append(aline);
        } else {
            qDebug() << "ClueEngine: Checking response:" << resp
                     << "with expected pattern:" << line.expAnswer
                     << "and forbidden pattern:" << line.forbidAnswer;

            Clue::AnalyzedLine aline(line, ml[0].first, ml[0].second, 0, resp);

            aline.topic = topic;

            if (!m_regexp.exactMatch(line.expAnswer, resp)) {
                qDebug() << "ClueEngine: Mismatch expected answer!";
                aline.status = Clue::MismatchExpectedAnswer;
            } else if (m_regexp.exactMatch(line.forbidAnswer, resp)) {
                qDebug() << "ClueEngine: Match forbidden answer!";
                aline.status = Clue::MatchForbiddenAnswer;
            } else {
                qDebug() << "ClueEngine: Answer OK";
                aline.status = Clue::AnswerOk;
                aline.outputIdx = 0;
                ++matches;
            }

            ascript.append(aline);
        }
    }

    if (script.size() > 0) {
        ascript.coverage = matches/(float)script.size()*100;
    } else {
        ascript.coverage = 100; //?
    }
}


