#include "pch.h"

// DirectSound includes
#include <mmsystem.h>
#include <mmreg.h>
#include "DSStreamPlay.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Dsound.lib")

//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define NUM_BUFFERS     (4)
#define MAX(a,b)        ( (a) > (b) ? (a) : (b) )

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }




CDSStreamPlay::CDSStreamPlay(HWND hWnd, WAVEFORMATEX *pwfx, DWORD dwBufferMS, GUID PlaybackDeviceGUID)
{
	m_pDS            = NULL;
	m_pDSBOutput     = NULL;
	m_pDSFullDuplex  = NULL;
	m_pDSPlaybackNotify = NULL;
	m_hNotificationPlaybackEvent = NULL; 
	m_hShutdownEvent = NULL;
	m_dwNotifyPlaybackThreadID = 0;
	m_hNotifyPlaybackThread = NULL;
	m_dwLastPlaybackPlayCursor = 0;
	m_dwPlaybackStreamPosition = 0;
	m_hWnd = hWnd;
	m_dwBufferMS = dwBufferMS;
	m_pLastBuffer = NULL;
	m_pwfxInput = NULL;
	m_funcCallback  = NULL;
	m_guidPlaybackDevice = PlaybackDeviceGUID;

    m_hNotificationPlaybackEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hShutdownEvent = CreateEvent( NULL, TRUE, FALSE, NULL ); // manual reset event
	m_hProcessNow = CreateEvent( NULL, FALSE, FALSE, NULL );

	m_hNotifyPlaybackThread = CreateThread( NULL, 0, CDSStreamPlay::NotificationPlaybackProc, 
                                    this, 0, &m_dwNotifyPlaybackThreadID );
	SetThreadPriority(m_hNotifyPlaybackThread, THREAD_PRIORITY_TIME_CRITICAL);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	HRESULT hr = S_OK;
	hr = CreateBuffers(pwfx);
}

CDSStreamPlay::~CDSStreamPlay()
{
	SetEvent(m_hShutdownEvent);
	HANDLE hWaitThreads[2];

	hWaitThreads[0] = m_hNotifyPlaybackThread;
	WaitForMultipleObjects(1, hWaitThreads, TRUE, INFINITE);
	
	m_pDSBOutput->Release();
	m_pDSBOutput = NULL;
	delete [] m_pLastBuffer;
	m_pLastBuffer = NULL;
	delete [] m_aPosPlaybackNotify;
	delete [] m_pwfxInput;

	CloseHandle(m_hNotificationPlaybackEvent);
	CloseHandle(m_hShutdownEvent);
}


HANDLE CDSStreamPlay::GetProcessEvent()
{
	return m_hProcessNow;
}


BOOL CDSStreamPlay::SetCallback(BufferCallback_type func, DWORD_PTR InstanceData)
{
	m_InstanceData = InstanceData;
	m_funcCallback = func;
	return TRUE;
}


//-----------------------------------------------------------------------------
// Name: CreateOutputBuffer()
// Desc: Creates the ouptut buffer and sets up the notification positions
//       on the capture buffer
//-----------------------------------------------------------------------------
HRESULT CDSStreamPlay::CreateBuffers(WAVEFORMATEX *pwfx)
{
    HRESULT hr; 
    //WAVEFORMATEX m_wfxInput;

    // This sample works by creating notification events which 
    // are signaled when the buffer reachs specific offsets. 
    // Our thread waits for the associated event to be signaled, and
    // when it is, it calls HandlePlaybackNotification() which copy the 
    // data into the output buffer.
	m_pwfxInput = (WAVEFORMATEX*)new BYTE[sizeof(WAVEFORMATEX) + pwfx->cbSize];
	memcpy(m_pwfxInput, pwfx, sizeof(WAVEFORMATEX) + pwfx->cbSize);

	// Calculate the desired buffer size in bytes
	DWORD BufferSize = m_pwfxInput->nAvgBytesPerSec * m_dwBufferMS / 1000;
	BufferSize -= BufferSize % m_pwfxInput->nBlockAlign;

    // Set the notification size
	m_dwNotifySize = 160;

    // Set the buffer sizes 
    m_dwOutputBufferSize  = BufferSize;

	// calculate the number of notifications
	int nNotifies = m_dwOutputBufferSize / m_dwNotifySize;

    // Create the direct sound buffer 
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize          = sizeof(DSBUFFERDESC);
    dsbd.dwFlags         = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLPOSITIONNOTIFY;
    dsbd.dwBufferBytes   = m_dwOutputBufferSize;
    dsbd.guid3DAlgorithm = GUID_NULL;
    dsbd.lpwfxFormat     = m_pwfxInput;

	// Create the DirectSound buffer 
	LPDIRECTSOUNDBUFFER pDSBuffer = NULL;

	if( FAILED( hr = DirectSoundCreate8(&m_guidPlaybackDevice, &m_pDS, NULL) ) )
		return hr; //DXTRACE_ERR_MSGBOX( TEXT("DirectSoundCreate8"), hr );

	if (m_hWnd)
		hr = m_pDS->SetCooperativeLevel(m_hWnd, DSSCL_PRIORITY);

	if( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbd, &pDSBuffer, NULL ) ) )
        return hr; //DXTRACE_ERR_MSGBOX( TEXT("CreateSoundBuffer"), hr );
    if( FAILED( hr = pDSBuffer->QueryInterface( IID_IDirectSoundBuffer8, (VOID**)&m_pDSBOutput ) ) )
        return hr; //DXTRACE_ERR_MSGBOX( TEXT("QueryInterface"), hr );

	// create the repeat buffer
	m_pLastBuffer = new BYTE[m_dwOutputBufferSize];

    // Create a notification event, for when the sound stops playing
    if( FAILED( hr = m_pDSBOutput->QueryInterface( IID_IDirectSoundNotify, (VOID**)&m_pDSPlaybackNotify ) ) )
        return hr; //DXTRACE_ERR_MSGBOX( TEXT("QueryInterface"), hr );

	m_aPosPlaybackNotify = new DSBPOSITIONNOTIFY[nNotifies];
    // Setup the notification positions
    for( INT i = 0; i <nNotifies ; i++ )
    {
        m_aPosPlaybackNotify[i].dwOffset = (m_dwNotifySize * i) + m_dwNotifySize - 1;
        m_aPosPlaybackNotify[i].hEventNotify = m_hNotificationPlaybackEvent;
    }
    
    // Tell DirectSound when to notify us. the notification will come in the from 
    // of signaled events that are handled in WinMain()
    if( FAILED( hr = m_pDSPlaybackNotify->SetNotificationPositions( nNotifies, m_aPosPlaybackNotify ) ) )
        return hr; //DXTRACE_ERR_MSGBOX( TEXT("SetNotificationPositions Playback"), hr );

    return S_OK;
}


HRESULT CDSStreamPlay::StopBuffers()
{
	//DebugMsg("\nStopping Audio Buffers\n");
    m_pDSBOutput->Stop();

	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: StartBuffers()
// Desc: Start the capture buffer, and the start playing the output buffer
//-----------------------------------------------------------------------------
HRESULT CDSStreamPlay::StartBuffers()
{
    VOID*        pDSLockedBuffer = NULL;
    DWORD        dwDSLockedBufferSize;
    HRESULT hr;

	//DebugMsg("\nStarting Audio Buffers\n");
	FillMemory(m_pLastBuffer, m_dwOutputBufferSize, 0);
	m_dwLastPlaybackPlayCursor = m_dwOutputBufferSize / 2;
	m_dwPlaybackStreamPosition = 0;

	// Restore lost buffers
    if( FAILED( hr = RestoreBuffer( m_pDSBOutput, NULL ) ) )
        return hr; //DXTRACE_ERR_MSGBOX( TEXT("RestoreBuffer"), hr );

    // Reset the buffers
    DWORD dwNextOutputOffset = 0;
    m_pDSBOutput->SetCurrentPosition( 0 );
    
    // Rewind the output buffer, fill it with silence, and play it
    m_pDSBOutput->SetCurrentPosition( dwNextOutputOffset );

    // Fill the output buffer with silence at first
    // As data arrives, HandleNotifications() will fill
    // the output buffer with wave data.
    if( FAILED( hr = m_pDSBOutput->Lock( 0, m_dwOutputBufferSize, &pDSLockedBuffer, &dwDSLockedBufferSize, NULL, NULL, 0 ) ) )
        return hr; //DXTRACE_ERR_MSGBOX( TEXT("Lock"), hr );
    FillMemory( (BYTE*) pDSLockedBuffer, dwDSLockedBufferSize, 
                (BYTE)( 0 ) );
    m_pDSBOutput->Unlock( pDSLockedBuffer, dwDSLockedBufferSize, NULL, NULL ); 

    // Play the output buffer 
	DWORD flags = DSBPLAY_LOOPING;
    m_pDSBOutput->Play( 0, 0, flags );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreBuffer()
// Desc: Restores a lost buffer. *pbWasRestored returns TRUE if the buffer was 
//       restored.  It can also NULL if the information is not needed.
//-----------------------------------------------------------------------------
HRESULT CDSStreamPlay::RestoreBuffer( LPDIRECTSOUNDBUFFER pDSBuffer, BOOL* pbRestored )
{
    HRESULT hr;

    if( pbRestored != NULL )
        *pbRestored = FALSE;

    if( NULL == pDSBuffer )
        return S_FALSE;

    DWORD dwStatus;
    if( FAILED( hr = pDSBuffer->GetStatus( &dwStatus ) ) )
        return hr; //DXTRACE_ERR_MSGBOX( TEXT("GetStatus"), hr );

    if( dwStatus & DSBSTATUS_BUFFERLOST )
    {
        // Since the app could have just been activated, then
        // DirectSound may not be giving us control yet, so 
        // the restoring the buffer may fail.  
        // If it does, sleep until DirectSound gives us control.
        do 
        {
            hr = pDSBuffer->Restore();
            if( hr == DSERR_BUFFERLOST )
                Sleep( 10 );
        }
        while( ( hr = pDSBuffer->Restore() ) == DSERR_BUFFERLOST );

        if( pbRestored != NULL )
            *pbRestored = TRUE;

        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}


void CDSStreamPlay::LoadData(PBYTE pBuffer, DWORD dwAmountRequested, DWORD &dwAmountRead)
{
	//m_pFSPlayback.Pop(pBuffer, dwAmountRequested, (int&)dwAmountRead);
	if (m_funcCallback)
		m_funcCallback(pBuffer, dwAmountRequested, dwAmountRead, m_InstanceData);
}


//-----------------------------------------------------------------------------
// Name: HandleNotification()
// Desc: Handle the notification that tells us to copy data from the 
//       capture buffer to the output buffer 
//-----------------------------------------------------------------------------
HRESULT CDSStreamPlay::HandlePlaybackNotification() 
{
    HRESULT hr;
    VOID* pDSOutputLockedBuffer1     = NULL;
    VOID* pDSOutputLockedBuffer2     = NULL;
    DWORD dwDSOutputLockedBufferSize1;
    DWORD dwDSOutputLockedBufferSize2;
   
	DWORD dwWriteSize = 0;


	DWORD dwPlaybackCurrentPlayCursor, dwPlaybackCurrentWriteCursor;
	// Find where the current play cursor is at.  We can lock the memory between the 
	// current write cursor and the current play cursor.  For this sample we ignore
	// the current write cursor location and keep track of our own buffer position.
	m_pDSBOutput->GetCurrentPosition(&dwPlaybackCurrentPlayCursor, &dwPlaybackCurrentWriteCursor);

	// Check if the cursor has moved at all.
	if (dwPlaybackCurrentPlayCursor != m_dwLastPlaybackPlayCursor)
	{
		// Are we behind the play cursor or infront of it?
		if (dwPlaybackCurrentPlayCursor > m_dwLastPlaybackPlayCursor)
		{
			dwWriteSize = dwPlaybackCurrentPlayCursor - m_dwLastPlaybackPlayCursor;
		}
		else
		{
			dwWriteSize = dwPlaybackCurrentPlayCursor + m_dwOutputBufferSize - m_dwLastPlaybackPlayCursor;
		}
	}

	//round down to a chunk size
	dwWriteSize -= dwWriteSize % m_dwNotifySize;

	if (dwWriteSize > 0)
	{
		// limit our lock to half to total buffer, MAX
		if (dwWriteSize > m_dwOutputBufferSize / 2)
			dwWriteSize = m_dwOutputBufferSize / 2;

		// this is just a counter for external reference
		m_dwPlaybackStreamPosition += dwWriteSize;

		// Lock the output buffer down
		hr = m_pDSBOutput->Lock( m_dwLastPlaybackPlayCursor, dwWriteSize, 
											&pDSOutputLockedBuffer1, 
											&dwDSOutputLockedBufferSize1, 
											&pDSOutputLockedBuffer2, 
											&dwDSOutputLockedBufferSize2, 
											0L );

		if (FAILED(hr))
		{
			return hr;
		}

		DWORD nRead=0;

		// request data for our buffer
		LoadData(m_pLastBuffer, dwWriteSize, nRead);
		
		// copy data into part 1 of the buffer
		CopyMemory(pDSOutputLockedBuffer1, m_pLastBuffer, dwDSOutputLockedBufferSize1);

		if (nRead < (int)dwDSOutputLockedBufferSize1)
		{
			//DebugMsg("Buffer underrun1\n");
		}

		// if there was a part 2 (wrap) copy that data as well
		if (pDSOutputLockedBuffer2)
		{
			CopyMemory(pDSOutputLockedBuffer2, m_pLastBuffer + dwDSOutputLockedBufferSize1, dwDSOutputLockedBufferSize2);

			if (nRead < (int)(dwDSOutputLockedBufferSize1 + dwDSOutputLockedBufferSize2))
			{
				//DebugMsg("Buffer underrun2\n");
			}
		}

		// zero out the repeat buffer
		ZeroMemory(m_pLastBuffer + nRead, m_dwOutputBufferSize - nRead);

		// Unlock the play buffer, both pieces
		m_pDSBOutput->Unlock( pDSOutputLockedBuffer1, dwDSOutputLockedBufferSize1, 
							pDSOutputLockedBuffer2, dwDSOutputLockedBufferSize2 );

		// move our position along based on how much we wrote, wrap if necessary
		m_dwLastPlaybackPlayCursor += dwWriteSize;
		m_dwLastPlaybackPlayCursor %= m_dwOutputBufferSize;
	}

    return S_OK;
}



DWORD CDSStreamPlay::OutputBufferSize()
{
	return m_dwOutputBufferSize;
}


DWORD WINAPI CDSStreamPlay::NotificationPlaybackProc(LPVOID param)
{
	CDSStreamPlay *thiss = (CDSStreamPlay*)param;
	return thiss->NotificationPlaybackProc();
}


//-----------------------------------------------------------------------------
// Name: NotificationProc()
// Desc: Handles dsound notifcation events
//-----------------------------------------------------------------------------
DWORD CDSStreamPlay::NotificationPlaybackProc()
{
    HRESULT hr;
    DWORD   dwResult;
    BOOL    bDone = FALSE;

	HANDLE hWaitHandles[2];

	hWaitHandles[0] = m_hShutdownEvent;
	hWaitHandles[1] = m_hNotificationPlaybackEvent;

    while( !bDone ) 
    { 
		dwResult = WaitForMultipleObjects(2, hWaitHandles, FALSE, INFINITE);

	
        switch( dwResult )
        {
            case WAIT_OBJECT_0 + 1:
                // m_hNotificationPlaybackEvent is signaled

                // This means that DirectSound just finished playing 
                // a piece of the buffer, so we need to fill the circular 
                // buffer with new sound from our buffer

                if( FAILED( hr = HandlePlaybackNotification() ) )
                {
                    //DXTRACE_ERR_MSGBOX( TEXT("HandleNotification"), hr );
                    bDone = TRUE;
                }
				// set an event the caller can use for processing
				SetEvent(m_hProcessNow);
                break;

            case WAIT_OBJECT_0 + 0: // the done event
                m_pDSBOutput->Stop();
                bDone = TRUE;
                break;

			default:
				Sleep(100); // never called
        }
    }

    return 0;
}

