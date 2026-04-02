// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "efdPCH.h"

#include <efd/PathUtils.h>
#include <efd/ILogger.h>
#include <efd/File.h>
#include <efd/AssetUriReader.h>
#include <efd/AssetMetaGraph.h>
#if defined(EE_USE_NATIVE_STL)
    #include <algorithm>
#else
    #include <algorithm>
#endif
#include <efd/efdLogIDs.h>

using namespace efd;

//--------------------------------------------------------------------------------------------------
Graph::Graph (const efd::utf8string& predicate,
              efd::Bool isIndexed,
              const efd::utf8string& refValue)
: m_isLoaded (false)
, m_isIndexed (isIndexed)
, m_dirty (false)
, m_reference_property (predicate)
, m_reference_value (refValue)
{
}

Graph::Graph ()
: m_isLoaded (false)
, m_isIndexed (false)
, m_dirty (false)
{
}

bool Graph::load (const utf8string& filename)
{
    m_filename = PathUtils::PathMakeNative(filename);
    m_filename = PathUtils::PathRemoveDotDots(m_filename);

    File* pkFile = File::GetFile(m_filename.c_str(), File::READ_ONLY);
    NTripleFormat parser;
    char buf[513];

    if (pkFile)
    {
        unsigned int sz = pkFile->Read(buf, 512);
        while (sz)
        {
            buf[sz] = '\0';
            m_isLoaded = parser.parse (buf, this);
            if (!m_isLoaded) break;
            sz = pkFile->Read(buf, 512);
        }

        EE_DELETE pkFile;
    }
    else
    {
        m_isLoaded = true;
    }

    m_dirty = false;
    return m_isLoaded;
}

bool Graph::isLoaded()
{
    return m_isLoaded;
}

GraphSaveResult Graph::save(bool always_overwrite)
{
    GraphSaveResult retval = GRAPH_SAVE_UNCHANGED;

    if (m_dirty || always_overwrite)
    {
        if (m_store.size()>0)
        {
            NTripleFormat nt;
            if (nt.serialize (m_filename, this))
            {
                m_dirty = false;
                retval = GRAPH_SAVE_SUCCESS;
            }
            else
            {
                retval = GRAPH_SAVE_FAIL;
            }
        }
        else
        {
            purge (true);
            retval = GRAPH_SAVE_SUCCESS;
        }
    }

    return retval;
}

void Graph::purge(efd::Bool removePersistent)
{
    m_store.clear();
    m_dirty = false;
    m_isLoaded = false;

    if (m_isIndexed)
    {
        m_index.clear();
    }

    if (removePersistent)
    {
#ifdef EE_PLATFORM_WIN32
        ::DeleteFile (m_filename.c_str());
#endif
    }
}

void Graph::query (const Triple& q,
                   TripleSet& matches)
{
    Graph::Iterator i(this), end;

    if (q.subject_desc!=NodeDescNone)
    {
        if (i.find (q.subject))
        {
            // check matching subject triple in Graph
            if (compareTriple (*i, q))
                matches.push_back (*i);
        }
    }
    else if (q.predicate == m_reference_property)
    {
        // check matching object values in graph
        if (m_isIndexed)
        {
            index_lookup(q.object, matches);
        }
        else
        {
            for (i.begin(); i!=end; ++i)
            {
                if (compareTriple (*i, q))
                    matches.push_back (*i);
            }
        }
    }
}

void Graph::queryWithCompare (const Triple& q,
                              AssetQueryCompareFunc comp,
                              TripleSet& matches)
{
    Graph::Iterator i(this), end;
    for (i.begin(); i!=end; ++i)
    {
        if (comp (*i, q))
            matches.push_back (*i);
    }
}

void Graph::insert_triple (const Triple& triple)
{
    // The predicate of the inserted must match the predicate value
    // that was set by this Graph's constructor.
    EE_ASSERT (triple.predicate == m_reference_property);

    if (m_reference_value.length())
    {
        m_store[triple.subject] = m_reference_value;
    }
    else
    {
        m_store[triple.subject] = triple.object;
    }
    m_dirty = true;

    // Update the index if there is one for this Graph
    if (m_isIndexed)
    {
        set_index (triple);
    }
}

void Graph::merge_triple (const Triple& triple)
{
    insert_triple (triple);
}

bool Graph::append (const efd::utf8string& subject, const efd::utf8string& value)
{
    Triple t;
    t.subject = subject;
    t.predicate = m_reference_property;
    t.object = value;

    merge_triple(t);
    return true;
}

void Graph::deleteAllSubject (const efd::utf8string& subject)
{
    m_store.erase (subject);
    if (m_isIndexed)
    {
        remove_index (subject);
    }
    m_dirty = true;
}

bool Graph::compareTriple (const Triple& a, const Triple& b)
{
    bool sMatch = false
        , pMatch = false
        , oMatch = false;

    // subjects match?
    if ((a.subject_desc==NodeDescNone ||
          b.subject_desc==NodeDescNone)
         ||
         (a.subject==b.subject &&
          a.subject_desc==b.subject_desc))
    {
        sMatch = true;
    }

    // predicates match?
    if ((a.predicate_desc==NodeDescNone ||
          b.predicate_desc==NodeDescNone)
        ||
        (a.predicate==b.predicate &&
         a.predicate_desc==b.predicate_desc))
    {
        pMatch = true;
    }

    // objects match?
    if ((a.object_desc==NodeDescNone ||
          b.object_desc==NodeDescNone)
        ||
        (a.object==b.object &&
         a.object_desc==b.object_desc))
    {
        oMatch = true;
    }

    return sMatch && pMatch && oMatch;
}

void Graph::intersection (TripleSet& set, const TripleSet& other)
{
    TripleSetIterator i;
    TripleSet solution;

    // create a map of just the subjects from set
    // this makes the searching to come more efficient
    map<utf8string, int> temp;
    for (i=set.begin(); i!=set.end(); i++)
    {
        temp[i->subject] = 0;
    }

    // intersect triples from 'other' based only on subject value
    for (i=other.begin(); i!=other.end(); i++)
    {
        if (temp.find (i->subject) != temp.end())
        {
            // triple is in 'other' and in 'set'
            solution.push_back (*i);
        }
    }

    // copy solution over original 'set'
    set.clear();
    set = solution;
}

void Graph::intersection (TripleSet& set)
{
    TripleSetIterator i=set.begin();
    TripleSet solution;

    // intersect triples from 'set' based only on subject value
    for (; i!=set.end(); ++i)
    {
        if (m_store.find (i->subject) != m_store.end())
        {
            // triple is in Graph and in 'set'
            solution.push_back (*i);
        }
    }

    // copy solution over original 'set'
    set.clear();
    set = solution;
}

void Graph::unionize (TripleSet& set, const TripleSet& other)
{
    TripleSetIterator i;

    for (i=other.begin(); i!=other.end(); i++)
    {
        set.push_back (*i);
    }
}

void Graph::unionize (TripleSet& set)
{
    Graph::Iterator i(this), end;

    for (i.begin(); i!=end; ++i)
    {
        set.push_back (*i);
    }
}

void Graph::unique_values (TripleSet& set, const TripleSet& other)
{
    // don't intersect with an empty set
    if (set.size()==0 || other.size()==0)
    {
        return;
    }

    TripleSetIterator i;
    TripleSet solution;

    // create a map of just the object values from 'set'
    // this makes the searching to come more efficient
    map<utf8string, int> temp;
    for (i=set.begin(); i!=set.end(); i++)
    {
        if (temp.find (i->object) == temp.end())
        {
            temp[i->object] = 0;
            solution.push_back (*i);
        }
    }

    // include the triples from 'other' based on their object values
    // not already being included in 'set'
    for (i=other.begin(); i!=other.end(); i++)
    {
        if (temp.find (i->object) == temp.end())
        {
            temp[i->object] = 0;
            solution.push_back (*i);
        }
    }

    // copy solution over original 'set'
    set.clear();
    set = solution;
}

void Graph::set_index(const Triple& triple)
{
    if (triple.predicate==m_reference_property)
    {
        m_index[triple.object].push_back (triple.subject);
    }
}

void Graph::remove_index (const efd::utf8string& key)
{
    IndexContainer::const_iterator i=m_index.begin();
    for (; i!=m_index.end(); ++i)
    {
        const vector<utf8string>& old = i->second;
        vector<utf8string> replace;
        vector<utf8string>::const_iterator j = old.begin();
        for (; j!=old.end(); ++j)
        {
            if (key != *j)
            {
                replace.push_back (*j);
            }
        }
        m_index[i->first] = replace;
    }
}

bool Graph::index_lookup(const efd::utf8string& value, TripleSet& triples)
{
    IndexContainer::const_iterator i;
    i = m_index.find (value);
    if (i != m_index.end())
    {
        vector<utf8string>::const_iterator j=i->second.begin();
        for (; j != i->second.end(); ++j)
        {
            Triple t;
            t.subject = *j;
            t.predicate = m_reference_property;
            t.object = value;
            triples.push_back (t);
        }
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
NTripleFormat::NTripleFormat()
: m_graph (NULL)
, m_simple_graph (NULL)
, parserState (sNTriples)
, tidx (0)
{
}

bool NTripleFormat::parse (const char* stream, Graph* graph)
{
    m_graph = graph;
    return parse (stream);
}

bool NTripleFormat::parse (const char* stream, SimpleGraph* graph)
{
    m_simple_graph = graph;
    return parse (stream);
}

void NTripleFormat::insert_node (const Triple& triple)
{
    if (m_graph)
    {
        m_graph->insert_triple(triple);
    }
    else
    {
        m_simple_graph->insert_triple(triple);
    }
}

bool NTripleFormat::parse (const char* stream)
{
    char lookAhead;

    while (*stream)
    {
        lookAhead = *stream;

        switch (parserState)
        {
            case sNTriples:
                if (lookAhead == '<')
                {
                    parserState = sSubject;
                    add.subject.clear();
                    token[0] = 0;
                    tidx = 0;
                    add.subject_desc = NodeDescUri;
                }
                break;

            case sSubject:
                if (lookAhead != '\n')
                {
                    if (lookAhead != '>')
                    {
                        token[tidx++] = lookAhead;
                    }
                    else
                    {
                        token[tidx++] = 0;
                        add.subject = token;
                        parserState = sPWS;
                    }
                }
                else
                {
                    // parse error - newline in middle of subject
                    return false;
                }
                break;

            case sPWS:
                if (lookAhead != '\t' && lookAhead != ' ')
                {
                    if (lookAhead == '<')
                    {
                        parserState = sPredicate;
                        add.predicate.clear();
                        token[0] = 0;
                        tidx = 0;
                        add.predicate_desc = NodeDescUri;
                    }
                    else
                    {
                        // parse error
                        return false;
                    }
                }
                break;

            case sPredicate:
                if (lookAhead != '\n')
                {
                    if (lookAhead != '>')
                    {
                        token[tidx++] = lookAhead;
                    }
                    else
                    {
                        token[tidx++] = 0;
                        add.predicate = token;
                        parserState = sOWS;
                    }
                }
                else
                {
                    // parse error - newline in middle of predicate
                    return false;
                }
                break;

            case sOWS:
                if (lookAhead != '\t' && lookAhead != ' ')
                {
                    if (lookAhead == '"')
                    {
                        parserState = sObject;
                        add.object.clear();
                        token[0] = 0;
                        tidx = 0;
                        add.object_desc = NodeDescLiteral;
                    }
                    else if (lookAhead == '<')
                    {
                        parserState = sObject;
                        add.object.clear();
                        token[0] = 0;
                        tidx = 0;
                        add.object_desc = NodeDescUri;
                    }
                    else
                    {
                        // parse error
                        return false;
                    }
                }
                break;

            case sObject:
                if (lookAhead != '\n')
                {
                    if ((add.object_desc==NodeDescLiteral && lookAhead != '"') ||
                        (add.object_desc==NodeDescUri && lookAhead != '>'))
                    {
                        token[tidx++] = lookAhead;
                    }
                    else
                    {
                        token[tidx++] = 0;
                        add.object = token;
                        parserState = sTriple;
                    }
                }
                else
                {
                    // parse error - newline in middle of object
                    return false;
                }
                break;

            case sTriple:
                if (lookAhead == '\n')
                {
                    // add new triple to Graph
                    insert_node (add);
                    parserState = sNTriples;
                }
                break;

        }

        stream++;
    }
    return true;
}

bool NTripleFormat::serialize (const efd::utf8string& filename, Graph* graph)
{
    bool retval=false;

    File* pkFile = File::GetFile(filename.c_str(), File::WRITE_ONLY);

    if (pkFile)
    {
        Graph::Iterator it(graph), end;
        for (it.begin(); it != end; ++it)
        {
            write_triple (*it, pkFile);
        }
        EE_DELETE pkFile;
        retval = true;
    }
    else
    {
        EE_LOG(
            efd::kAssets,
            efd::ILogger::kERR1,
            ("Failed to persist asset web changes. Could not write to %s",
            filename.c_str()));
    }

    return retval;
}

bool NTripleFormat::serialize (const efd::utf8string& filename, SimpleGraph* graph)
{
    bool retval=false;

    File* pkFile = File::GetFile(filename.c_str(), File::WRITE_ONLY);

    if (!pkFile)
    {
        // Perhaps the parent directory needs to be created
        utf8string path = PathUtils::PathRemoveFileName(filename);
        MakeDir (path.c_str());
        pkFile = File::GetFile(filename.c_str(), File::WRITE_ONLY);
    }

    if (pkFile)
    {
        TripleSetIterator it = graph->begin();
        for (; it != graph->end(); ++it)
        {
            write_triple (*it, pkFile);
        }
        EE_DELETE pkFile;
        retval = true;
    }
    else
    {
        EE_LOG(
            efd::kAssets,
            efd::ILogger::kERR1,
            ("Failed to persist asset web changes. Could not write to %s",
            filename.c_str()));
    }

    return retval;
}

void NTripleFormat::write_triple (const Triple& triple, File* pkFile)
{
    write_node (triple.subject, triple.subject_desc, pkFile);
    pkFile->Write(" ", 1);
    write_node (triple.predicate, triple.predicate_desc, pkFile);
    pkFile->Write(" ", 1);
    write_node (triple.object, triple.object_desc, pkFile);
    pkFile->Write(".\n", 2);
}

void NTripleFormat::write_node (const utf8string& str, NodeDescription desc, File* pkFile)
{
    efd::utf8string strBuffer;
    switch (desc)
    {
    case NodeDescUri:
        strBuffer.sprintf("%c%s%c", '<', str.c_str(), '>');
        pkFile->Write(strBuffer.c_str(), strBuffer.length());
        break;

    case NodeDescLiteral:
    case NodeDescBlank:
        strBuffer.sprintf("%c%s%c", '"', str.c_str(), '"');
        pkFile->Write(strBuffer.c_str(), strBuffer.length());
        break;

    case NodeDescNone:
    default:
    break;

    }
}

//--------------------------------------------------------------------------------------------------
Graph::Iterator::Iterator(Graph* graph)
: m_graph (graph)
, m_end (true)
{
}

Graph::Iterator::Iterator()
: m_graph (NULL)
, m_end (true)
{
}

void Graph::Iterator::begin()
{
    m_iterator = m_graph->m_store.begin();
    m_end = (m_iterator == m_graph->m_store.end());

    if (!m_end)
    {
        m_cursor.subject = m_iterator->first;
        m_cursor.predicate = m_graph->m_reference_property;
        m_cursor.object = m_iterator->second;
    }
}

const Triple& Graph::Iterator::operator*() const
{
    return m_cursor;
}

bool Graph::Iterator::operator==(const Graph::Iterator& rhs) const
{
    return m_end == rhs.m_end;
}

bool Graph::Iterator::operator!=(const Graph::Iterator& rhs) const
{
    return m_end != rhs.m_end;
}

Graph::Iterator& Graph::Iterator::operator++()
{
    m_iterator++;
    m_end = (m_iterator == m_graph->m_store.end());

    if (!m_end)
    {
        m_cursor.subject = m_iterator->first;
        m_cursor.predicate = m_graph->m_reference_property;
        m_cursor.object = m_iterator->second;
    }

    return *this;
}

bool Graph::Iterator::find(const efd::utf8string &key)
{
    m_iterator = m_graph->m_store.find (key);
    m_end = (m_iterator == m_graph->m_store.end());

    if (!m_end)
    {
        m_cursor.subject = m_iterator->first;
        m_cursor.predicate = m_graph->m_reference_property;
        m_cursor.object = m_iterator->second;
    }

    return !m_end;
}

//--------------------------------------------------------------------------------------------------
SimpleGraph::SimpleGraph ()
: m_isLoaded (false)
, m_dirty (false)
{
}

bool SimpleGraph::load (const utf8string& filename)
{
    m_filename = PathUtils::PathMakeNative (filename);
    m_filename = PathUtils::PathRemoveDotDots(m_filename);

    File* pkFile = File::GetFile(m_filename.c_str(), File::READ_ONLY);
    NTripleFormat parser;
    char buf[513];

    if (pkFile)
    {
        unsigned int sz = pkFile->Read(buf, 512);
        while (sz)
        {
            buf[sz] = '\0';
            m_isLoaded = parser.parse (buf, this);
            if (!m_isLoaded) break;
            sz = pkFile->Read(buf, 512);
        }

        EE_DELETE pkFile;
    }
    else
    {
        m_isLoaded = true;
    }

    m_dirty = false;
    return m_isLoaded;
}

bool SimpleGraph::isLoaded(bool useCache)
{
    if (!useCache)
    {
        clear();
        m_dirty = false;
        m_isLoaded = false;
    }
    return m_isLoaded;
}

GraphSaveResult SimpleGraph::save(bool always_overwrite)
{
    GraphSaveResult retval = GRAPH_SAVE_UNCHANGED;

    if (m_dirty || always_overwrite)
    {
        if (size()>0)
        {
            NTripleFormat nt;
            if (nt.serialize (m_filename, this))
            {
                m_dirty = false;
                retval = GRAPH_SAVE_SUCCESS;
            }
            else
            {
                retval = GRAPH_SAVE_FAIL;
            }
        }
        else
        {
#ifdef EE_PLATFORM_WIN32
            ::DeleteFile (m_filename.c_str());
#endif
            m_dirty = false;
            retval = GRAPH_SAVE_SUCCESS;
        }
    }

    return retval;
}

bool SimpleGraph::find_triple (const Triple& triple)
{
    TripleSetIterator i=begin();
    for (; i!=end(); ++i)
    {
        if (Graph::compareTriple(*i, triple))
        {
            return true;
        }
    }
    return false;
}

void SimpleGraph::insert_triple (const Triple& triple)
{
    if (!find_triple (triple))
    {
        push_back (triple);
        m_dirty = true;
    }
}

void SimpleGraph::merge_triple (const Triple& triple)
{
    Triple search = triple;
    Triple replace = triple;

    vector<Triple>::iterator i=begin();
    search.object = "";
    search.object_desc = NodeDescNone;
    for (; i!=end(); ++i)
    {
        if (Graph::compareTriple(*i, search))
        {
            // triple is already in the graph, possibly with a different value
            if (i->object != replace.object)
            {
                i->object = replace.object;
                i->object_desc = replace.object_desc;
                m_dirty = true;
            }
            return;
        }
    }

    // add new triple to the end of the graph
    push_back (triple);
    m_dirty = true;
}

void SimpleGraph::query (const Triple& q, TripleSet& matches)
{
    TripleSetIterator i=begin();
    for (; i!=end(); ++i)
    {
        if (Graph::compareTriple (*i, q))
            matches.push_back (*i);
    }
}
