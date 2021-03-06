/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "DDEQ.C;1  16-Dec-92,10:15:52  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

/*
    TODO:
        
        - increase Q if full
            - allocate more entries
            - copy from oldest+1 through nEntries-1 to entry 1
                [none if oldest == nEntries-1]
              from there, copy 0 to newest
                [none if newest == nEntries-1]
            - set oldest = 0
            - set newest = old nEntries-1
            - call DDEQAdd() recursively
 */

#include    "host.h"
#include    "windows.h"
#include    "spt.h"
#include    "ddeq.h"
#include    "wwassert.h"
#include    "debug.h"
#include    "nddemsg.h"
#include    "nddelog.h"

USES_ASSERT

typedef struct {
    int         oldest;
    int         newest;
    int         nEntries;
    DDEQENT     qEnt[ 1 ];
} DDEQ;
typedef DDEQ FAR *LPDDEQ;

#ifdef COMMENT
    oldest      newest  #liveEntries    valid entries
        0       0       0               
        0       1       1               1
        0       2       2               1,2
        0       3       3               1,2,3
        1       3       2               2,3
        2       3       1               3
        3       3       0
        3       0       1               0
            
            
    oldest      newest  valid entries
        0       3       1,2,3
        1       0       2,3,0
        2       1       3,0,1
        3       2       0,1,2
#endif

HDDEQ
FAR PASCAL
DDEQAlloc( void )
{
    HDDEQ       hDDEQ;
    LPDDEQ      lpDDEQ;
    DWORD       size;

    size = (DWORD) sizeof( DDEQ ) + ((INIT_Q_SIZE-1) * sizeof(DDEQENT));
    hDDEQ = GetGlobalAlloc( GMEM_MOVEABLE, size);
    if( hDDEQ )  {                              // did the alloc succeed?
        lpDDEQ = (LPDDEQ) GlobalLock( hDDEQ );
        assert( lpDDEQ );                       // did the lock succeed?
        lpDDEQ->newest          = 0;
        lpDDEQ->oldest          = 0;
        lpDDEQ->nEntries        = INIT_Q_SIZE;
        GlobalUnlock( hDDEQ );
    }
    
    return( hDDEQ );
}

BOOL
FAR PASCAL
DDEQAdd(
    HDDEQ       hDDEQ,
    LPDDEQENT   lpDDEQEnt )
{
    register LPDDEQ     lpDDEQ;
    int                 candidate;
    BOOL                bAdded;
    
    lpDDEQ = (LPDDEQ) GlobalLock( hDDEQ );
    assert( lpDDEQ );
    candidate = (lpDDEQ->newest + 1) % lpDDEQ->nEntries;
    if( candidate == lpDDEQ->oldest )  {
        /*  Unable to add to DDE msg queue. Newest: %1, Oldest: %2, Entries: %3 */
        NDDELogError(MSG059,
            LogString("%d", lpDDEQ->newest),
            LogString("%d", lpDDEQ->oldest),
            LogString("%d", lpDDEQ->nEntries), NULL);
        bAdded = FALSE;
    } else {
        lpDDEQ->newest = candidate;
        lpDDEQ->qEnt[ lpDDEQ->newest ] = *lpDDEQEnt;
        bAdded = TRUE;
    }
    GlobalUnlock( hDDEQ );

    return( bAdded );
}

BOOL
FAR PASCAL
DDEQRemove(
    HDDEQ       hDDEQ,
    LPDDEQENT   lpDDEQEnt )
{
    register LPDDEQ     lpDDEQ;
    BOOL                bRemoved;
    
    lpDDEQ = (LPDDEQ) GlobalLock( hDDEQ );
    assert( lpDDEQ );
    if( lpDDEQ->oldest == lpDDEQ->newest )  {
        bRemoved = FALSE;
    } else {
        lpDDEQ->oldest = (lpDDEQ->oldest + 1) % lpDDEQ->nEntries;
        *lpDDEQEnt = lpDDEQ->qEnt[ lpDDEQ->oldest ];
        bRemoved = TRUE;
    }
    GlobalUnlock( hDDEQ );

    return( bRemoved );
}

VOID
FAR PASCAL
DDEQFree( HDDEQ hDDEQ )
{
    DDEQENT     DDEQEnt;
    DWORD       size;

    while( DDEQRemove(hDDEQ, &DDEQEnt )){
        if( DDEQEnt.hData )  {
            size = GlobalSize((HANDLE)DDEQEnt.hData);
            if (size)
                GlobalFree((HANDLE)DDEQEnt.hData);
            else {
                if (!DDEQEnt.fRelease) {
                    /*  DDEQFree() releasing invalid msg handle %1 */
                    NDDELogError(MSG060,
                        LogString("0x%0X", DDEQEnt.hData), NULL);
                }
            }
        }
    }
    GlobalFree( hDDEQ );
}

