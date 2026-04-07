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

#include <efd/AssetWeb.h>
#include <efd/AssetUriReader.h>

using namespace efd;

//-------------------------------------------------------------------------------------------------

// Constant string definitions
/*static*/ const char* URIReader::GlobalNamespace;
/*static*/ const char* URIReader::GlobalNamespace_tag;
/*static*/ const char* URIReader::GlobalNamespace_label;
/*static*/ const char* URIReader::GlobalNamespace_class;
/*static*/ const char* URIReader::GlobalNamespace_name;
/*static*/ const char* URIReader::GlobalNamespace_relpath;
/*static*/ const char* URIReader::GlobalNamespace_canonical;
/*static*/ const char* URIReader::GlobalNamespace_llid;
/*static*/ const char* URIReader::GlobalNamespace_logpath;
/*static*/ const char* URIReader::GlobalNamespace_stdtags;
/*static*/ const char* URIReader::GlobalNamespace_stdtags_value;
/*static*/ const char* URIReader::GlobalNamespace_usertags;
/*static*/ const char* URIReader::GlobalNamespace_usertags_value;
/*static*/ const char* URIReader::GlobalNamespace_userenums;
/*static*/ const char* URIReader::GlobalNamespace_userenums_value;
/*static*/ const char* URIReader::GlobalNamespace_ids;
/*static*/ const char* URIReader::GlobalNamespace_ids_instance;
/*static*/ const char* URIReader::GlobalNamespace_ids_author;
/*static*/ const char* URIReader::GlobalNamespace_ckpt;
/*static*/ const char* URIReader::GlobalNamespace_ckpt_success;
/*static*/ const char* URIReader::GlobalNamespace_ckpt_hash;
/*static*/ const char* URIReader::GlobalNamespace_ckpt_version;
/*static*/ const char* URIReader::GlobalNamespace_llid_persist;

//-------------------------------------------------------------------------------------------------
void URIReader::_SDMInit()
{
    URIReader::GlobalNamespace = "http://emergent.net/aweb/1.0/";
    URIReader::GlobalNamespace_tag = "http://emergent.net/aweb/1.0/tag";
    URIReader::GlobalNamespace_label = "http://emergent.net/aweb/1.0/label";
    URIReader::GlobalNamespace_class = "http://emergent.net/aweb/1.0/class";
    URIReader::GlobalNamespace_name = "http://emergent.net/aweb/1.0/name";
    URIReader::GlobalNamespace_relpath = "http://emergent.net/aweb/1.0/relpath";
    URIReader::GlobalNamespace_canonical = "http://emergent.net/aweb/1.0/canonical";
    URIReader::GlobalNamespace_llid = "http://emergent.net/aweb/1.0/llid";
    URIReader::GlobalNamespace_logpath = "http://emergent.net/aweb/1.0/logpath";
    URIReader::GlobalNamespace_stdtags = "http://emergent.net/aweb/1.0/std-tags/";
    URIReader::GlobalNamespace_stdtags_value = "http://emergent.net/aweb/1.0/std-tags/value";
    URIReader::GlobalNamespace_usertags = "http://emergent.net/aweb/1.0/user-tags/";
    URIReader::GlobalNamespace_usertags_value = "http://emergent.net/aweb/1.0/user-tags/value";
    URIReader::GlobalNamespace_userenums = "http://emergent.net/aweb/1.0/user-enums/";
    URIReader::GlobalNamespace_userenums_value = "http://emergent.net/aweb/1.0/user-enums/value";
    URIReader::GlobalNamespace_ids = "http://emergent.net/aweb/1.0/ids";
    URIReader::GlobalNamespace_ids_instance = "http://emergent.net/aweb/1.0/ids/instance";
    URIReader::GlobalNamespace_ids_author = "http://emergent.net/aweb/1.0/ids/author";
    URIReader::GlobalNamespace_ckpt = "http://emergent.net/aweb/1.0/ckpt";
    URIReader::GlobalNamespace_ckpt_success = "http://emergent.net/aweb/1.0/ckpt/success";
    URIReader::GlobalNamespace_ckpt_hash = "http://emergent.net/aweb/1.0/ckpt/hash";
    URIReader::GlobalNamespace_ckpt_version = "http://emergent.net/aweb/1.0/ckpt/version";
    URIReader::GlobalNamespace_llid_persist = "http://emergent.net/aweb/1.0/ckpt/llidpersist";
}
//-------------------------------------------------------------------------------------------------
void URIReader::_SDMShutdown()
{
}
//-------------------------------------------------------------------------------------------------
URIReader::URIReader(const utf8string& uri)
:   m_uri(uri)
,   m_isUid(false)
{
    vector<AssetTagDescriptor> tagNames;
    splitURI(tagNames);
}
//-------------------------------------------------------------------------------------------------
URIReader::URIReader(const utf8string& uri, const vector<AssetTagDescriptor>& tagNames)
:   m_uri(uri)
,   m_isUid(false)
{
    splitURI(tagNames);
}
//-------------------------------------------------------------------------------------------------
efd::Bool URIReader::isAssetId(const utf8string& uri)
{
    return uri.find("urn:uuid:")==0 && uri.length()==45;
}
//-------------------------------------------------------------------------------------------------
efd::Bool URIReader::isAssetId()
{
    // m_isUid means that uuid: appeared in the uri
    // m_name will hold the UID in standard hex form. Should be 36 chars.

    return (m_isUid && m_name.length()==36);
}
//-------------------------------------------------------------------------------------------------
void URIReader::getAssetId(efd::utf8string& uri)
{
    EE_ASSERT (m_isUid);

    uri = "urn:uuid:" + m_name;
}
//-------------------------------------------------------------------------------------------------
void URIReader::getScheme (utf8string& scheme)
{
    scheme = m_scheme;
}
//-------------------------------------------------------------------------------------------------
void URIReader::getNameLiteral(utf8string& name)
{
    name = m_name;
}
//-------------------------------------------------------------------------------------------------
void URIReader::getTagLiterals(vector<utf8string>& tagNames)
{
    map<utf8string, AWebTagType>::iterator i;
    tagNames.clear();
    for (i = m_uriTags.begin(); i!=m_uriTags.end(); i++)
    {
        if (i->second != AWEB_NONE_TAG_TYPE)
            tagNames.push_back(i->first);
    }
}
//-------------------------------------------------------------------------------------------------
Bool URIReader::hasTag(const utf8string& tag)
{
    if (m_uriTags.find(tag) != m_uriTags.end())
    {
        AWebTagType my_type = m_uriTags[tag];
        return my_type!=AWEB_NONE_TAG_TYPE;
    }
    else
    {
        return false;
    }
}
//-------------------------------------------------------------------------------------------------
int URIReader::UpperBitsValue()
{
    int retval=-1;

    if (isAssetId())
    {
        utf8string top_two_hex_digits = m_name.substr (0, 2);
        sscanf (top_two_hex_digits.c_str(), "%x", &retval);
    }

    return retval;
}
//-------------------------------------------------------------------------------------------------
void URIReader::splitURI(const efd::vector<efd::AssetTagDescriptor>& tagNames)
{
    int cnt = 0;
    utf8string temp = m_uri;

    // initialize m_uriTags from tagNames
    vector<AssetTagDescriptor>::const_iterator i;
    map<utf8string, AWebTagType> tagTypes;
    Bool recognize_all_tags = tagNames.size()==0;
    for (i=tagNames.begin(); i!=tagNames.end(); i++)
    {
        m_uriTags[i->tvalue] = AWEB_NONE_TAG_TYPE;
        tagTypes[i->tvalue] = i->ttype;
    }

    // read uri and split at every ":" character
    size_t pos = temp.find_first_of(':');
    while (pos != utf8string::npos)
    {
        utf8string tag;
        tag = temp.substr(0, pos);

        cnt++;

        if (cnt==1)
        {
            // first part is the URI scheme
            m_scheme = tag;
        }
        else if (tag=="uuid")
        {
            m_isUid = true;
        }
        else if (tag=="llid")
        {
            // Next parameter is a logical asset id
            temp = temp.substr(pos+1, temp.length()-pos-1);
            pos = temp.find_first_of(':');
            if (pos != utf8string::npos)
            {
                m_logical_id = temp.substr(0, pos);
            }
            else
            {
                m_logical_id = temp;
            }
        }
        else
        {
            if (tag.find_first_of('/')==0)
            {
                // tag refers to a path value
                if (cnt==2)
                {
                    // path value in the 2nd position matches the canonical path property
                    m_canonical = tag;
                }
                else if (cnt>2)
                {
                    // path value beyond the 2nd position is a directory override
                    m_uriOverrides.push_back(tag);
                }
            }
            else if (recognize_all_tags ||
                m_uriTags.find (tag) != m_uriTags.end())
            {
                // set this known tag as present in the uri
                if (!recognize_all_tags)
                    m_uriTags[tag] = tagTypes[tag];
                else
                    m_uriTags[tag] = AWEB_LABEL_TAG_TYPE;
            }
            else
            {
                // treat tag as the "name" value
                m_name = tag;
            }
        }

        temp = temp.substr(pos+1, temp.length()-pos-1);
        pos = temp.find_first_of(':');
    }

    // handle the last part of the URI
    if (temp.length()>0)
    {
        if (m_isUid && cnt==2)
        {
            // for a urn:uuid, the name is the UID numeric portion
            m_name = temp;
        }
        else if (cnt==0)
        {
            // there were no colons; all we have is a scheme value
            m_scheme = temp;
        }
        else if (temp.find_first_of('/')==0)
        {
            if (cnt==1)
            {
                // tag refers to the value of the 'canonical' property
                m_canonical = temp;
            }
            else
            {
                m_uriOverrides.push_back(temp);
            }
        }
        else if (recognize_all_tags ||
            m_uriTags.find (temp) != m_uriTags.end())
        {
            // set this known tag as present in the uri
            if (!recognize_all_tags)
                m_uriTags[temp] = tagTypes[temp];
            else
                m_uriTags[temp] = AWEB_LABEL_TAG_TYPE;
        }
        else if (temp != m_logical_id)
        {
            // treat tag as the "name" value
            m_name = temp;
        }
    }
}
//-------------------------------------------------------------------------------------------------
void efd::URIReader::getCanonicalLiteral(efd::utf8string& name)
{
    name = m_canonical;
}
//-------------------------------------------------------------------------------------------------
void efd::URIReader::getLogicalId(efd::utf8string& id)
{
    id = m_logical_id;
}
//-------------------------------------------------------------------------------------------------
void URIReader::getLabels(vector<utf8string>& labelNames)
{
    map<utf8string, AWebTagType>::iterator i;
    labelNames.clear();
    for (i = m_uriTags.begin(); i!=m_uriTags.end(); i++)
    {
        if (i->second==AWEB_LABEL_TAG_TYPE || i->second==AWEB_STANDARD_TAG_TYPE)
            labelNames.push_back(i->first);
    }
}
//-------------------------------------------------------------------------------------------------
void URIReader::getClassifications(vector<utf8string>& classNames)
{
    map<utf8string, AWebTagType>::iterator i;
    classNames.clear();
    for (i = m_uriTags.begin(); i != m_uriTags.end(); i++)
    {
        if (i->second == AWEB_CLASSIFICATION_TAG_TYPE)
            classNames.push_back(i->first);
    }

    vector<utf8string>::iterator it;
    for (it = m_uriOverrides.begin(); it != m_uriOverrides.end(); it++)
    {
        classNames.push_back(*it);
    }
}
//-------------------------------------------------------------------------------------------------
