/**
 *  file    Window.c
 *  date    2009/09/28
 *  author  kkamagui
 *          Copyright(c)2008 All rights reserved by kkamagui
 *  brief   GUI �ý��ۿ� ���õ� �Լ��� ������ �ҽ� ����
 */
#include "Window.h"
#include "VBE.h"
#include "Task.h"
#include "Font.h"
#include "DynamicMemory.h"
#include "Utility.h"

// GUI �ý��� ���� �ڷᱸ��
static WINDOWPOOLMANAGER gs_stWindowPoolManager;
// ������ �Ŵ��� ���� �ڷᱸ��
static WINDOWMANAGER gs_stWindowManager;

//==============================================================================
//  ������ Ǯ ����
//==============================================================================
/**
 *  ������ Ǯ�� �ʱ�ȭ
 */
static void kInitializeWindowPool( void )
{
    int i;
    void* pvWindowPoolAddress;
    
    // �ڷᱸ�� �ʱ�ȭ
    kMemSet( &gs_stWindowPoolManager, 0, sizeof( gs_stWindowPoolManager ) );
    
    // ������ Ǯ�� �޸𸮸� �Ҵ�
    pvWindowPoolAddress = ( void* ) kAllocateMemory( sizeof( WINDOW ) * WINDOW_MAXCOUNT );
    if( pvWindowPoolAddress == NULL )
    {
        kPrintf( "Window Pool Allocate Fail\n" );
        while( 1 )
        {
            ;
        }
    }
    
    // ������ Ǯ�� ��巹���� �����ϰ� �ʱ�ȭ
    gs_stWindowPoolManager.pstStartAddress = ( WINDOW* ) pvWindowPoolAddress;
    kMemSet( pvWindowPoolAddress, 0, sizeof( WINDOW ) * WINDOW_MAXCOUNT );

    // ������ Ǯ�� ID�� �Ҵ�
    for( i = 0 ; i < WINDOW_MAXCOUNT ; i++ )
    {
        gs_stWindowPoolManager.pstStartAddress[ i ].stLink.qwID = i;
    }
    
    // �������� �ִ� ������ �Ҵ�� Ƚ���� �ʱ�ȭ
    gs_stWindowPoolManager.iMaxCount = WINDOW_MAXCOUNT;
    gs_stWindowPoolManager.iAllocatedCount = 1;
    
    // ���ؽ� �ʱ�ȭ
    kInitializeMutex( &( gs_stWindowPoolManager.stLock ) );
}


/**
 *  ������ �ڷᱸ���� �Ҵ�
 */
static WINDOW* kAllocateWindow( void )
{
    WINDOW* pstEmptyWindow;
    int i;

    // ����ȭ ó��
    kLock( &( gs_stWindowPoolManager.stLock ) );

    // �����찡 ��� �Ҵ�Ǿ����� ����
    if( gs_stWindowPoolManager.iUseCount == gs_stWindowPoolManager.iMaxCount )
    {
        // ����ȭ ó��
        kUnlock( &gs_stWindowPoolManager.stLock );
        return NULL;
    }

    // ������ Ǯ�� ��� ���鼭 �� ������ �˻�
    for( i = 0 ; i < gs_stWindowPoolManager.iMaxCount ; i++ )
    {
        // ID�� ���� 32��Ʈ�� 0�̸� ����ִ� ������ �ڷᱸ����
        if( ( gs_stWindowPoolManager.pstStartAddress[ i ].stLink.qwID >> 32 ) == 0 )
        {
            pstEmptyWindow = &( gs_stWindowPoolManager.pstStartAddress[ i ] );
            break;
        }
    }

    // ���� 32��Ʈ�� 0�� �ƴ� ������ �����ؼ� �Ҵ�� ������� ����
    pstEmptyWindow->stLink.qwID =
        ( ( QWORD ) gs_stWindowPoolManager.iAllocatedCount << 32 ) | i;

    // �ڷᱸ���� ��� ���� ������ �Ҵ�� Ƚ���� ����
    gs_stWindowPoolManager.iUseCount++;
    gs_stWindowPoolManager.iAllocatedCount++;
    if( gs_stWindowPoolManager.iAllocatedCount == 0 )
    {
        gs_stWindowPoolManager.iAllocatedCount = 1;
    }

    // ����ȭ ó��
    kUnlock( &( gs_stWindowPoolManager.stLock ) );

    // �������� ���ؽ� �ʱ�ȭ
    kInitializeMutex( &( pstEmptyWindow->stLock ) );

    return pstEmptyWindow;
}

/**
 *  ������ �ڷᱸ���� ����
 */
static void kFreeWindow( QWORD qwID )
{
    int i;

    // ������ ID�� ������ Ǯ�� �������� ���, ������ ID�� ���� 32��Ʈ�� �ε��� ������ ��
    i = GETWINDOWOFFSET( qwID );

    // ����ȭ ó��
    kLock( &( gs_stWindowPoolManager.stLock ) );
    
    // ������ �ڷᱸ���� �ʱ�ȭ�ϰ� ID ����
    kMemSet( &( gs_stWindowPoolManager.pstStartAddress[ i ] ), 0, sizeof( WINDOW ) );
    gs_stWindowPoolManager.pstStartAddress[ i ].stLink.qwID = i;

    // ��� ���� �ڷᱸ���� ������ ����
    gs_stWindowPoolManager.iUseCount--;

    // ����ȭ ó��
    kUnlock( &( gs_stWindowPoolManager.stLock ) );
}

//==============================================================================
//  ������� ������ �Ŵ��� ����
//==============================================================================
/**
 *  GUI �ý����� �ʱ�ȭ
 */
void kInitializeGUISystem( void )
{
    VBEMODEINFOBLOCK* pstModeInfo;
    QWORD qwBackgroundWindowID;
    EVENT* pstEventBuffer;

    // ������ Ǯ�� �ʱ�ȭ
    kInitializeWindowPool();

    // VBE ��� ���� ������ ��ȯ
    pstModeInfo = kGetVBEModeInfoBlock();

    // ���� �޸� ��巹�� ����
    gs_stWindowManager.pstVideoMemory = ( COLOR* )
        ( ( QWORD ) pstModeInfo->dwPhysicalBasePointer & 0xFFFFFFFF );

    // ���콺 Ŀ���� �ʱ� ��ġ ����
    gs_stWindowManager.iMouseX = pstModeInfo->wXResolution / 2;
    gs_stWindowManager.iMouseY = pstModeInfo->wYResolution / 2;

    // ȭ�� ������ ���� ����
    gs_stWindowManager.stScreenArea.iX1 = 0;
    gs_stWindowManager.stScreenArea.iY1 = 0;
    gs_stWindowManager.stScreenArea.iX2 = pstModeInfo->wXResolution - 1;
    gs_stWindowManager.stScreenArea.iY2 = pstModeInfo->wYResolution - 1;

    // ���ؽ� �ʱ�ȭ
    kInitializeMutex( &( gs_stWindowManager.stLock ) );

    // ������ ����Ʈ �ʱ�ȭ
    kInitializeList( &( gs_stWindowManager.stWindowList ) );
    
    // �̺�Ʈ ť���� ����� �̺�Ʈ �ڷᱸ�� Ǯ�� ����
    pstEventBuffer = ( EVENT* ) kAllocateMemory( sizeof( EVENT ) * 
        EVENTQUEUE_WNIDOWMANAGERMAXCOUNT );
    if( pstEventBuffer == NULL )
    {
        kPrintf( "Window Manager Event Queue Allocate Fail\n" );
        while( 1 )
        {
            ;
        }
    }
    // �̺�Ʈ ť�� �ʱ�ȭ
    kInitializeQueue( &( gs_stWindowManager.stEventQueue ), pstEventBuffer, 
            EVENTQUEUE_WNIDOWMANAGERMAXCOUNT, sizeof( EVENT ) );
    
    // ȭ���� ������Ʈ�� �� ����� ��Ʈ�� ���۸� ����
    // ��Ʈ���� ȭ�� ��ü ũ��� �����ؼ� �������� ����ϵ��� ��
    gs_stWindowManager.pbDrawBitmap = kAllocateMemory( ( pstModeInfo->wXResolution * 
        pstModeInfo->wYResolution + 7 ) / 8 );
    if( gs_stWindowManager.pbDrawBitmap == NULL )
    {
        kPrintf( "Draw Bitmap Allocate Fail\n" );
        while( 1 )
        {
            ;
        }
    }
    
    // ���콺 ��ư�� ���¿� ������ �̵� ���θ� �ʱ�ȭ
    gs_stWindowManager.bPreviousButtonStatus = 0;
    gs_stWindowManager.bWindowMoveMode = FALSE;
    gs_stWindowManager.qwMovingWindowID = WINDOW_INVALIDID;

    //--------------------------------------------------------------------------
    // ��� ������ ����
    //--------------------------------------------------------------------------
    // �÷��׿� 0�� �Ѱܼ� ȭ�鿡 �����츦 �׸��� �ʵ��� ��. ��� ������� ������ ���� 
    // ������ ��� ĥ�� �� ��Ÿ��
    qwBackgroundWindowID = kCreateWindow( 0, 0, pstModeInfo->wXResolution, 
            pstModeInfo->wYResolution, 0, WINDOW_BACKGROUNDWINDOWTITLE );
    gs_stWindowManager.qwBackgoundWindowID = qwBackgroundWindowID; 
    // ��� ������ ���ο� ������ ä��
    kDrawRect( qwBackgroundWindowID, 0, 0, pstModeInfo->wXResolution - 1, 
            pstModeInfo->wYResolution - 1, WINDOW_COLOR_SYSTEMBACKGROUND, TRUE );
    // ��� �����츦 ȭ�鿡 ��Ÿ��
    kShowWindow( qwBackgroundWindowID, TRUE );
}

/**
 *  ������ �Ŵ����� ��ȯ
 */
WINDOWMANAGER* kGetWindowManager( void )
{
    return &gs_stWindowManager;
}

/**
 *  ��� �������� ID�� ��ȯ
 */
QWORD kGetBackgroundWindowID( void )
{
    return gs_stWindowManager.qwBackgoundWindowID;
}

/**
 *  ȭ�� ������ ũ�⸦ ��ȯ
 */
void kGetScreenArea( RECT* pstScreenArea )
{
    kMemCpy( pstScreenArea, &( gs_stWindowManager.stScreenArea ), sizeof( RECT ) );
}

/**
 *  �����츦 ����
 *      ������ ���ð� ���� ���� �̺�Ʈ�� ���� ����
 */
QWORD kCreateWindow( int iX, int iY, int iWidth, int iHeight, DWORD dwFlags,
        const char* pcTitle )
{
    WINDOW* pstWindow;
    TCB* pstTask;
    QWORD qwActiveWindowID;
    EVENT stEvent;

    // ũ�Ⱑ 0�� ������� ���� �� ����
    if( ( iWidth <= 0 ) || ( iHeight <= 0 ) )
    {
        return WINDOW_INVALIDID;
    }

    // ������ �ڷᱸ���� �Ҵ�
    pstWindow = kAllocateWindow();
    if( pstWindow == NULL )
    {
        return WINDOW_INVALIDID;
    }

    // ������ ���� ����
    pstWindow->stArea.iX1 = iX;
    pstWindow->stArea.iY1 = iY;
    pstWindow->stArea.iX2 = iX + iWidth - 1;
    pstWindow->stArea.iY2 = iY + iHeight - 1;
    
    // ������ ���� ����
    kMemCpy( pstWindow->vcWindowTitle, pcTitle, WINDOW_TITLEMAXLENGTH );
    pstWindow->vcWindowTitle[ WINDOW_TITLEMAXLENGTH ] = '\0';

    // ������ ȭ�� ���ۿ� �̺�Ʈ ť���� ����� �̺�Ʈ �ڷᱸ�� Ǯ�� ����
    pstWindow->pstWindowBuffer = ( COLOR* ) kAllocateMemory( iWidth * iHeight *
            sizeof( COLOR ) );
    pstWindow->pstEventBuffer = ( EVENT* ) kAllocateMemory(
        EVENTQUEUE_WINDOWMAXCOUNT * sizeof( EVENT ) );
    if( ( pstWindow->pstWindowBuffer == NULL ) ||
        ( pstWindow->pstEventBuffer == NULL ) )
    {
        // ������ ���ۿ� �̺�Ʈ �ڷᱸ�� Ǯ�� ��� ��ȯ
        kFreeMemory( pstWindow->pstWindowBuffer );
        kFreeMemory( pstWindow->pstEventBuffer );

        // �޸� �Ҵ翡 �����ϸ� ������ �ڷᱸ�� ��ȯ
        kFreeWindow( pstWindow->stLink.qwID );
        return WINDOW_INVALIDID;
    }

    // �̺�Ʈ ť�� �ʱ�ȭ
    kInitializeQueue( &( pstWindow->stEventQueue ), pstWindow->pstEventBuffer,
            EVENTQUEUE_WINDOWMAXCOUNT, sizeof( EVENT ) );

    // �����츦 ������ �½�ũ�� ID�� ����
    pstTask = kGetRunningTask( kGetAPICID() );
    pstWindow->qwTaskID =  pstTask->stLink.qwID;

    // ������ �Ӽ� ����
    pstWindow->dwFlags = dwFlags;

    // ������ ��� �׸���
    kDrawWindowBackground( pstWindow->stLink.qwID );

    // ������ �׵θ� �׸���
    if( dwFlags & WINDOW_FLAGS_DRAWFRAME )
    {
        kDrawWindowFrame( pstWindow->stLink.qwID );
    }

    // ������ ���� ǥ���� �׸���
    if( dwFlags & WINDOW_FLAGS_DRAWTITLE )
    {
        kDrawWindowTitle( pstWindow->stLink.qwID, pcTitle, TRUE );
    }

    // ����ȭ ó��
    kLock( &( gs_stWindowManager.stLock ) );

    // ���� �ֻ����� �ִ� �����츦 ��ȯ
    qwActiveWindowID = kGetTopWindowID();
    
    // ������ ����Ʈ�� ���� �տ� �߰��Ͽ� �ֻ��� ������� ����
    kAddListToHeader( &gs_stWindowManager.stWindowList, pstWindow );

    // ����ȭ ó��
    kUnlock( &( gs_stWindowManager.stLock ) );

    //--------------------------------------------------------------------------
    // ������ �̺�Ʈ ����
    //--------------------------------------------------------------------------
    // ������ ������ŭ ȭ�鿡 ������Ʈ�ϰ� ���õǾ��ٴ� �̺�Ʈ�� ����
    kUpdateScreenByID( pstWindow->stLink.qwID );
    kSetWindowEvent( pstWindow->stLink.qwID, EVENT_WINDOW_SELECT, &stEvent );
    kSendEventToWindow( pstWindow->stLink.qwID, &stEvent );
    
    // ������ �ֻ��� �����찡 ��� �����찡 �ƴϸ� ���� �ֻ��� �������� ���� ǥ������
    // ���õ��� ���� ������ ������Ʈ�ϰ� ���� �����Ǿ��ٴ� �̺�Ʈ�� ����
    if( qwActiveWindowID != gs_stWindowManager.qwBackgoundWindowID )
    {
        kUpdateWindowTitle( qwActiveWindowID, FALSE );
        kSetWindowEvent( qwActiveWindowID, EVENT_WINDOW_DESELECT, &stEvent );
        kSendEventToWindow( qwActiveWindowID, &stEvent );
    }    
    return pstWindow->stLink.qwID;
}

/**
 *  �����츦 ����
 *      ������ ���� �̺�Ʈ�� ���� ����
 */
BOOL kDeleteWindow( QWORD qwWindowID )
{
    WINDOW* pstWindow;
    RECT stArea;
    QWORD qwActiveWindowID;
    BOOL bActiveWindow;
    EVENT stEvent;

    // ����ȭ ó��
    kLock( &( gs_stWindowManager.stLock ) );
    
    // ������ �˻�
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        // ����ȭ ó��
        kUnlock( &( gs_stWindowManager.stLock ) );
        return FALSE;
    }

    // �����츦 �����ϱ� ���� ������ �����ص�
    kMemCpy( &stArea, &( pstWindow->stArea ), sizeof( RECT ) );

    // ������ ����Ʈ���� ������ ������, �� �ֻ��� �����츦 ��ȯ
    qwActiveWindowID = kGetTopWindowID();

    // �ֻ��� �����찡 �������� ��� �÷��׸� ����
    if( qwActiveWindowID == qwWindowID )
    {
        bActiveWindow = TRUE;
    }
    else
    {
        bActiveWindow = FALSE;
    }
    
    // ������ ����Ʈ���� ������ ����
    if( kRemoveList( &( gs_stWindowManager.stWindowList ), qwWindowID ) == NULL )
    {
        // ����ȭ ó��
        kUnlock( &( pstWindow->stLock ) );
        kUnlock( &( gs_stWindowManager.stLock ) );
        return FALSE;
    }

    //--------------------------------------------------------------------------
    // ������ ȭ�� ���ۿ� �̺�Ʈ ť ���۸� ��ȯ
    //--------------------------------------------------------------------------
    // ������ ȭ�� ���۸� ��ȯ
    kFreeMemory( pstWindow->pstWindowBuffer );
    pstWindow->pstWindowBuffer = NULL;
    
    // ������ �̺�Ʈ ť ���۸� ��ȯ
    kFreeMemory( pstWindow->pstEventBuffer );
    pstWindow->pstEventBuffer = NULL;

    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );

    // ������ �ڷᱸ���� ��ȯ
    kFreeWindow( qwWindowID );

    // ����ȭ ó��
    kUnlock( &( gs_stWindowManager.stLock ) );

    // �����Ǳ� ���� �����찡 �ִ� ������ ȭ�鿡 �ٽ� ������Ʈ
    kUpdateScreenByScreenArea( &stArea );
    
    //--------------------------------------------------------------------------
    // �ֻ��� �����찡 �������ٸ� ���� ����Ʈ���� �ֻ����� �ִ� �����츦 Ȱ��ȭ�ϰ�
    // ���õǾ��ٴ� ������ �̺�Ʈ�� ����
    //--------------------------------------------------------------------------
    if( bActiveWindow == TRUE )
    {
        // ������ ����Ʈ���� ������ ������, �� �ֻ��� �����츦 ��ȯ
        qwActiveWindowID = kGetTopWindowID();
        
        // �ֻ��� �������� ���� ǥ������ Ȱ��ȭ�� ���·� ǥ��
        if( qwActiveWindowID != WINDOW_INVALIDID )
        {
            kUpdateWindowTitle( qwActiveWindowID, TRUE );
            
            kSetWindowEvent( qwActiveWindowID, EVENT_WINDOW_SELECT, &stEvent );
            kSendEventToWindow( qwActiveWindowID, &stEvent );   
        }
    }
    return TRUE;
}

/**
 *  �½�ũ ID�� ��ġ�ϴ� ��� �����츦 ����
 */
BOOL kDeleteAllWindowInTaskID( QWORD qwTaskID )
{
    WINDOW* pstWindow;
    WINDOW* pstNextWindow;

    // ����ȭ ó��
    kLock( &( gs_stWindowManager.stLock ) );

    // ����Ʈ���� ù ��° �����츦 ��ȯ
    pstWindow = kGetHeaderFromList( &( gs_stWindowManager.stWindowList ) );
    while( pstWindow != NULL )
    {
        // ���� �����츦 �̸� ����
        pstNextWindow = kGetNextFromList( &( gs_stWindowManager.stWindowList ),
                pstWindow );

        // ��� �����찡 �ƴϰ� �½�ũ ID�� ��ġ�ϸ� ������ ����
        if( ( pstWindow->stLink.qwID != gs_stWindowManager.qwBackgoundWindowID ) &&
            ( pstWindow->qwTaskID == qwTaskID ) )
        {
            kDeleteWindow( pstWindow->stLink.qwID );
        }

        // �̸� ���ص� ���� �������� ���� ����
        pstWindow = pstNextWindow;
    }

    // ����ȭ ó��
    kUnlock( &( gs_stWindowManager.stLock ) );
}

/**
 *  ������ ID�� ������ �����͸� ��ȯ
 */
WINDOW* kGetWindow( QWORD qwWindowID )
{
    WINDOW* pstWindow;

    // ������ ID�� ��ȿ ���� �˻�
    if( GETWINDOWOFFSET( qwWindowID ) >= WINDOW_MAXCOUNT )
    {
        return NULL;
    }

    // ID�� ������ �����͸� ã�� �� ID�� ��ġ�ϸ� ��ȯ
    pstWindow = &gs_stWindowPoolManager.pstStartAddress[ GETWINDOWOFFSET( qwWindowID )];
    if( pstWindow->stLink.qwID == qwWindowID )
    {
        return pstWindow;
    }

    return NULL;
}

/**
 *  ������ ID�� ������ �����͸� ã�� ������ ���ؽ��� ��� �� ��ȯ
 */
WINDOW* kGetWindowWithWindowLock( QWORD qwWindowID )
{
    WINDOW* pstWindow;
    BOOL bResult;

    // �����츦 �˻�
    pstWindow = kGetWindow( qwWindowID );
    if( pstWindow == NULL )
    {
        return NULL;
    }
    
    // ����ȭ ó���� �� �ٽ� ������ ID�� ������ �˻�
    kLock( &(pstWindow->stLock ) );
    // ������ ����ȭ�� �� �ڿ� ������ ID�� �����츦 �˻��� �� ���ٸ� ���߿� �����찡
    // �ٲ� ���̹Ƿ� NULL ��ȯ
    pstWindow = kGetWindow( qwWindowID );
    if( ( pstWindow == NULL ) || ( pstWindow->pstEventBuffer == NULL ) ||
        ( pstWindow->pstWindowBuffer == NULL ) )                
    {
        // ����ȭ ó��
        kUnlock( &(pstWindow->stLock ) );
        return NULL;
    }
    
    return pstWindow;
}

/**
 *  �����츦 ȭ�鿡 ��Ÿ���ų� ����
 */
BOOL kShowWindow( QWORD qwWindowID, BOOL bShow )
{
    WINDOW* pstWindow;
    RECT stWindowArea;

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // ������ �Ӽ� ����
    if( bShow == TRUE )
    {
        pstWindow->dwFlags |= WINDOW_FLAGS_SHOW;
    }
    else
    {
        pstWindow->dwFlags &= ~WINDOW_FLAGS_SHOW;
    }

    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );
    
    // �����찡 �ִ� ������ �ٽ� ������Ʈ�����ν� �����츦 ��Ÿ���ų� ����
    if( bShow == TRUE )
    {
        kUpdateScreenByID( qwWindowID );
    }
    else
    {
        kGetWindowArea( qwWindowID, &stWindowArea );
        kUpdateScreenByScreenArea( &stWindowArea );
    }
    return TRUE;
}

/**
 *  Ư�� ������ �����ϴ� ������� ��� �׸�
 *      ������ �Ŵ����� ȣ���ϴ� �Լ�, ������ kUpdateScreen()�� �Լ��� ���
 */
BOOL kRedrawWindowByArea( const RECT* pstArea, QWORD qwDrawWindowID )
{
    WINDOW* pstWindow;
    WINDOW* pstTargetWindow = NULL;
    RECT stOverlappedArea;
    RECT stCursorArea;
    DRAWBITMAP stDrawBitmap;
    RECT stTempOverlappedArea;
    RECT vstLargestOverlappedArea[ WINDOW_OVERLAPPEDAREALOGMAXCOUNT ];
    int viLargestOverlappedAreaSize[ WINDOW_OVERLAPPEDAREALOGMAXCOUNT ];
    int iTempOverlappedAreaSize;
    int iMinAreaSize;
    int iMinAreaIndex;
    int i;

    // ȭ�� ������ ��ġ�� ������ ������ �׸� �ʿ䰡 ����
    if( kGetOverlappedRectangle( &( gs_stWindowManager.stScreenArea ), pstArea,
            &stOverlappedArea ) == FALSE )
    {
        return FALSE;
    }

    //--------------------------------------------------------------------------
    // Z ������ �ֻ���, �� ������ ����Ʈ�� ù ��°���� ���������� ������ ���鼭 
    // ������Ʈ�� ������ ��ġ�� �����츦 ã�� ���� �޸𸮷� ����
    //--------------------------------------------------------------------------
    // ȭ�鿡 ������Ʈ�� ������ ����� ������ �ʱ�ȭ
    kMemSet( viLargestOverlappedAreaSize, 0, sizeof( viLargestOverlappedAreaSize ) );
    kMemSet( vstLargestOverlappedArea, 0, sizeof( vstLargestOverlappedArea ) );

    // ����ȭ ó��
    kLock( &( gs_stWindowManager.stLock ) );

    // ȭ�鿡 ������Ʈ�� ������ �����ϴ� ��Ʈ���� ����
    kCreateDrawBitmap( &stOverlappedArea, &stDrawBitmap );

    // ���� ������ ����Ʈ�� Z ������ ���� ���� �Ǿ� �����Ƿ�, ������ ����Ʈ�� ó������
    // ���󰡸鼭 ������Ʈ�� ������ �����ϴ� �����츦 ã�� �Ʒ��� �������� ȭ�鿡 ����
    pstWindow = kGetHeaderFromList( &( gs_stWindowManager.stWindowList ) );
    while( pstWindow != NULL )
    {
        // �����츦 ȭ�鿡 ��Ÿ���� �ɼ��� �����Ǿ�������,
        // ������Ʈ�� �κа� �����찡 �����ϴ� ������ ��ġ�� ��ġ�� ��ŭ�� ȭ�鿡 ����
        if( ( pstWindow->dwFlags & WINDOW_FLAGS_SHOW ) &&
            ( kGetOverlappedRectangle( &( pstWindow->stArea ), &stOverlappedArea,
                                       &stTempOverlappedArea ) == TRUE ) )
        {
            // ������� ��ġ�� ������ ���̸� ���
            iTempOverlappedAreaSize = kGetRectangleWidth( &stTempOverlappedArea ) * 
                kGetRectangleHeight( &stTempOverlappedArea );
            
            // ������ ����� ������ ������Ʈ ������ �˻��Ͽ� ���� ������ ���ԵǴ��� Ȯ��
            for( i = 0 ; i < WINDOW_OVERLAPPEDAREALOGMAXCOUNT ; i++ )
            {
                // ��ġ�� ������ ������ ������ ������ ���Ͽ� ȭ���� ������Ʈ���� 
                // ���θ� ����
                // ������ ������Ʈ�� ���� �߿��� ū ������ ���ԵǸ� ������Ʈ���� ����
                if( ( iTempOverlappedAreaSize <= viLargestOverlappedAreaSize[ i ] ) &&
                    ( kIsInRectangle( &( vstLargestOverlappedArea[ i ] ), 
                      stTempOverlappedArea.iX1, stTempOverlappedArea.iY1 ) == TRUE ) &&
                    ( kIsInRectangle( &( vstLargestOverlappedArea[ i ] ), 
                      stTempOverlappedArea.iX2, stTempOverlappedArea.iY2 ) == TRUE ) )
                {
                    break;
                }
            }
            
            // ��ġ�ϴ� ������Ʈ ������ ã�Ҵٸ� ������ ������Ʈ �Ǿ��ٴ� ���̹Ƿ�
            // ���� ������� �̵�        
            if( i < WINDOW_OVERLAPPEDAREALOGMAXCOUNT )
            {
                // ���� �����츦 ã��
                pstWindow = kGetNextFromList( &( gs_stWindowManager.stWindowList ),
                        pstWindow );
                continue;
            }           
            
            // ���� ������ ������ ������Ʈ�ߴ� ���� ū ������ ������ ���Ե��� �ʴ´ٸ�
            // ���̸� ���ؼ� ������ ������Ʈ�� ���� �� ���� ���� ������ �˻�
            iMinAreaSize = 0xFFFFFF;
            iMinAreaIndex = 0;
            for( i = 0 ; i < WINDOW_OVERLAPPEDAREALOGMAXCOUNT ; i++ )
            {
                if( viLargestOverlappedAreaSize[ i ] < iMinAreaSize )
                {
                    iMinAreaSize = viLargestOverlappedAreaSize[ i ];
                    iMinAreaIndex = i;
                }
            }
            
            // ����� ���� �߿� �ּ� ũ�⺸�� ���� ��ġ�� ������ ���̰� ũ�� �ش� ��ġ��
            // ��ü�Ͽ� ���� ũ�� ������ 10���� ����
            if( iMinAreaSize < iTempOverlappedAreaSize )
            {
                kMemCpy( &( vstLargestOverlappedArea[ iMinAreaIndex ] ), 
                         &stTempOverlappedArea, sizeof( RECT ) );
                viLargestOverlappedAreaSize[ iMinAreaIndex ] = iTempOverlappedAreaSize;
            }
            
            // ����ȭ ó��
            kLock( &( pstWindow->stLock ) );

            // ������ ID�� ��ȿ�ϴٸ� �� ������ ������� ȭ�鿡 �׸��� �ʰ� 
            // ������Ʈ ��Ʈ�ʸ� ������Ʈ �� ������ ����
            if( ( qwDrawWindowID != WINDOW_INVALIDID ) &&
                ( qwDrawWindowID != pstWindow->stLink.qwID ) )
            {
                // ��Ʈ�ʸ� ������Ʈ
                kFillDrawBitmap( &stDrawBitmap, &( pstWindow->stArea ), FALSE );
            }
            else
            {
                // ������ ȭ�� ���۸� ���� �޸𸮷� ����
                kCopyWindowBufferToFrameBuffer( pstWindow, &stDrawBitmap );
            }
            // ����ȭ ó��
            kUnlock( &( pstWindow->stLock ) );
        }
        
        // ��� ������ �� ������Ʈ �Ǿ����� �� �̻� �׸� �ʿ� ����
        if( kIsDrawBitmapAllOff( &stDrawBitmap ) == TRUE )
        {
            break;
        }
        
        // ���� �����츦 ã��
        pstWindow = kGetNextFromList( &( gs_stWindowManager.stWindowList ),
                pstWindow );
    }

    // ����ȭ ó��
    kUnlock( &( gs_stWindowManager.stLock ) );

    //--------------------------------------------------------------------------
    // ���콺 Ŀ�� ������ ���ԵǸ� ���콺 Ŀ���� ���� �׸�
    //--------------------------------------------------------------------------
    // ���콺 ������ RECT �ڷᱸ���� ����
    kSetRectangleData( gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY,
            gs_stWindowManager.iMouseX + MOUSE_CURSOR_WIDTH,
            gs_stWindowManager.iMouseY + MOUSE_CURSOR_HEIGHT, &stCursorArea );
    
    // ��ġ���� Ȯ���Ͽ� ��ģ�ٸ� ���콺 Ŀ���� �׸�
    if( kIsRectangleOverlapped( &stOverlappedArea, &stCursorArea ) == TRUE )
    {
        kDrawCursor( gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY );
    }
}

/**
 *  ������ ȭ�� ������ �Ϻ� �Ǵ� ��ü�� ������ ���۷� ����
 */
static void kCopyWindowBufferToFrameBuffer( const WINDOW* pstWindow,
        DRAWBITMAP* pstDrawBitmap )
{
    RECT stTempArea;
    RECT stOverlappedArea;
    int iOverlappedWidth;
    int iOverlappedHeight;
    int iScreenWidth;
    int iWindowWidth;
    int i;
    COLOR* pstCurrentVideoMemoryAddress;
    COLOR* pstCurrentWindowBufferAddress;
    BYTE bTempBitmap;
    int iByteOffset;
    int iBitOffset;
    int iOffsetX;
    int iOffsetY;
    int iLastBitOffset;
    int iBulkCount;

    // �����ؾ� �ϴ� ������ ȭ�� ������ ��ġ�� �κ��� �ӽ÷� ���
    if( kGetOverlappedRectangle( &( gs_stWindowManager.stScreenArea ), 
            &( pstDrawBitmap->stArea ), &stTempArea ) == FALSE )
    {
        return ;
    }

    // ������ ������ �ӽ÷� ����� ������ ��ġ�� �κ��� �ٽ� ���
    // �� ������ ��ġ�� �ʴ´ٸ� ���� �޸𸮷� ������ �ʿ� ����
    if( kGetOverlappedRectangle( &stTempArea, &( pstWindow->stArea ),
            &stOverlappedArea ) == FALSE )
    {
        return ;
    }

    // �� ������ �ʺ�� ���̸� ���
    iScreenWidth = kGetRectangleWidth( &( gs_stWindowManager.stScreenArea ) );
    iWindowWidth = kGetRectangleWidth( &( pstWindow->stArea ) );
    iOverlappedWidth = kGetRectangleWidth( &stOverlappedArea );
    iOverlappedHeight = kGetRectangleHeight( &stOverlappedArea );

    //--------------------------------------------------------------------------
    // ��ġ�� ������ ���̸�ŭ ����ϴ� ������ �ݺ�
    //--------------------------------------------------------------------------
    for( iOffsetY = 0 ; iOffsetY < iOverlappedHeight ; iOffsetY++ )
    {
        // ��ġ�� ������ ȭ�� ������Ʈ ��Ʈ�ʿ��� �����ϴ� ��ġ�� ���
        if( kGetStartPositionInDrawBitmap( pstDrawBitmap, stOverlappedArea.iX1,
                stOverlappedArea.iY1 + iOffsetY, &iByteOffset, &iBitOffset ) == FALSE )
        {
            break ;
        }

        // ������ ������ ���� �޸� ��巹���� ������ ȭ�� ������ ��巹���� ���
        pstCurrentVideoMemoryAddress = gs_stWindowManager.pstVideoMemory +
            ( stOverlappedArea.iY1 + iOffsetY )* iScreenWidth + stOverlappedArea.iX1;

        // ������ ȭ�� ���۴� ȭ�� ��ü�� �ƴ� �����츦 �������� �� ��ǥ�̹Ƿ�,
        // ��ġ�� ������ ������ ���� ��ǥ �������� ��ȯ
        pstCurrentWindowBufferAddress = pstWindow->pstWindowBuffer +
            ( stOverlappedArea.iY1 - pstWindow->stArea.iY1 + iOffsetY ) * iWindowWidth +
            ( stOverlappedArea.iX1 - pstWindow->stArea.iX1 );
        
        //----------------------------------------------------------------------
        // ��ģ ������ �ʺ�ŭ ����ϴ� ������ �ݺ�
        //----------------------------------------------------------------------
        for( iOffsetX = 0 ; iOffsetX < iOverlappedWidth ; )
        {        
            // 8���� �ȼ��� �ѹ��� ������Ʈ�� �� ������ 8�ȼ� ������ ó���� �� �ִ�
            // ũ�⸦ ����Ͽ� �ѹ��� ó��
            if( ( pstDrawBitmap->pbBitmap[ iByteOffset ] == 0xFF ) && 
                ( iBitOffset == 0x00 ) &&
                ( ( iOverlappedWidth - iOffsetX ) >= 8 ) )
            {
                // ���� ��ġ���� 8�ȼ� ������ ó���� �� �ִ� �ִ� ũ�⸦ ���
                for( iBulkCount = 0 ; 
                     ( iBulkCount < ( ( iOverlappedWidth - iOffsetX ) >> 3 ) ) ;
                     iBulkCount++ )
                {
                    if( pstDrawBitmap->pbBitmap[ iByteOffset + iBulkCount ] != 0xFF )
                    {
                        break;
                    }
                }
                
                // 8�ȼ� ������ �ѹ��� ó��
                kMemCpy( pstCurrentVideoMemoryAddress, pstCurrentWindowBufferAddress,
                        ( sizeof( COLOR ) * iBulkCount ) << 3 );                                

                // �޸� ��巹���� ��Ʈ�� ������ 8�ȼ� ������ ������Ʈ                
                pstCurrentVideoMemoryAddress += iBulkCount << 3;
                pstCurrentWindowBufferAddress += iBulkCount << 3;
                kMemSet( pstDrawBitmap->pbBitmap + iByteOffset, 0x00, 
                        iBulkCount );
                
                // ��ü �������� 8�ȼ� ������ ������ ����ŭ ���� ������
                iOffsetX += iBulkCount << 3;
                
                // ��Ʈ���� �������� ����
                iByteOffset += iBulkCount;
                iBitOffset = 0;
            }
            // ���� ������ �̹� ������Ʈ �Ǿ� 8���� �ȼ��� �ѹ��� ������ �� ������
            // 8�ȼ� ������ ó���� �� �ִ� ũ�⸦ ����Ͽ� �ѹ��� ó��
            else if( ( pstDrawBitmap->pbBitmap[ iByteOffset ] == 0x00 ) && 
                ( iBitOffset == 0x00 ) &&
                ( ( iOverlappedWidth - iOffsetX ) >= 8 ) )
            {
                // ���� ��ġ���� 8�ȼ� ������ ó���� �� �ִ� �ִ� ũ�⸦ ���
                for( iBulkCount = 0 ; 
                     ( iBulkCount < ( ( iOverlappedWidth - iOffsetX ) >> 3 ) ) ;
                     iBulkCount++ )
                {
                    if( pstDrawBitmap->pbBitmap[ iByteOffset + iBulkCount ] != 0x00 )
                    {
                        break;
                    }
                }
                
                // �޸� ��巹���� ����� ������ ������Ʈ                
                pstCurrentVideoMemoryAddress += iBulkCount << 3;
                pstCurrentWindowBufferAddress += iBulkCount << 3;
                
                // ��ü �������� 8�ȼ� ������ ������ ����ŭ ���� ������
                iOffsetX += iBulkCount << 3;
                
                // ��Ʈ���� �������� ����
                iByteOffset += iBulkCount;
                iBitOffset = 0;                
            }
            else
            {
                // ���� ������Ʈ�� ��ġ�� ��Ʈ��
                bTempBitmap = pstDrawBitmap->pbBitmap[ iByteOffset ];

                // ���� ��Ʈ�ʿ��� ����ؾ� �� ������ �ȼ��� ��Ʈ �������� ����
                iLastBitOffset = MIN( 8, iOverlappedWidth - iOffsetX + iBitOffset );
                
                // �� ���� �̵���Ŵ
                for( i = iBitOffset ; i < iLastBitOffset ; i++ )
                {
                    // ��Ʈ���� 1�� �����Ǿ� ������ ȭ�鿡 ����ϰ� �ش� ��Ʈ�� 0���� ����
                    if( bTempBitmap & ( 0x01 << i ) )
                    {
                        *pstCurrentVideoMemoryAddress = *pstCurrentWindowBufferAddress;
                        
                        // ��Ʈ�� ������ ����� ������ ������Ʈ
                        bTempBitmap &= ~( 0x01 << i );
                    }
                    
                    // �޸� ��巹���� ����� ������ ������Ʈ
                    pstCurrentVideoMemoryAddress++;
                    pstCurrentWindowBufferAddress++;
                }
                
                // ��ü �������� 1�ȼ� ������ ������ ����ŭ�� ���� ������
                iOffsetX += ( iLastBitOffset - iBitOffset );
                
                // ��Ʈ�� ������ ����� ������ ������Ʈ
                pstDrawBitmap->pbBitmap[ iByteOffset ] = bTempBitmap;
                iByteOffset++;
                iBitOffset = 0;
            }
        }
    }
}

/**
 *  Ư�� ��ġ�� �����ϴ� ������ �߿��� ���� ���� �ִ� �����츦 ��ȯ
 */
QWORD kFindWindowByPoint( int iX, int iY )
{
    QWORD qwWindowID;
    WINDOW* pstWindow;
    
    // ���콺�� ��� �����츦 ����� ���ϹǷ�, �⺻ ���� ��� ������� ����
    qwWindowID = gs_stWindowManager.qwBackgoundWindowID;
    
    // ����ȭ ó��
    kLock( &( gs_stWindowManager.stLock ) );
    
    // �ֻ��� ��������� �˻� ����
    pstWindow = kGetHeaderFromList( &( gs_stWindowManager.stWindowList ) );
    do
    {
        // �����찡 ȭ�鿡 ���̰� �����찡 X, Y ��ǥ�� �����Ѵٸ� ������ ID ������Ʈ
        if( ( pstWindow->dwFlags & WINDOW_FLAGS_SHOW ) &&
            ( kIsInRectangle( &( pstWindow->stArea ), iX, iY ) == TRUE ) )
        {
            qwWindowID = pstWindow->stLink.qwID;
            break;
        }
        
        // ���� �����츦 ��ȯ
        pstWindow = kGetNextFromList( &( gs_stWindowManager.stWindowList ), pstWindow );
    } while( pstWindow != NULL );
    
    // ����ȭ ó��
    kUnlock( &( gs_stWindowManager.stLock ) );
    return qwWindowID;
}

/**
 *  ������ ������ ��ġ�ϴ� �����츦 ��ȯ
 */
QWORD kFindWindowByTitle( const char* pcTitle )
{
    QWORD qwWindowID;
    WINDOW* pstWindow;
    int iTitleLength;

    qwWindowID = WINDOW_INVALIDID;
    iTitleLength = kStrLen( pcTitle );
    
    // ����ȭ ó��
    kLock( &( gs_stWindowManager.stLock ) );
    
    // ��� ��������� �˻� ����
    pstWindow = kGetHeaderFromList( &( gs_stWindowManager.stWindowList ) );
    while( pstWindow != NULL )
    {
        // ������ ������ ��ġ�ϸ� ������ ������ ��ġ�ϴ� �������� ID�� ��ȯ
        if( ( kStrLen( pstWindow->vcWindowTitle ) == iTitleLength ) &&
            ( kMemCmp( pstWindow->vcWindowTitle, pcTitle, iTitleLength ) == 0 ) )
        {
            qwWindowID = pstWindow->stLink.qwID;
            break;
        }
        
        // ���� �����츦 ��ȯ
        pstWindow = kGetNextFromList( &( gs_stWindowManager.stWindowList ), 
                                      pstWindow );
    }
    
    // ����ȭ ó��
    kUnlock( &( gs_stWindowManager.stLock ) );
    return qwWindowID;
}

/**
 *  �����찡 �����ϴ��� ���θ� ��ȯ
 */
BOOL kIsWindowExist( QWORD qwWindowID )
{
    // �����츦 �˻��� ����� NULL�̶�� �����찡 �������� ����
    if( kGetWindow( qwWindowID ) == NULL )
    {
        return FALSE;
    }
    return TRUE;
}

/**
 *  �ֻ��� �������� ID�� ��ȯ
 */
QWORD kGetTopWindowID( void )
{
    WINDOW* pstActiveWindow;
    QWORD qwActiveWindowID;
    
    // ����ȭ ó��
    kLock( &( gs_stWindowManager.stLock ) );
    
    // ������ ����Ʈ�� ���� �տ� �ִ� �����츦 ��ȯ
    pstActiveWindow = ( WINDOW* ) kGetHeaderFromList( &( gs_stWindowManager.stWindowList ) );
    if( pstActiveWindow != NULL )
    {
        qwActiveWindowID = pstActiveWindow->stLink.qwID;
    }
    else
    {
        qwActiveWindowID = WINDOW_INVALIDID;
    }
    
    // ����ȭ ó��
    kUnlock( &( gs_stWindowManager.stLock ) );
    
    return qwActiveWindowID;
}

/**
 *  �������� Z ������ �ֻ����� ����
 *      �����츦 �ֻ����� �̵���Ŵ�� ���ÿ� ������ ���ð� ���� ���� �̺�Ʈ�� ����
 */
BOOL kMoveWindowToTop( QWORD qwWindowID )
{
    WINDOW* pstWindow;
    RECT stArea;
    DWORD dwFlags;
    QWORD qwTopWindowID;
    EVENT stEvent;
    
    // ���� ������ ����Ʈ���� �ֻ��� ������, �� ���õ� �������� ID�� ��ȯ
    qwTopWindowID = kGetTopWindowID();    
    // �ֻ��� �����찡 �ڽ��̸� �� ������ �ʿ� ����
    if( qwTopWindowID == qwWindowID )
    {
        return TRUE;
    }
    
    // ����ȭ ó��
    kLock( &( gs_stWindowManager.stLock ) );
    
    // ������ ����Ʈ���� �����Ͽ� ������ ����Ʈ�� ���� ������ �̵�
    pstWindow = kRemoveList( &( gs_stWindowManager.stWindowList ), qwWindowID );
    if( pstWindow != NULL )
    {
        kAddListToHeader( &( gs_stWindowManager.stWindowList ), pstWindow );
        
        // �������� ������ ������ ���� ��ǥ�� ��ȯ�Ͽ� �÷��׿� �Բ� �����ص�
        // �Ʒ����� ������ ȭ���� ������Ʈ�� �� ���
        kConvertRectScreenToClient( qwWindowID, &( pstWindow->stArea ), &stArea );
        dwFlags = pstWindow->dwFlags;
    }
    
    // ����ȭ ó��
    kUnlock( &( gs_stWindowManager.stLock ) );
    
    // �����찡 �ֻ����� �̵��ߴٸ� ������ ���� ǥ������ ȭ�鿡 ������Ʈ�ϰ�
    // ����/���� ���� ������ �̺�Ʈ�� ���� ����
    if( pstWindow != NULL )
    {
        //----------------------------------------------------------------------
        // ���õ� �������� ȭ���� ������Ʈ�ϰ� ������ �̺�Ʈ ����
        //----------------------------------------------------------------------
        // ���õǾ��ٴ� �̺�Ʈ�� ����
        kSetWindowEvent( qwWindowID, EVENT_WINDOW_SELECT, &stEvent );
        kSendEventToWindow( qwWindowID, &stEvent );   
        // ���� ǥ������ �ִٸ� ���� �������� ���� ǥ������ ���õ� ������ ����� 
        // ȭ�� ������Ʈ
        if( dwFlags & WINDOW_FLAGS_DRAWTITLE )
        {
            // ������ ���� ǥ������ ���õ� ���·� ������Ʈ
            kUpdateWindowTitle( qwWindowID, TRUE );
            // ���� ǥ������ ������ �ٽ� �׷����Ƿ� ���� ǥ������ ������ ������ �κи�
            // ȭ�� ������Ʈ ����
            stArea.iY1 += WINDOW_TITLEBAR_HEIGHT;
            kUpdateScreenByWindowArea( qwWindowID, &stArea );
        }
        // ���� ǥ������ ���ٸ� ������ ���� ��ü�� ������Ʈ
        else
        {
            kUpdateScreenByID( qwWindowID );
        }
        
        //----------------------------------------------------------------------
        // ������ Ȱ��ȭ�Ǿ��� ������� ���� ǥ������ ��Ȱ��ȭ�� ����� ���� �����Ǿ��ٴ�
        // �̺�Ʈ�� ����
        //----------------------------------------------------------------------
        // ���� �����Ǿ��ٴ� �̺�Ʈ�� ����
        kSetWindowEvent( qwTopWindowID, EVENT_WINDOW_DESELECT, &stEvent );
        kSendEventToWindow( qwTopWindowID, &stEvent );   
        // ���� ǥ������ ���õ��� ���� ���·� ������Ʈ
        kUpdateWindowTitle( qwTopWindowID, FALSE );
        return TRUE;
    }
    
    return FALSE;
}

/**
 *  X, Y��ǥ�� �������� ���� ǥ���� ��ġ�� �ִ����� ��ȯ
 */
BOOL kIsInTitleBar( QWORD qwWindowID, int iX, int iY )
{
    WINDOW* pstWindow;
    
    // ������ �˻�
    pstWindow = kGetWindow( qwWindowID );
    
    // �����찡 ���ų� �����찡 ���� ǥ������ ������ ���� ������ ó���� �ʿ䰡 ����
    if( ( pstWindow == NULL ) ||
        ( ( pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE ) == 0 ) )
    {
        return FALSE;
    }
    
    // ��ǥ�� ���� ǥ���� ������ �ִ����� ��
    if( ( pstWindow->stArea.iX1 <= iX ) && ( iX <= pstWindow->stArea.iX2 ) &&
        ( pstWindow->stArea.iY1 <= iY ) && 
        ( iY <= pstWindow->stArea.iY1 + WINDOW_TITLEBAR_HEIGHT ) )
    {
        return TRUE;
    }
    
    return FALSE;
}

/**
 *  X, Y��ǥ�� �������� �ݱ� ��ư ��ġ�� �ִ����� ��ȯ
 */
BOOL kIsInCloseButton( QWORD qwWindowID, int iX, int iY )
{
    WINDOW* pstWindow;
    
    // �����츦 �˻�
    pstWindow = kGetWindow( qwWindowID );
    
    // �����찡 ���ų� �����찡 ���� ǥ������ ������ ���� ������ ó���� �ʿ䰡 ����
    if( ( pstWindow == NULL ) &&
        ( ( pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE ) == 0 ) )
    {
        return FALSE;
    }
    
    // ��ǥ�� �ݱ� ��ư ������ �ִ����� ��
    if( ( ( pstWindow->stArea.iX2 - WINDOW_XBUTTON_SIZE - 1 ) <= iX ) && 
          ( iX <= ( pstWindow->stArea.iX2 - 1 ) ) &&
          ( ( pstWindow->stArea.iY1 + 1 ) <= iY ) && 
          ( iY <= ( pstWindow->stArea.iY1 + 1 + WINDOW_XBUTTON_SIZE ) ) )
    {
        return TRUE;
    }
    
    return FALSE;
}

/**
 *  �����츦 �ش� ��ġ�� �̵�
 */
BOOL kMoveWindow( QWORD qwWindowID, int iX, int iY )
{
    WINDOW* pstWindow;
    RECT stPreviousArea;
    int iWidth;
    int iHeight;
    EVENT stEvent;
    
    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // ������ �����찡 �ִ� ���� ����
    kMemCpy( &stPreviousArea, &( pstWindow->stArea ), sizeof( RECT ) );

    // ���̿� �ʺ� ����Ͽ� ���� ������ ��ġ�� ����
    iWidth = kGetRectangleWidth( &stPreviousArea );
    iHeight = kGetRectangleHeight( &stPreviousArea );
    kSetRectangleData( iX, iY, iX + iWidth - 1, iY + iHeight - 1, 
            &( pstWindow->stArea ) );

    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );
    
    // ���� �����찡 �ִ� ȭ�� ������ ������Ʈ
    kUpdateScreenByScreenArea( &stPreviousArea );

    // ���� ������ ������ ȭ�鿡 ������Ʈ
    kUpdateScreenByID( qwWindowID );
    
    // ������ �̵� �޽����� ����
    kSetWindowEvent( qwWindowID, EVENT_WINDOW_MOVE, &stEvent );
    kSendEventToWindow( qwWindowID, &stEvent );

    return TRUE;
}

/**
 *  ������ ���� ǥ������ ���� �׸�
 */
static BOOL kUpdateWindowTitle( QWORD qwWindowID, BOOL bSelectedTitle )
{
    WINDOW* pstWindow;
    RECT stTitleBarArea;
    
    // �����츦 �˻�
    pstWindow = kGetWindow( qwWindowID );

    // �����찡 �����ϸ� �������� ���� ǥ������ �ٽ� �׷��� ȭ�鿡 ������Ʈ    
    if( ( pstWindow != NULL ) &&
        ( pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE ) )
    {
        // ����/���� ���� ���ο� ���� ������ ���� ǥ������ �ٽ� �׸�
        kDrawWindowTitle( pstWindow->stLink.qwID, pstWindow->vcWindowTitle,
                          bSelectedTitle );
        // ������ ���� ǥ������ ��ġ�� ������ ���� ��ǥ�� ����
        stTitleBarArea.iX1 = 0;
        stTitleBarArea.iY1 = 0;
        stTitleBarArea.iX2 = kGetRectangleWidth( &( pstWindow->stArea ) ) - 1;
        stTitleBarArea.iY2 = WINDOW_TITLEBAR_HEIGHT;

        // ������ ������ŭ ȭ�鿡 ������Ʈ
        kUpdateScreenByWindowArea( qwWindowID, &stTitleBarArea );
        
        return TRUE;
    }
    
    return FALSE;
}

//==============================================================================
//  ȭ�� ��ǥ <--> ������ ��ǥ ��ȯ ���� �Լ�
//==============================================================================
/**
 *  ������ ������ ��ȯ
 */
BOOL kGetWindowArea( QWORD qwWindowID, RECT* pstArea )
{
    WINDOW* pstWindow;
    
    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }

    // ������ ������ �����ؼ� ��ȯ
    kMemCpy( pstArea, &( pstWindow->stArea ), sizeof( RECT ) );
    
    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );
    return TRUE;
}

/**
 *  ��ü ȭ���� �������� �� X,Y ��ǥ�� ������ ���� ��ǥ�� ��ȯ
 */
BOOL kConvertPointScreenToClient( QWORD qwWindowID, const POINT* pstXY, 
        POINT* pstXYInWindow )
{
    RECT stArea;
    
    // ������ ������ ��ȯ
    if( kGetWindowArea( qwWindowID, &stArea ) == FALSE )
    {
        return FALSE;
    }
    
    pstXYInWindow->iX = pstXY->iX - stArea.iX1;
    pstXYInWindow->iY = pstXY->iY - stArea.iY1;
    return TRUE;
}

/**
 *  ������ ���θ� �������� �� X,Y ��ǥ�� ȭ�� ��ǥ�� ��ȯ
 */
BOOL kConvertPointClientToScreen( QWORD qwWindowID, const POINT* pstXY, 
        POINT* pstXYInScreen )
{
    RECT stArea;
    
    // ������ ������ ��ȯ
    if( kGetWindowArea( qwWindowID, &stArea ) == FALSE )
    {
        return FALSE;
    }
    
    pstXYInScreen->iX = pstXY->iX + stArea.iX1;
    pstXYInScreen->iY = pstXY->iY + stArea.iY1;
    return TRUE;
}

/**
 *  ��ü ȭ���� �������� �� �簢�� ��ǥ�� ������ ���� ��ǥ�� ��ȯ
 */
BOOL kConvertRectScreenToClient( QWORD qwWindowID, const RECT* pstArea, 
        RECT* pstAreaInWindow )
{
    RECT stWindowArea;
    
    // ������ ������ ��ȯ
    if( kGetWindowArea( qwWindowID, &stWindowArea ) == FALSE )
    {
        return FALSE;
    }
    
    pstAreaInWindow->iX1 = pstArea->iX1 - stWindowArea.iX1;
    pstAreaInWindow->iY1 = pstArea->iY1 - stWindowArea.iY1;
    pstAreaInWindow->iX2 = pstArea->iX2 - stWindowArea.iX1;
    pstAreaInWindow->iY2 = pstArea->iY2 - stWindowArea.iY1;
    return TRUE;
}

/**
 *  ������ ���θ� �������� �� �簢�� ��ǥ�� ȭ�� ��ǥ�� ��ȯ
 */
BOOL kConvertRectClientToScreen( QWORD qwWindowID, const RECT* pstArea, 
        RECT* pstAreaInScreen )
{
    RECT stWindowArea;
    
    // ������ ������ ��ȯ
    if( kGetWindowArea( qwWindowID, &stWindowArea ) == FALSE )
    {
        return FALSE;
    }
    
    pstAreaInScreen->iX1 = pstArea->iX1 + stWindowArea.iX1;
    pstAreaInScreen->iY1 = pstArea->iY1 + stWindowArea.iY1;
    pstAreaInScreen->iX2 = pstArea->iX2 + stWindowArea.iX1;
    pstAreaInScreen->iY2 = pstArea->iY2 + stWindowArea.iY1;
    return TRUE;
}

//==============================================================================
//  �½�ũ�� ����ϴ� ȭ�� ������Ʈ �Լ�
//==============================================================================
/**
 *  �����츦 ȭ�鿡 ������Ʈ
 *      �½�ũ���� ����ϴ� �Լ�
 */
BOOL kUpdateScreenByID( QWORD qwWindowID )
{
    EVENT stEvent;
    WINDOW* pstWindow;
    
    // �����츦 �˻�
    pstWindow = kGetWindow( qwWindowID );
    // �����찡 ���ų� ȭ�鿡 ������ ������ �̺�Ʈ�� ������ �Ŵ����� ������ �ʿ� ����
    if( ( pstWindow == NULL ) &&
        ( ( pstWindow->dwFlags & WINDOW_FLAGS_SHOW ) == 0 ) )
    {
        return FALSE; 
    }
    
    // �̺�Ʈ �ڷᱸ���� ä��. ������ ID�� ����
    stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYID;
    stEvent.stWindowEvent.qwWindowID = qwWindowID;
    
    return kSendEventToWindowManager( &stEvent );  
}

/**
 *  �������� ���θ� ȭ�鿡 ������Ʈ
 *      �½�ũ���� ����ϴ� �Լ�
 */
BOOL kUpdateScreenByWindowArea( QWORD qwWindowID, const RECT* pstArea )
{
    EVENT stEvent;
    WINDOW* pstWindow;
    
    // �����츦 �˻�
    pstWindow = kGetWindow( qwWindowID );
    // �����찡 ���ų� ȭ�鿡 ������ ������ �̺�Ʈ�� ������ �Ŵ����� ������ �ʿ� ����
    if( ( pstWindow == NULL ) &&
        ( ( pstWindow->dwFlags & WINDOW_FLAGS_SHOW ) == 0 ) )
    {
        return FALSE; 
    }
    
    // �̺�Ʈ �ڷᱸ���� ä��. ������ ID�� ������ ���� ������ ����
    stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA;
    stEvent.stWindowEvent.qwWindowID = qwWindowID;
    kMemCpy( &( stEvent.stWindowEvent.stArea ), pstArea, sizeof( RECT ) );
    
    return kSendEventToWindowManager( &stEvent );  
}

/**
 *  ȭ�� ��ǥ�� ȭ���� ������Ʈ
 *      �½�ũ���� ����ϴ� �Լ�
 */
BOOL kUpdateScreenByScreenArea( const RECT* pstArea )
{
    EVENT stEvent;
    
    // �̺�Ʈ �ڷᱸ���� ä��. ������ ID�� ȭ�� ������ ����
    stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA;
    stEvent.stWindowEvent.qwWindowID = WINDOW_INVALIDID;
    kMemCpy( &( stEvent.stWindowEvent.stArea ), pstArea, sizeof( RECT ) );
    
    return kSendEventToWindowManager( &stEvent );  
}

//==============================================================================
//  �̺�Ʈ ť ���� �Լ�
//==============================================================================
/**
 *  ������� �̺�Ʈ�� ����
 */
BOOL kSendEventToWindow( QWORD qwWindowID, const EVENT* pstEvent )
{
    WINDOW* pstWindow;
    BOOL bResult;
    
    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }

    bResult = kPutQueue( &( pstWindow->stEventQueue ), pstEvent );
    
    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );

    return bResult;
}

/**
 *  �������� �̺�Ʈ ť�� ����� �̺�Ʈ�� ����
 */
BOOL kReceiveEventFromWindowQueue( QWORD qwWindowID, EVENT* pstEvent )
{
    WINDOW* pstWindow;
    BOOL bResult;

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    bResult = kGetQueue( &( pstWindow->stEventQueue ), pstEvent );
    
    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );
    
    return bResult;
}

/**
 *  ������ �Ŵ����� �̺�Ʈ�� ����
 */
BOOL kSendEventToWindowManager( const EVENT* pstEvent )
{
    BOOL bResult = FALSE;
    
    // ť�� ���� �� ���� ������ �����͸� ���� �� ����
    if( kIsQueueFull( &( gs_stWindowManager.stEventQueue ) ) == FALSE )
    {
        // ����ȭ ó��
        kLock( &( gs_stWindowManager.stLock ) );
    
        bResult = kPutQueue( &( gs_stWindowManager.stEventQueue ), pstEvent );
        
        // ����ȭ ó��
        kUnlock( &( gs_stWindowManager.stLock ) );
    }
    return bResult;    
}

/**
 *  ������ �Ŵ����� �̺�Ʈ ť�� ����� �̺�Ʈ�� ����
 */
BOOL kReceiveEventFromWindowManagerQueue( EVENT* pstEvent )
{
    BOOL bResult = FALSE;
    
    // ť�� ������� ������ �����͸� ���� �� ����
    if( kIsQueueEmpty( &( gs_stWindowManager.stEventQueue ) ) == FALSE )
    {
        // ����ȭ ó��
        kLock( &( gs_stWindowManager.stLock ) );

        bResult = kGetQueue( &( gs_stWindowManager.stEventQueue ), pstEvent );

        // ����ȭ ó��
        kUnlock( &( gs_stWindowManager.stLock ) );
    }    
    return bResult;
}

/**
 *  ���콺 �̺�Ʈ �ڷᱸ���� ����
 */
BOOL kSetMouseEvent( QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY, 
        BYTE bButtonStatus, EVENT* pstEvent )
{
    POINT stMouseXYInWindow;
    POINT stMouseXY;
    
    // �̺�Ʈ ������ Ȯ���Ͽ� ���콺 �̺�Ʈ ����
    switch( qwEventType )
    {
        // ���콺 �̺�Ʈ ó��
    case EVENT_MOUSE_MOVE:
    case EVENT_MOUSE_LBUTTONDOWN:
    case EVENT_MOUSE_LBUTTONUP:            
    case EVENT_MOUSE_RBUTTONDOWN:
    case EVENT_MOUSE_RBUTTONUP:
    case EVENT_MOUSE_MBUTTONDOWN:
    case EVENT_MOUSE_MBUTTONUP:
        // ���콺�� X, Y��ǥ�� ����
        stMouseXY.iX = iMouseX;
        stMouseXY.iY = iMouseY;
        
        // ���콺 X, Y��ǥ�� ������ ���� ��ǥ�� ��ȯ
        if( kConvertPointScreenToClient( qwWindowID, &stMouseXY, &stMouseXYInWindow ) 
                == FALSE )
        {
            return FALSE;
        }

        // �̺�Ʈ Ÿ�� ����
        pstEvent->qwType = qwEventType;
        // ������ ID ����
        pstEvent->stMouseEvent.qwWindowID = qwWindowID;    
        // ���콺 ��ư�� ���� ����
        pstEvent->stMouseEvent.bButtonStatus = bButtonStatus;
        // ���콺 Ŀ���� ��ǥ�� ������ ���� ��ǥ�� ��ȯ�� ���� ����
        kMemCpy( &( pstEvent->stMouseEvent.stPoint ), &stMouseXYInWindow, 
                sizeof( POINT ) );
        break;
        
    default:
        return FALSE;
        break;
    }    
    return TRUE;
}

/**
 *  ������ �̺�Ʈ �ڷᱸ���� ����
 */
BOOL kSetWindowEvent( QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent )
{
    RECT stArea;
    
    // �̺�Ʈ ������ Ȯ���Ͽ� ������ �̺�Ʈ ����
    switch( qwEventType )
    {
        // ������ �̺�Ʈ ó��
    case EVENT_WINDOW_SELECT:
    case EVENT_WINDOW_DESELECT:
    case EVENT_WINDOW_MOVE:
    case EVENT_WINDOW_RESIZE:
    case EVENT_WINDOW_CLOSE:
        // �̺�Ʈ Ÿ�� ����
        pstEvent->qwType = qwEventType;
        // ������ ID ����
        pstEvent->stWindowEvent.qwWindowID = qwWindowID;
        // ������ ������ ��ȯ
        if( kGetWindowArea( qwWindowID, &stArea ) == FALSE )
        {
            return FALSE;
        }
        
        // �������� ���� ��ǥ�� ����
        kMemCpy( &( pstEvent->stWindowEvent.stArea ), &stArea, sizeof( RECT ) );
        break;
        
    default:
        return FALSE;
        break;
    }    
    return TRUE;
}

/**
 *  Ű �̺�Ʈ �ڷᱸ���� ����
 */
void kSetKeyEvent( QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent )
{
    // ���� �Ǵ� ������ ó��
    if( pstKeyData->bFlags & KEY_FLAGS_DOWN )
    {
        pstEvent->qwType = EVENT_KEY_DOWN;
    }
    else
    {
        pstEvent->qwType = EVENT_KEY_UP;
    }
    
    // Ű�� �� ������ ����
    pstEvent->stKeyEvent.bASCIICode = pstKeyData->bASCIICode;
    pstEvent->stKeyEvent.bScanCode = pstKeyData->bScanCode;
    pstEvent->stKeyEvent.bFlags = pstKeyData->bFlags;
}

//==============================================================================
//  ������ ���ο� �׸��� �Լ��� ���콺 Ŀ�� ����
//==============================================================================
/**
 *  ������ ȭ�� ���ۿ� ������ �׵θ� �׸���
 */
BOOL kDrawWindowFrame( QWORD qwWindowID )
{
    WINDOW* pstWindow;
    RECT stArea;
    int iWidth;
    int iHeight;

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // �������� �ʺ�� ���̸� ���
    iWidth = kGetRectangleWidth( &( pstWindow->stArea ) );
    iHeight = kGetRectangleHeight( &( pstWindow->stArea ) );
    // Ŭ���� ���� ����
    kSetRectangleData( 0, 0, iWidth - 1, iHeight - 1, &stArea );

    // ������ �������� �����ڸ��� �׸�, 2 �ȼ� �β�
    kInternalDrawRect( &stArea, pstWindow->pstWindowBuffer,
            0, 0, iWidth - 1, iHeight - 1, WINDOW_COLOR_FRAME, FALSE );

    kInternalDrawRect( &stArea, pstWindow->pstWindowBuffer,
            1, 1, iWidth - 2, iHeight - 2, WINDOW_COLOR_FRAME, FALSE );

    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );

    return TRUE;
}


/**
 *  ������ ȭ�� ���ۿ� ��� �׸���
 */
BOOL kDrawWindowBackground( QWORD qwWindowID )
{
    WINDOW* pstWindow;
    int iWidth;
    int iHeight;
    RECT stArea;
    int iX;
    int iY;

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }

    // �������� �ʺ�� ���̸� ���
    iWidth = kGetRectangleWidth( &( pstWindow->stArea ) );
    iHeight = kGetRectangleHeight( &( pstWindow->stArea ) );
    // Ŭ���� ���� ����
    kSetRectangleData( 0, 0, iWidth - 1, iHeight - 1, &stArea );

    // �����쿡 ���� ǥ������ ������ �� �Ʒ����� ä��
    if( pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE )
    {
        iY = WINDOW_TITLEBAR_HEIGHT;
    }
    else
    {
        iY = 0;
    }

    // ������ �׵θ��� �׸��� �ɼ��� �����Ǿ� ������ �׵θ��� ������ ������ ä��
    if( pstWindow->dwFlags & WINDOW_FLAGS_DRAWFRAME )
    {
        iX = 2;
    }
    else
    {
        iX = 0;
    }

    // �������� ���θ� ä��
    kInternalDrawRect( &stArea, pstWindow->pstWindowBuffer,
            iX, iY, iWidth - 1 - iX, iHeight - 1 - iX, WINDOW_COLOR_BACKGROUND, 
            TRUE );
    
    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );

    return TRUE;
}

/**
 *  ������ ȭ�� ���ۿ� ������ ���� ǥ���� �׸���
 */
BOOL kDrawWindowTitle( QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle )
{
    WINDOW* pstWindow;
    int iWidth;
    int iHeight;
    int iX;
    int iY;
    RECT stArea;
    RECT stButtonArea;
    COLOR stTitleBarColor;

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // �������� �ʺ�� ���̸� ���
    iWidth = kGetRectangleWidth( &( pstWindow->stArea ) );
    iHeight = kGetRectangleHeight( &( pstWindow->stArea ) );
    // Ŭ���� ���� ����
    kSetRectangleData( 0, 0, iWidth - 1, iHeight - 1, &stArea );

    //--------------------------------------------------------------------------
    // ���� ǥ���� �׸���
    //--------------------------------------------------------------------------
    // ���� ǥ������ ä��
    if( bSelectedTitle == TRUE )
    {
        stTitleBarColor = WINDOW_COLOR_TITLEBARACTIVEBACKGROUND;
    }
    else
    {
        stTitleBarColor = WINDOW_COLOR_TITLEBARINACTIVEBACKGROUND;
    }
    kInternalDrawRect( &stArea, pstWindow->pstWindowBuffer,
            0, 3, iWidth - 1, WINDOW_TITLEBAR_HEIGHT - 1, stTitleBarColor, TRUE );

    // ������ ������ ǥ��
    kInternalDrawText( &stArea, pstWindow->pstWindowBuffer,
            6, 3, WINDOW_COLOR_TITLEBARTEXT, stTitleBarColor, pcTitle, 
            kStrLen( pcTitle ) );

    // ���� ǥ������ ��ü�� ���̰� ������ ���� �׸�, 2 �ȼ� �β�
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            1, 1, iWidth - 1, 1, WINDOW_COLOR_TITLEBARBRIGHT1 );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            1, 2, iWidth - 1, 2, WINDOW_COLOR_TITLEBARBRIGHT2 );

    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            1, 2, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT1 );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            2, 2, 2, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT2 );

    // ���� ǥ������ �Ʒ��ʿ� ���� �׸�
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            2, WINDOW_TITLEBAR_HEIGHT - 2, iWidth - 2, WINDOW_TITLEBAR_HEIGHT - 2,
            WINDOW_COLOR_TITLEBARUNDERLINE );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            2, WINDOW_TITLEBAR_HEIGHT - 1, iWidth - 2, WINDOW_TITLEBAR_HEIGHT - 1,
            WINDOW_COLOR_TITLEBARUNDERLINE );

    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );

    //--------------------------------------------------------------------------
    // �ݱ� ��ư �׸���
    //--------------------------------------------------------------------------
    // �ݱ� ��ư�� �׸�, ������ ���� ǥ��
    stButtonArea.iX1 = iWidth - WINDOW_XBUTTON_SIZE - 1;
    stButtonArea.iY1 = 1;
    stButtonArea.iX2 = iWidth - 2;
    stButtonArea.iY2 = WINDOW_XBUTTON_SIZE - 1;
    kDrawButton( qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "", 
            WINDOW_COLOR_BACKGROUND );

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // �ݱ� ��ư ���ο� �밢�� X�� 3 �ȼ��� �׸�
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            iWidth - 2 - 18 + 4, 1 + 4, iWidth - 2 - 4,
            WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_XBUTTONLINECOLOR );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            iWidth - 2 - 18 + 5, 1 + 4, iWidth - 2 - 4,
            WINDOW_TITLEBAR_HEIGHT - 7, WINDOW_COLOR_XBUTTONLINECOLOR );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            iWidth - 2 - 18 + 4, 1 + 5, iWidth - 2 - 5,
            WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_XBUTTONLINECOLOR );

    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            iWidth - 2 - 18 + 4, 19 - 4, iWidth - 2 - 4, 1 + 4,
            WINDOW_COLOR_XBUTTONLINECOLOR );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            iWidth - 2 - 18 + 5, 19 - 4, iWidth - 2 - 4, 1 + 5,
            WINDOW_COLOR_XBUTTONLINECOLOR );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            iWidth - 2 - 18 + 4, 19 - 5, iWidth - 2 - 5, 1 + 4,
            WINDOW_COLOR_XBUTTONLINECOLOR );

    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );
    
    return TRUE;
}

/**
 *  ������ ���ο� ��ư �׸���
 */
BOOL kDrawButton( QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor,
        const char* pcText, COLOR stTextColor )
{
    WINDOW* pstWindow;
    RECT stArea;
    int iWindowWidth;
    int iWindowHeight;
    int iTextLength;
    int iTextWidth;
    int iButtonWidth;
    int iButtonHeight;
    int iTextX;
    int iTextY;

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // �������� �ʺ�� ���̸� ���
    iWindowWidth = kGetRectangleWidth( &( pstWindow->stArea ) );
    iWindowHeight = kGetRectangleHeight( &( pstWindow->stArea ) );
    // Ŭ���� ���� ����
    kSetRectangleData( 0, 0, iWindowWidth - 1, iWindowHeight - 1, &stArea );

    // ��ư�� ������ ǥ��
    kInternalDrawRect( &stArea, pstWindow->pstWindowBuffer,
            pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX2,
            pstButtonArea->iY2, stBackgroundColor, TRUE );

    // ��ư�� �ؽ�Ʈ�� �ʺ�� ���̸� ���
    iButtonWidth = kGetRectangleWidth( pstButtonArea );
    iButtonHeight = kGetRectangleHeight( pstButtonArea );
    iTextLength = kStrLen( pcText );
    iTextWidth = iTextLength * FONT_ENGLISHWIDTH;
    
    // �ؽ�Ʈ�� ��ư�� ����� ��ġ�ϵ��� �����
    iTextX = ( pstButtonArea->iX1 + iButtonWidth / 2 ) - iTextWidth / 2;
    iTextY = ( pstButtonArea->iY1 + iButtonHeight / 2 ) - FONT_ENGLISHHEIGHT / 2;
    kInternalDrawText( &stArea, pstWindow->pstWindowBuffer, iTextX, iTextY, 
            stTextColor, stBackgroundColor, pcText, iTextLength );      
    
    // ��ư�� ��ü�� ���̰� �׵θ��� �׸�, 2 �ȼ� �β��� �׸�
    // ��ư�� ���ʰ� ���� ��� ǥ��
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX2,
            pstButtonArea->iY1, WINDOW_COLOR_BUTTONBRIGHT );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            pstButtonArea->iX1, pstButtonArea->iY1 + 1, pstButtonArea->iX2 - 1,
            pstButtonArea->iY1 + 1, WINDOW_COLOR_BUTTONBRIGHT );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX1,
            pstButtonArea->iY2, WINDOW_COLOR_BUTTONBRIGHT );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            pstButtonArea->iX1 + 1, pstButtonArea->iY1, pstButtonArea->iX1 + 1,
            pstButtonArea->iY2 - 1, WINDOW_COLOR_BUTTONBRIGHT );

    // ��ư�� �����ʰ� �Ʒ��� ��Ӱ� ǥ��
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            pstButtonArea->iX1 + 1, pstButtonArea->iY2, pstButtonArea->iX2,
            pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            pstButtonArea->iX1 + 2, pstButtonArea->iY2 - 1, pstButtonArea->iX2,
            pstButtonArea->iY2 - 1, WINDOW_COLOR_BUTTONDARK );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            pstButtonArea->iX2, pstButtonArea->iY1 + 1, pstButtonArea->iX2,
            pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK );
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer,
            pstButtonArea->iX2 - 1, pstButtonArea->iY1 + 2, pstButtonArea->iX2 -1,
            pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK );
    
    // ����ȭ ó��
    kUnlock( &( pstWindow->stLock ) );

    return TRUE;
}

// ���콺 Ŀ���� �̹����� �����ϴ� ������
static BYTE gs_vwMouseBuffer[ MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT ] =
{
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 0, 0,
    0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 3, 2, 1, 1, 2, 3, 2, 2, 2, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 2, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
    0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
};

/**
 *  X, Y ��ġ�� ���콺 Ŀ���� ���
 */
static void kDrawCursor( int iX, int iY )
{
    int i;
    int j;
    BYTE* pbCurrentPos;

    // Ŀ�� �������� ���� ��ġ�� ����
    pbCurrentPos = gs_vwMouseBuffer;

    // Ŀ���� �ʺ�� ���̸�ŭ ������ ���鼭 �ȼ��� ȭ�鿡 ���
    for( j = 0 ; j < MOUSE_CURSOR_HEIGHT ; j++ )
    {
        for( i = 0 ; i < MOUSE_CURSOR_WIDTH ; i++ )
        {
            switch( *pbCurrentPos )
            {
                // 0�� ������� ����
            case 0:
                // nothing
                break;

                // ���� �ٱ��� �׵θ�, ���������� ���
            case 1:
                kInternalDrawPixel( &( gs_stWindowManager.stScreenArea ),
                        gs_stWindowManager.pstVideoMemory, i + iX, j + iY,
                        MOUSE_CURSOR_OUTERLINE );
                break;

                // ���ʰ� �ٱ����� ���, ��ο� ������� ���
            case 2:
                kInternalDrawPixel( &( gs_stWindowManager.stScreenArea ),
                        gs_stWindowManager.pstVideoMemory, i + iX, j + iY,
                        MOUSE_CURSOR_OUTER );
                break;

                // Ŀ���� ��, ���� ������ ���
            case 3:
                kInternalDrawPixel( &( gs_stWindowManager.stScreenArea ),
                        gs_stWindowManager.pstVideoMemory, i + iX, j + iY,
                        MOUSE_CURSOR_INNER );
                break;
            }

            // Ŀ���� �ȼ��� ǥ�õʿ� ���� Ŀ�� �������� ��ġ�� ���� �̵�
            pbCurrentPos++;
        }
    }
}

/**
 *  ���콺 Ŀ���� �ش� ��ġ�� �̵��ؼ� �׷���
 */
void kMoveCursor( int iX, int iY )
{
    RECT stPreviousArea;

    // ���콺 Ŀ���� ȭ���� ����� ���ϵ��� ����
    if( iX < gs_stWindowManager.stScreenArea.iX1 )
    {
        iX = gs_stWindowManager.stScreenArea.iX1;
    }
    else if( iX > gs_stWindowManager.stScreenArea.iX2 )
    {
        iX = gs_stWindowManager.stScreenArea.iX2;
    }

    if( iY < gs_stWindowManager.stScreenArea.iY1 )
    {
        iY = gs_stWindowManager.stScreenArea.iY1;
    }
    else if( iY > gs_stWindowManager.stScreenArea.iY2 )
    {
        iY = gs_stWindowManager.stScreenArea.iY2;
    }

    // ����ȭ ó��
    kLock( &( gs_stWindowManager.stLock ) );
    
    // ������ ���콺 Ŀ���� �ִ� �ڸ��� ����
    stPreviousArea.iX1 = gs_stWindowManager.iMouseX;
    stPreviousArea.iY1 = gs_stWindowManager.iMouseY;
    stPreviousArea.iX2 = gs_stWindowManager.iMouseX + MOUSE_CURSOR_WIDTH - 1;
    stPreviousArea.iY2 = gs_stWindowManager.iMouseY + MOUSE_CURSOR_HEIGHT - 1;
    
    // ���콺 Ŀ���� �� ��ġ�� ����
    gs_stWindowManager.iMouseX = iX;
    gs_stWindowManager.iMouseY = iY;

    // ����ȭ ó��
    kUnlock( &( gs_stWindowManager.stLock ) );
    
    // ���콺�� ������ �ִ� ������ �ٽ� �׸�
    kRedrawWindowByArea( &stPreviousArea, WINDOW_INVALIDID );

    // ���ο� ��ġ�� ���콺 Ŀ���� ���
    kDrawCursor( iX, iY );
}

/**
 *  ���� ���콺 Ŀ���� ��ġ�� ��ȯ
 */
void kGetCursorPosition( int* piX, int* piY )
{
    *piX = gs_stWindowManager.iMouseX;
    *piY = gs_stWindowManager.iMouseY;
}

/**
 *  ������ ���ο� �� �׸���
 */
BOOL kDrawPixel( QWORD qwWindowID, int iX, int iY, COLOR stColor )
{
    WINDOW* pstWindow;
    RECT stArea;

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // ������ ���� ��ǥ�� 0,0���� �ϴ� ��ǥ�� ������ ��ȯ
    kSetRectangleData( 0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1, 
            pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea );

    // ���� �Լ��� ȣ��
    kInternalDrawPixel( &stArea, pstWindow->pstWindowBuffer, iX, iY,
            stColor );

    // ����ȭ ó��
    kUnlock( &pstWindow->stLock );

    return TRUE;
}


/**
 *  ������ ���ο� ���� �׸���
 */
BOOL kDrawLine( QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor )
{
    WINDOW* pstWindow;
    RECT stArea;

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // ������ ���� ��ǥ�� 0,0���� �ϴ� ������ ���� ��ǥ�� ������ ��ȯ
    kSetRectangleData( 0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1, 
            pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea );
    
    // ���� �Լ��� ȣ��
    kInternalDrawLine( &stArea, pstWindow->pstWindowBuffer, iX1, iY1,
            iX2, iY2, stColor );

    // ����ȭ ó��
    kUnlock( &pstWindow->stLock );
    return TRUE;
}

/**
 *  ������ ���ο� �簢�� �׸���
 */
BOOL kDrawRect( QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2,
        COLOR stColor, BOOL bFill )
{
    WINDOW* pstWindow;
    RECT stArea;
    
    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }

    // ������ ���� ��ǥ�� 0,0���� �ϴ� ������ ���� ��ǥ�� ������ ��ȯ
    kSetRectangleData( 0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1, 
            pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea );
    
    // ���� �Լ��� ȣ��
    kInternalDrawRect( &stArea, pstWindow->pstWindowBuffer, iX1, iY1,
            iX2, iY2, stColor, bFill );

    // ����ȭ ó��
    kUnlock( &pstWindow->stLock );
    return TRUE;
}

/**
 *  ������ ���ο� �� �׸���
 */
BOOL kDrawCircle( QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor,
        BOOL bFill )
{
    WINDOW* pstWindow;
    RECT stArea;
    
    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // ������ ���� ��ǥ�� 0,0���� �ϴ� ������ ���� ��ǥ�� ������ ��ȯ
    kSetRectangleData( 0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1, 
            pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea );
    
    // ���� �Լ��� ȣ��
    kInternalDrawCircle( &stArea, pstWindow->pstWindowBuffer,
            iX, iY, iRadius, stColor, bFill );

    // ����ȭ ó��
    kUnlock( &pstWindow->stLock );
    return TRUE;
}

/**
 *  ������ ���ο� ���� ���
 */
BOOL kDrawText( QWORD qwWindowID, int iX, int iY, COLOR stTextColor,
        COLOR stBackgroundColor, const char* pcString, int iLength )
{
    WINDOW* pstWindow;
    RECT stArea;

    // ������ �˻��� ����ȭ ó��
    pstWindow = kGetWindowWithWindowLock( qwWindowID );
    if( pstWindow == NULL )
    {
        return FALSE;
    }
    
    // ������ ���� ��ǥ�� 0,0���� �ϴ� ������ ���� ��ǥ�� ������ ��ȯ
    kSetRectangleData( 0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1, 
            pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea );
    
    // ���� �Լ��� ȣ��
    kInternalDrawText( &stArea, pstWindow->pstWindowBuffer, iX, iY,
            stTextColor, stBackgroundColor, pcString, iLength );

    // ����ȭ ó��
    kUnlock( &pstWindow->stLock );
    return TRUE;
}

//==============================================================================
//  ȭ�� ������Ʈ�� ����ϴ� ȭ�� ������Ʈ ��Ʈ�� ����
//==============================================================================
/**
 *  ȭ�� ������Ʈ�� ����� ��Ʈ���� ����
 *      ��ǥ�� ȭ�� ��ǥ�� ���
 */
BOOL kCreateDrawBitmap( const RECT* pstArea, DRAWBITMAP* pstDrawBitmap )
{
    // ȭ�� ������ ��ġ�� �κ��� ������ ��Ʈ���� ������ �ʿ䰡 ����
    if( kGetOverlappedRectangle( &( gs_stWindowManager.stScreenArea ), pstArea,
            &( pstDrawBitmap->stArea ) ) == FALSE )
    {
        return FALSE;
    }
 
    // ������ �Ŵ����� �ִ� ȭ�� ������Ʈ ��Ʈ�� ���۸� ����
    pstDrawBitmap->pbBitmap = gs_stWindowManager.pbDrawBitmap;
    
    return kFillDrawBitmap( pstDrawBitmap, &( pstDrawBitmap->stArea ), TRUE );
}

/**
 *  ȭ�鿡 ������Ʈ�� ��Ʈ�� ������ ���� ������ ��ġ�� �κп� ���� 0 �Ǵ� 1�� ä��
 */
static BOOL kFillDrawBitmap( DRAWBITMAP* pstDrawBitmap, RECT* pstArea, BOOL bFill )
{
    RECT stOverlappedArea;
    int iByteOffset;
    int iBitOffset;
    int iAreaSize;
    int iOverlappedWidth;
    int iOverlappedHeight;
    BYTE bTempBitmap;
    int i;
    int iOffsetX;
    int iOffsetY;
    int iBulkCount;
    int iLastBitOffset;
    
    // ������Ʈ�� ������ ��ġ�� �κ��� ������ ��Ʈ�� ���ۿ� ���� ä�� �ʿ� ����
    if( kGetOverlappedRectangle( &( pstDrawBitmap->stArea ), pstArea,
            &stOverlappedArea ) == FALSE )
    {
        return FALSE;
    }
    
    // ��ġ�� ������ �ʺ�� ���̸� ���
    iOverlappedWidth = kGetRectangleWidth( &stOverlappedArea );
    iOverlappedHeight = kGetRectangleHeight( &stOverlappedArea );
    
    // ��ġ�� ������ ���̸�ŭ ����ϴ� ������ �ݺ�
    for( iOffsetY = 0 ; iOffsetY < iOverlappedHeight ; iOffsetY++ )
    {
        // ��Ʈ�� ���� ���� ������ ���� ��ġ�� ��ȯ
        if( kGetStartPositionInDrawBitmap( pstDrawBitmap, stOverlappedArea.iX1,
                stOverlappedArea.iY1 + iOffsetY, &iByteOffset, &iBitOffset ) == 
                    FALSE )
        {
            break;
        }
    
        //----------------------------------------------------------------------
        // ��ģ ������ �ʺ�ŭ ����ϴ� ������ �ݺ�
        //----------------------------------------------------------------------
        for( iOffsetX = 0 ; iOffsetX < iOverlappedWidth ; )
        {        
            // 8�ȼ� ������ ó���� �� �ִ� ũ�⸦ ����Ͽ� �ѹ��� ó��
            if( ( iBitOffset == 0x00 ) && ( ( iOverlappedWidth - iOffsetX ) >= 8 ) )
            {
                // ���� ��ġ���� 8�ȼ� ������ ó���� �� �ִ� �ִ� ũ�⸦ ���
                iBulkCount = ( iOverlappedWidth - iOffsetX ) >> 3;
                
                // 8�ȼ� ������ �ѹ��� ó��
                if( bFill == TRUE )
                {
                    kMemSet( pstDrawBitmap->pbBitmap + iByteOffset, 0xFF, 
                            iBulkCount );
                }
                else
                {
                    kMemSet( pstDrawBitmap->pbBitmap + iByteOffset, 0x00, 
                            iBulkCount );
                }
                
                // ��ü �������� ���������� ������ ��Ʈ���� ����ŭ�� ���� ��������
                iOffsetX += iBulkCount << 3;
                
                // ��Ʈ���� �������� ����
                iByteOffset += iBulkCount;
                iBitOffset = 0;
            }
            else
            {
                // ���� ��Ʈ�ʿ��� ����ؾ� �� ������ �ȼ��� ��Ʈ �������� ���
                iLastBitOffset = MIN( 8, iOverlappedWidth - iOffsetX + iBitOffset );                

                // ��Ʈ�� ����
                bTempBitmap = 0;
                for( i = iBitOffset ; i < iLastBitOffset ; i++ )
                {
                    bTempBitmap |= ( 0x01 << i );
                }
                
                // ��ü �������� 8�ȼ��� ������ ��Ʈ���� ����ŭ�� ���� ��������
                iOffsetX += ( iLastBitOffset - iBitOffset );
                
                // ��Ʈ�� ������ ����� ������ ������Ʈ
                if( bFill == TRUE )
                {
                    pstDrawBitmap->pbBitmap[ iByteOffset ] |= bTempBitmap;
                }
                else
                {
                    pstDrawBitmap->pbBitmap[ iByteOffset ] &= ~( bTempBitmap );
                }
                iByteOffset++;
                iBitOffset = 0;                
            }
        }
    }
    return TRUE;
}

/**
 *  ȭ�� ��ǥ�� ȭ�� ������Ʈ ��Ʈ�� ���ο��� �����ϴ� ����Ʈ �����°� ��Ʈ ��������
 *  ��ȯ
 *      ��ǥ�� ȭ�� ��ǥ�� ���
 */
inline BOOL kGetStartPositionInDrawBitmap( const DRAWBITMAP* pstDrawBitmap, 
        int iX, int iY, int* piByteOffset, int* piBitOffset )
{       
    int iWidth;
    int iOffsetX;
    int iOffsetY;
    
    // ��Ʈ�� ���� ���ο� ��ǥ�� ���Ե��� ������ ã�� �ʿ� ����
    if( kIsInRectangle( &( pstDrawBitmap->stArea ), iX, iY ) == FALSE )
    {
        return FALSE;
    }
    
    // ������Ʈ ���� ������ �������� ���
    iOffsetX = iX - pstDrawBitmap->stArea.iX1;
    iOffsetY = iY - pstDrawBitmap->stArea.iY1;
    // ������Ʈ�� ������ �ʺ� ���
    iWidth = kGetRectangleWidth( &( pstDrawBitmap->stArea ) );
    
    // ����Ʈ �������� X, Y�� �׸� �������� ��ġ�� ���� 8(����Ʈ�� 8�ȼ�)�� ������ ���
    *piByteOffset = ( iOffsetY * iWidth + iOffsetX ) >> 3;
    // ������ ����� ����Ʈ ���� ��Ʈ �������� 8�� ���� �������� ���
    *piBitOffset = ( iOffsetY * iWidth + iOffsetX ) & 0x7;
    
    return TRUE;
}

/**
 *  ȭ�鿡 �׸� ��Ʈ���� ��� 0���� �����Ǿ� ���̻� ������Ʈ�� ���� �������� ��ȯ
 */
inline BOOL kIsDrawBitmapAllOff( const DRAWBITMAP* pstDrawBitmap )
{
    int iByteCount;
    int iLastBitIndex;
    int iWidth;
    int iHeight;
    int i;
    BYTE* pbTempPosition;
    int iSize;
    
    // ������Ʈ�� ������ �ʺ�� ���̸� ���
    iWidth = kGetRectangleWidth( &( pstDrawBitmap->stArea) );
    iHeight = kGetRectangleHeight( &( pstDrawBitmap->stArea ) );
    
    // ��Ʈ���� ����Ʈ ���� ���
    iSize = iWidth * iHeight;
    iByteCount = iSize >> 3;
    
    // 8����Ʈ�� �ѹ��� ��
    pbTempPosition = pstDrawBitmap->pbBitmap;
    for( i = 0 ; i < ( iByteCount >> 3 ) ; i++ )
    {
        if( *( QWORD* ) ( pbTempPosition ) != 0 )
        {
            return FALSE;
        }
        pbTempPosition += 8;
    }
    
    // 8����Ʈ ������ �������� �ʴ� �������� ��
    for( i = 0 ; i < ( iByteCount & 0x7 ) ; i++ )
    {
        if( *pbTempPosition != 0 )
        {
            return FALSE;
        }
        pbTempPosition++;
    }
    
    // ��ü ũ�Ⱑ 8�� ������ �������� �ʴ´ٸ� �� ����Ʈ�� ���� ���� ���� ������
    // ����Ʈ�� �����Ƿ� �̸� �˻�
    iLastBitIndex = iSize & 0x7;
    for( i = 0 ; i < iLastBitIndex ; i++ )
    {
        if( *pbTempPosition & ( 0x01 << i ) )
        {
            return FALSE;
        }
    }    
    
    return TRUE;
}