
// CameraWatcherDlg.h : header file
//

#include <strmif.h>
#import "VCamRenderer.tlb" no_namespace, raw_interfaces_only exclude("UINT_PTR") 

#pragma once
#pragma comment(lib, "Strmiids.lib")

struct IVCamRenderer;

// CCameraWatcherDlg dialog
class CCameraWatcherDlg : public CDialogEx
{
// Construction
public:
	CCameraWatcherDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CAMERAWATCHER_DIALOG };
#endif

	HRESULT SetupCameras();
	void CleanCameras();

	void DetectVCamUsage();
	static DWORD WINAPI notification_usage__proc(__inout LPVOID pv);
	void usage_proc();
	void ShowUsingInfo();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()

// Members
protected:
	IBaseFilter* m_vcam_renderer;
	IVCamRenderer* m_vcam;
	LONG m_notification_monitor;
	HANDLE m_thread;
};
