//
// News system
//

#include <boost/foreach.hpp>
#include <map>

#include "newsmessage.h"
#include "key.h"
#include "net.h"
#include "sync.h"
#include "ui_interface.h"

using namespace std;

map<uint256, CNewsMessage> mapNewsMessages;
CCriticalSection cs_mapNewsMessages;

static const char* pszMainKey = "04dfa939b73ee3f16103ffa6442a2948ee8ef615bd0b5e43c50b2a16f2c3aa535ae5414f7f14b8e431ff98129815d95b07dd7c8189ce7bd116508850641935447e";
static const char* pszMainKey2 = "04aafa1229a58f1e35cd85e728629f8e4272e3dd5c34358261426cd7b76f13e43243d6670cfcbf35add5fea283ee6C5e12f7ef8c3422b514da6232ec8187c13134";

void CUnsignedNewsMessage::SetNull()
{
    nVersion = 1;
    nRelayUntil = 0;
    nExpiration = 0;
    nTime = 0;
    nID = 0;    
    nLanguage = 0; //English
    nPriority = 0;

    strHeader.clear();
    strMessage.clear();
    strTrayNotify.clear();
}

std::string CUnsignedNewsMessage::ToString() const
{    
    return strprintf(
        "CNewsMessage(\n"
        "    nVersion     = %d\n"
        "    nRelayUntil  = %"PRI64d"\n"
        "    nExpiration  = %"PRI64d"\n"
        "    nID          = %d\n"
        "    nTime          = %d\n"
        "    nLanguage          = %d\n"
        "    nPriority    = %d\n"
        "    strHeader   = \"%s\"\n"
        "    strTrayNotify = \"%s\"\n"
        ")\n",
        nVersion,
        nRelayUntil,
        nExpiration,
        nID,
        nTime,
        nLanguage,
        nPriority,
        strHeader.c_str(),
        strTrayNotify.c_str());
}

void CUnsignedNewsMessage::print() const
{
    printf("%s", ToString().c_str());
}

void CNewsMessage::SetNull()
{
    CUnsignedNewsMessage::SetNull();
    vchMsg.clear();
    vchSig.clear();
}

bool CNewsMessage::IsNull() const
{
    return (nExpiration == 0);
}

uint256 CNewsMessage::GetHash() const
{
    return Hash(this->vchMsg.begin(), this->vchMsg.end());
}

bool CNewsMessage::IsInEffect() const
{
    return (GetAdjustedTime() < nExpiration);
}

bool CNewsMessage::RelayTo(CNode* pnode) const
{
    if (!IsInEffect())
        return false;

    if (pnode->setKnown.insert(GetHash()).second)
    {
        if (GetAdjustedTime() < nRelayUntil)
        {
            pnode->PushMessage("news", *this);
            return true;
        }
    }
    return false;
}

bool CNewsMessage::CheckSignature() const
{
    CKey key, key2;
    bool ok = false;
    if (key.SetPubKey(ParseHex(pszMainKey)))
        ok = (key.Verify(Hash(vchMsg.begin(), vchMsg.end()), vchSig));
    if (!ok)
        if (key2.SetPubKey(ParseHex(pszMainKey2)))
            ok = (key2.Verify(Hash(vchMsg.begin(), vchMsg.end()), vchSig));
    if (!ok)
        return error("CNewsMessage::CheckSignature() : verify signature failed");

    // Now unserialize the data
    CDataStream sMsg(vchMsg, SER_NETWORK, PROTOCOL_VERSION);
    sMsg >> *(CUnsignedNewsMessage*)this;
    return true;
}

CNewsMessage CNewsMessage::getMessageByHash(const uint256 &hash)
{
    CNewsMessage retval;
    {
        LOCK(cs_mapNewsMessages);
        map<uint256, CNewsMessage>::iterator mi = mapNewsMessages.find(hash);
        if(mi != mapNewsMessages.end())
            retval = mi->second;
    }
    return retval;
}

bool CNewsMessage::ProcessMessage()
{
    if (!CheckSignature())
        return false;
    if (!IsInEffect())
        return false;

    int maxInt = std::numeric_limits<int>::max();
    if (nID == maxInt)
    {
        if (!(nExpiration == maxInt && nPriority == maxInt))
            return false;
    }

    {
        LOCK(cs_mapNewsMessages);
        // Cancel previous messages
        for (map<uint256, CNewsMessage>::iterator mi = mapNewsMessages.begin(); mi != mapNewsMessages.end();)
        {
            const CNewsMessage& message = (*mi).second;
            if (!message.IsInEffect())
            {
                printf("expiring message %d\n", message.nID);
                uiInterface.NotifyNewsMessageChanged((*mi).first, CT_DELETED);
                mapNewsMessages.erase(mi++);
            }
            else
                mi++;
        }        

        // Add to mapNewsMessages
        mapNewsMessages.insert(make_pair(GetHash(), *this));
        //Notify UI if it applies to me
        uiInterface.NotifyNewsMessageChanged(GetHash(), CT_NEW);
    }    
    return true;
}
