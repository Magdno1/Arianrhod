//====== Copyright 1996-2013, Valve Corporation, All rights reserved. =======
//
// Purpose: interface to steam ugc
//
//=============================================================================

#ifndef ISTEAMUGC_H
#define ISTEAMUGC_H
#ifdef _WIN32
#pragma once
#endif

#include "isteamclient.h"

// callbacks
#if defined( VALVE_CALLBACK_PACK_SMALL )
#pragma pack( push, 4 )
#elif defined( VALVE_CALLBACK_PACK_LARGE )
#pragma pack( push, 8 )
#else
#error isteamclient.h must be included
#endif


typedef uint64 UGCQueryHandle_t;
typedef uint64 UGCUpdateHandle_t;


const UGCQueryHandle_t k_UGCQueryHandleInvalid = 0xffffffffffffffffull;
const UGCUpdateHandle_t k_UGCUpdateHandleInvalid = 0xffffffffffffffffull;


// Matching UGC types for queries
enum EUGCMatchingUGCType
{
    k_EUGCMatchingUGCType_Items              = 0,       // both mtx items and ready-to-use items
    k_EUGCMatchingUGCType_Items_Mtx          = 1,
    k_EUGCMatchingUGCType_Items_ReadyToUse   = 2,
    k_EUGCMatchingUGCType_Collections        = 3,
    k_EUGCMatchingUGCType_Artwork            = 4,
    k_EUGCMatchingUGCType_Videos             = 5,
    k_EUGCMatchingUGCType_Screenshots        = 6,
    k_EUGCMatchingUGCType_AllGuides          = 7,       // both web guides and integrated guides
    k_EUGCMatchingUGCType_WebGuides          = 8,
    k_EUGCMatchingUGCType_IntegratedGuides   = 9,
    k_EUGCMatchingUGCType_UsableInGame       = 10,      // ready-to-use items and integrated guides
    k_EUGCMatchingUGCType_ControllerBindings = 11,
};

// Different lists of published UGC for a user.
// If the current logged in user is different than the specified user, then some options may not be allowed.
enum EUserUGCList
{
    k_EUserUGCList_Published,
    k_EUserUGCList_VotedOn,
    k_EUserUGCList_VotedUp,
    k_EUserUGCList_VotedDown,
    k_EUserUGCList_WillVoteLater,
    k_EUserUGCList_Favorited,
    k_EUserUGCList_Subscribed,
    k_EUserUGCList_UsedOrPlayed,
    k_EUserUGCList_Followed,
};

// Sort order for user published UGC lists (defaults to creation order descending)
enum EUserUGCListSortOrder
{
    k_EUserUGCListSortOrder_CreationOrderDesc,
    k_EUserUGCListSortOrder_CreationOrderAsc,
    k_EUserUGCListSortOrder_TitleAsc,
    k_EUserUGCListSortOrder_LastUpdatedDesc,
    k_EUserUGCListSortOrder_SubscriptionDateDesc,
    k_EUserUGCListSortOrder_VoteScoreDesc,
    k_EUserUGCListSortOrder_ForModeration,
};

// Combination of sorting and filtering for queries across all UGC
enum EUGCQuery
{
    k_EUGCQuery_RankedByVote                                  = 0,
    k_EUGCQuery_RankedByPublicationDate                       = 1,
    k_EUGCQuery_AcceptedForGameRankedByAcceptanceDate         = 2,
    k_EUGCQuery_RankedByTrend                                 = 3,
    k_EUGCQuery_FavoritedByFriendsRankedByPublicationDate     = 4,
    k_EUGCQuery_CreatedByFriendsRankedByPublicationDate       = 5,
    k_EUGCQuery_RankedByNumTimesReported                      = 6,
    k_EUGCQuery_CreatedByFollowedUsersRankedByPublicationDate = 7,
    k_EUGCQuery_NotYetRated                                   = 8,
    k_EUGCQuery_RankedByTotalVotesAsc                         = 9,
    k_EUGCQuery_RankedByVotesUp                               = 10,
    k_EUGCQuery_RankedByTextSearch                            = 11,
};


const uint32 kNumUGCResultsPerPage = 50;

// Details for a single published file/UGC
struct SteamUGCDetails_t
{
    PublishedFileId_t m_nPublishedFileId;
    EResult m_eResult;                                              // The result of the operation.
    EWorkshopFileType m_eFileType;                                  // Type of the file
    AppId_t m_nCreatorAppID;                                        // ID of the app that created this file.
    AppId_t m_nConsumerAppID;                                       // ID of the app that will consume this file.
    char m_rgchTitle[k_cchPublishedDocumentTitleMax];               // title of document
    char m_rgchDescription[k_cchPublishedDocumentDescriptionMax];   // description of document
    uint64 m_ulSteamIDOwner;                                        // Steam ID of the user who created this content.
    uint32 m_rtimeCreated;                                          // time when the published file was created
    uint32 m_rtimeUpdated;                                          // time when the published file was last updated
    uint32 m_rtimeAddedToUserList;                                  // time when the user added the published file to their list (not always applicable)
    ERemoteStoragePublishedFileVisibility m_eVisibility;            // visibility
    bool m_bBanned;                                                 // whether the file was banned
    bool m_bAcceptedForUse;                                         // developer has specifically flagged this item as accepted in the Workshop
    bool m_bTagsTruncated;                                          // whether the list of tags was too long to be returned in the provided buffer
    char m_rgchTags[k_cchTagListMax];                               // comma separated list of all tags associated with this file
    // file/url information
    UGCHandle_t m_hFile;                                            // The handle of the primary file
    UGCHandle_t m_hPreviewFile;                                     // The handle of the preview file
    char m_pchFileName[k_cchFilenameMax];                           // The cloud filename of the primary file
    int32 m_nFileSize;                                              // Size of the primary file
    int32 m_nPreviewFileSize;                                       // Size of the preview file
    char m_rgchURL[k_cchPublishedFileURLMax];                       // URL (for a video or a website)
    // voting information
    uint32 m_unVotesUp;                                             // number of votes up
    uint32 m_unVotesDown;                                           // number of votes down
    float m_flScore;                                                // calculated score
};

//-----------------------------------------------------------------------------
// Purpose: Steam UGC support API
//-----------------------------------------------------------------------------
class ISteamUGC
{
public:

    // Query UGC associated with a user. Creator app id or consumer app id must be valid and be set to the current running app. unPage should start at 1.
    virtual UGCQueryHandle_t CreateQueryUserUGCRequest( AccountID_t unAccountID, EUserUGCList eListType, EUGCMatchingUGCType eMatchingUGCType, EUserUGCListSortOrder eSortOrder, AppId_t nCreatorAppID, AppId_t nConsumerAppID, uint32 unPage ) = 0;

    // Query for all matching UGC. Creator app id or consumer app id must be valid and be set to the current running app. unPage should start at 1.
    virtual UGCQueryHandle_t CreateQueryAllUGCRequest( EUGCQuery eQueryType, EUGCMatchingUGCType eMatchingeMatchingUGCTypeFileType, AppId_t nCreatorAppID, AppId_t nConsumerAppID, uint32 unPage ) = 0;

    // Send the query to Steam
    virtual SteamAPICall_t SendQueryUGCRequest( UGCQueryHandle_t handle ) = 0;

    // Retrieve an individual result after receiving the callback for querying UGC
    virtual bool GetQueryUGCResult( UGCQueryHandle_t handle, uint32 index, SteamUGCDetails_t *pDetails ) = 0;

    // Release the request to free up memory, after retrieving results
    virtual bool ReleaseQueryUGCRequest( UGCQueryHandle_t handle ) = 0;

    // Options to set for querying UGC
    virtual bool AddRequiredTag( UGCQueryHandle_t handle, const char *pTagName ) = 0;
    virtual bool AddExcludedTag( UGCQueryHandle_t handle, const char *pTagName ) = 0;
    virtual bool SetReturnLongDescription( UGCQueryHandle_t handle, bool bReturnLongDescription ) = 0;
    virtual bool SetReturnTotalOnly( UGCQueryHandle_t handle, bool bReturnTotalOnly ) = 0;
    virtual bool SetAllowCachedResponse( UGCQueryHandle_t handle, uint32 unMaxAgeSeconds ) = 0;

    // Options only for querying user UGC
    virtual bool SetCloudFileNameFilter( UGCQueryHandle_t handle, const char *pMatchCloudFileName ) = 0;

    // Options only for querying all UGC
    virtual bool SetMatchAnyTag( UGCQueryHandle_t handle, bool bMatchAnyTag ) = 0;
    virtual bool SetSearchText( UGCQueryHandle_t handle, const char *pSearchText ) = 0;
    virtual bool SetRankedByTrendDays( UGCQueryHandle_t handle, uint32 unDays ) = 0;

    // Request full details for one piece of UGC
    virtual SteamAPICall_t RequestUGCDetails( PublishedFileId_t nPublishedFileID, uint32 unMaxAgeSeconds ) = 0;

};

#define STEAMUGC_INTERFACE_VERSION "STEAMUGC_INTERFACE_VERSION002"

//-----------------------------------------------------------------------------
// Purpose: Callback for querying UGC
//-----------------------------------------------------------------------------
struct SteamUGCQueryCompleted_t
{
    enum { k_iCallback = k_iClientUGCCallbacks + 1 };
    UGCQueryHandle_t m_handle;
    EResult m_eResult;
    uint32 m_unNumResultsReturned;
    uint32 m_unTotalMatchingResults;
    bool m_bCachedData; // indicates whether this data was retrieved from the local on-disk cache
};


//-----------------------------------------------------------------------------
// Purpose: Callback for requesting details on one piece of UGC
//-----------------------------------------------------------------------------
struct SteamUGCRequestUGCDetailsResult_t
{
    enum { k_iCallback = k_iClientUGCCallbacks + 2 };
    SteamUGCDetails_t m_details;
    bool m_bCachedData; // indicates whether this data was retrieved from the local on-disk cache
};


//-----------------------------------------------------------------------------
// Purpose: k_iClientUGCCallbacks + 3 to k_iClientUGCCallbacks + 6 in use
//-----------------------------------------------------------------------------


#pragma pack( pop )

#endif // ISTEAMUGC_H
