#pragma once


#include <dsound.h>

#define MAX_NUM_PLAY_NOTIFICATIONS  200

typedef WINUSERAPI BOOL (WINAPI *BufferCallback_type) (PBYTE pBuffer, DWORD dwAmountRequested, DWORD &dwAmountRead, DWORD_PTR InstanceData);


class CDSStreamPlay : public CObject
{
public:
	CDSStreamPlay(HWND hWnd, WAVEFORMATEX *pwfx, DWORD dwBufferMS=100, GUID PlaybackDeviceName=DSDEVID_DefaultPlayback);
	~CDSStreamPlay();
	HRESULT StartBuffers();
	HRESULT StopBuffers();
	HANDLE GetProcessEvent();
	DWORD CaptureBufferSize();
	DWORD OutputBufferSize();
		
	DWORD NotificationCaptureProc();
	DWORD NotificationPlaybackProc();
	static DWORD WINAPI NotificationPlaybackProc(LPVOID param);
	BOOL SetCallback(BufferCallback_type func, DWORD_PTR InstanceData);

protected:
	LPDIRECTSOUND8              m_pDS;
	LPDIRECTSOUNDBUFFER8        m_pDSBOutput;
	LPDIRECTSOUNDFULLDUPLEX     m_pDSFullDuplex;
	LPDIRECTSOUNDNOTIFY         m_pDSPlaybackNotify;
	PBYTE						m_pLastBuffer;

	DSBPOSITIONNOTIFY   *m_aPosPlaybackNotify;//[ MAX_NUM_PLAY_NOTIFICATIONS ];  
	HANDLE              m_hNotificationPlaybackEvent;
	HANDLE				m_hShutdownEvent;
	HANDLE				m_hProcessNow;

	DWORD               m_dwOutputBufferSize;
	DWORD               m_dwNotifySize;
	DWORD               m_dwNotifyPlaybackThreadID;
	HANDLE			    m_hNotifyPlaybackThread;
	HWND				m_hWnd;
	DWORD				m_dwBufferMS;
	GUID				m_guidPlaybackDevice;
	DWORD				m_dwLastCaptureCursorReadPosition;
	DWORD				m_dwLastPlaybackPlayCursor;
	DWORD				m_dwPlaybackStreamPosition;
	BufferCallback_type m_funcCallback;
	DWORD_PTR			m_InstanceData;

	WAVEFORMATEX  *m_pwfxInput;

	HRESULT RestoreBuffer( LPDIRECTSOUNDBUFFER pDSBuffer, BOOL* pbRestored );
	HRESULT HandlePlaybackNotification();
	HRESULT CreateBuffers(WAVEFORMATEX *pwfx);
	void LoadData(PBYTE pBuffer, DWORD dwAmountRequested, DWORD &dwAmountRead);
};

