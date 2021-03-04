
// CameraWatcherDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "CameraWatcher.h"
#include "CameraWatcherDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

MIDL_DEFINE_GUID(CLSID, CLSID_VCamRenderer, 0x3D2F839E, 0x1186, 0x4FCE, 0xB7, 0x72, 0xB6, 0x1F, 0xAE, 0x1A, 0xCE, 0xD7);

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCameraWatcherDlg dialog



CCameraWatcherDlg::CCameraWatcherDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CAMERAWATCHER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_vcam_renderer = nullptr;
	m_vcam = nullptr;
	m_notification_monitor = FALSE;
	m_thread = nullptr;
}

void CCameraWatcherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCameraWatcherDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CCameraWatcherDlg message handlers

BOOL CCameraWatcherDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	SetupCameras();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCameraWatcherDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCameraWatcherDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCameraWatcherDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCameraWatcherDlg::OnClose()
{
	CleanCameras();
	CDialogEx::OnClose();
}

void CCameraWatcherDlg::OnDestroy()
{
	CleanCameras();
	CDialogEx::OnDestroy();
}

//
// Initialize virtual camera
//
HRESULT CCameraWatcherDlg::SetupCameras()
{
	HRESULT hr = ::CoInitialize(nullptr);

	// Create VCam renderer filter
	if (FAILED(hr = CoCreateInstance(CLSID_VCamRenderer, NULL, CLSCTX_INPROC, IID_IBaseFilter,
		reinterpret_cast<void**>(&m_vcam_renderer)))) {
		MessageBox(_T("VCamRenderer filter not registered!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	// get [IVCamRender] interface from VCam Renderer filter
	if (FAILED(hr = m_vcam_renderer->QueryInterface(&m_vcam))) {
		MessageBox(_T("VCam Driver not installed!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	// create a thread to detect VCam usage.
	if (m_vcam) {
		DetectVCamUsage();
	}

	return hr;
}

//
// When dialog quit, release camera resource
//
void CCameraWatcherDlg::CleanCameras()
{
	// set an Empty Event Handle for VCam usage 
	m_notification_monitor = FALSE;
	if (m_vcam)	m_vcam->SetConnectionNotificationEvent(reinterpret_cast<__int64>(nullptr));

	if (m_vcam_renderer) m_vcam_renderer->Release(), m_vcam_renderer = nullptr;
	if (m_vcam) m_vcam->Release(), m_vcam = nullptr;
}

void CCameraWatcherDlg::DetectVCamUsage()
{
	ShowUsingInfo();

	// create a thread to detect vcam usage
	m_notification_monitor = TRUE;
	m_thread = CreateThread(nullptr, 0, notification_usage__proc, this, 0, nullptr);
}

DWORD WINAPI CCameraWatcherDlg::notification_usage__proc(LPVOID data)
{
	CCameraWatcherDlg* impl = reinterpret_cast<CCameraWatcherDlg*>(data);
	impl->usage_proc();
	return 0;
}

void CCameraWatcherDlg::usage_proc()
{
	// create a event and set it to VCam, when VCam usage number is changed, this event will be notified
	HANDLE h_notification = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (FAILED(m_vcam->SetConnectionNotificationEvent(reinterpret_cast<__int64>(h_notification)))) {
		CloseHandle(h_notification);
		h_notification = nullptr;
		return;
	}

	while (m_notification_monitor) {
		// wait for the notification event
		WaitForSingleObject(h_notification, INFINITE);

		// update using information
		if (m_notification_monitor)
			ShowUsingInfo();
	}
}

void CCameraWatcherDlg::ShowUsingInfo()
{
	if (m_vcam == nullptr)
		return;

	long connected_num = 0;
	if (m_vcam) {
		m_vcam->GetConnectedCount(&connected_num);
	}

	CString tt = _T("No application is ");
	if (connected_num == 1) tt = _T("1 application is ");
	else if (connected_num > 1) tt.Format(_T("%d applications are "), connected_num);

	CString message = tt + _T("using our camera.");
	SetDlgItemText(IDC_STATIC_VCAM_USAGE, message);
}
