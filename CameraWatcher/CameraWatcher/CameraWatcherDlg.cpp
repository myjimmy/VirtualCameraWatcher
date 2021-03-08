
// CameraWatcherDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "CameraWatcher.h"
#include "CameraWatcherDlg.h"
#include "afxdialogex.h"
#include <uuids.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define VCAM_NAME L"Virtual Camera"

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }
#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

MIDL_DEFINE_GUID(CLSID, CLSID_VCamRenderer, 0x3D2F839E, 0x1186, 0x4FCE, 0xB7, 0x72, 0xB6, 0x1F, 0xAE, 0x1A, 0xCE, 0xD7);

#define PROP_GUID 0xcb043957, 0x7b35, 0x456e, 0x9b, 0x61, 0x55, 0x13, 0x93, 0xf, 0x4d, 0x8e
#define PROP_DATA_ID		0
#define PROP_STATE_ID		1
#define PROP_PROCESS_ID		2

const GUID GUID_PROP_CLASS = { PROP_GUID };

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

	m_propertySet = nullptr;

	m_strActiveCameraName = L"";
}

void CCameraWatcherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CAMERA_COMBO, m_CameraList);
}

BEGIN_MESSAGE_MAP(CCameraWatcherDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SET_BUTTON, &CCameraWatcherDlg::OnBnClickedSetButton)
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
	InitCameraList();

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
	CleanCamera();
	CDialogEx::OnClose();
}

void CCameraWatcherDlg::OnDestroy()
{
	CleanCamera();
	CDialogEx::OnDestroy();
}

void CCameraWatcherDlg::InitCameraList()
{
	HRESULT hr = S_OK;
	IBaseFilter* pSrc = NULL;
	IMoniker* pMoniker = NULL;
	ICreateDevEnum* pDevEnum = NULL;
	IEnumMoniker* pClassEnum = NULL;

	hr = ::CoInitialize(nullptr);

	// Create the system device enumerator
	if (FAILED(hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void**)&pDevEnum))) {
		MessageBox(_T("Couldn't create system enumerator!"), _T("Error"), MB_OK | MB_ICONERROR);
		return;
	}

	// Create an enumerator for the video capture devices
	if (SUCCEEDED(hr))
	{
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
		if (FAILED(hr))
		{
			MessageBox(_T("Couldn't create class enumerator!"), _T("Error"), MB_OK | MB_ICONERROR);
			return;
		}
	}

	if (SUCCEEDED(hr))
	{
		// If there are no enumerators for the requested type, then 
		// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
		if (pClassEnum == NULL)
		{
			MessageBox(_T("No video capture device was detected!"), _T("Error"), MB_OK | MB_ICONERROR);
			return;
		}
	}

	// Use the devnum'th video capture device on the device list.
	// Note that if the Next() call succeeds but there are no monikers,
	// it will return S_FALSE (which is not a failure).  Therefore, we
	// check that the return code is S_OK instead of using SUCCEEDED() macro.

	if (SUCCEEDED(hr))
	{
		while (pClassEnum->Next(1, &pMoniker, NULL) == S_OK) {
			// Bind Moniker to a filter object
			//hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
			//if (FAILED(hr))
			//	continue;
			VARIANT var;
			IPropertyBag* props;
			HRESULT h2;
			CString strText;
			h2 = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&props);
			if (SUCCEEDED(h2)) {
				VariantInit(&var);
				var.vt = VT_BSTR;
				h2 = props->Read(L"FriendlyName", &var, 0);
				if (SUCCEEDED(h2)) {
					printf("%d:%ls\n", 0, var.bstrVal);
					strText.Format(L"%ls", var.bstrVal);
				}
				SysFreeString(var.bstrVal);
				VariantClear(&var);
			}
			SAFE_RELEASE(props);

			if (!strText.IsEmpty()) {
				m_CameraList.AddString(strText);
			}
		}
	}

	SAFE_RELEASE(pSrc);
	SAFE_RELEASE(pMoniker);
	SAFE_RELEASE(pDevEnum);
	SAFE_RELEASE(pClassEnum);
}

void CCameraWatcherDlg::OnBnClickedSetButton()
{
	int index = m_CameraList.GetCurSel();
	if (index == -1) {
		MessageBox(_T("Please select any camera from list."), _T("Warning"), MB_OK | MB_ICONWARNING);
		return;
	}

	CString strCameraName;
	m_CameraList.GetWindowText(strCameraName);

	CleanCamera();

	m_strActiveCameraName = strCameraName;
	SetupCamera(index);
}

HRESULT CCameraWatcherDlg::SetupCamera(int camera_index)
{
	HRESULT hr = S_OK;
	if (m_strActiveCameraName.IsEmpty())
		return hr;

	if (m_strActiveCameraName == VCAM_NAME) {
		hr = SetupCameraForVCam();
	}
	else {
		hr = SetupCamerasForAvshws(camera_index);
	}

	return hr;
}

void CCameraWatcherDlg::CleanCamera()
{
	if (m_strActiveCameraName.IsEmpty())
		return;

	if (m_strActiveCameraName == VCAM_NAME) {
		CleanCameraForVCam();
	}
	else {
		CleanCamerasForAvshws();
	}
}

//
// Initialize virtual camera
//
HRESULT CCameraWatcherDlg::SetupCameraForVCam()
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
		DetectCameraUsageForVCam();
	}

	return hr;
}

HRESULT CCameraWatcherDlg::SetupCamerasForAvshws(int camera_index)
{
	HRESULT hr = S_OK;
	IBaseFilter* pSrc = NULL;
	IMoniker* pMoniker = NULL;
	ICreateDevEnum* pDevEnum = NULL;
	IEnumMoniker* pClassEnum = NULL;

	hr = ::CoInitialize(nullptr);

	// Create the system device enumerator
	if (FAILED(hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void**)&pDevEnum))) {
		MessageBox(_T("Couldn't create system enumerator!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	// Create an enumerator for the video capture devices
	if (SUCCEEDED(hr))
	{
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
		if (FAILED(hr))
		{
			MessageBox(_T("Couldn't create class enumerator!"), _T("Error"), MB_OK | MB_ICONERROR);
			return hr;
		}
	}

	if (SUCCEEDED(hr))
	{
		// If there are no enumerators for the requested type, then 
		// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
		if (pClassEnum == NULL)
		{
			MessageBox(_T("No video capture device was detected!"), _T("Error"), MB_OK | MB_ICONERROR);
			return hr;
		}
	}

	// Use the devnum'th video capture device on the device list.
	// Note that if the Next() call succeeds but there are no monikers,
	// it will return S_FALSE (which is not a failure).  Therefore, we
	// check that the return code is S_OK instead of using SUCCEEDED() macro.

	if (SUCCEEDED(hr))
	{
		pClassEnum->Skip(camera_index);
		hr = pClassEnum->Next(1, &pMoniker, NULL);
		if (hr == S_FALSE)
		{
			MessageBox(_T("Unable to access video capture device!"), _T("Error"), MB_OK | MB_ICONERROR);
			return hr;
		}
	}

	if (SUCCEEDED(hr))
	{
		// Bind Moniker to a filter object
		hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
		if (FAILED(hr))
		{
			MessageBox(_T("Couldn't bind moniker to filter object!"), _T("Error"), MB_OK | MB_ICONERROR);
			return hr;
		}

		VARIANT var;
		IPropertyBag* props;
		HRESULT h2;
		h2 = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&props);
		if (SUCCEEDED(h2)) {
			VariantInit(&var);
			var.vt = VT_BSTR;
			h2 = props->Read(L"FriendlyName", &var, 0);
			if (SUCCEEDED(h2))
				printf("%d:%ls\n", 0, var.bstrVal);
			SysFreeString(var.bstrVal);
			VariantClear(&var);
		}
		SAFE_RELEASE(props);
	}

	// Copy the found filter pointer to the output parameter.
	if (SUCCEEDED(hr))
	{
		m_vcam_renderer = pSrc;
		m_vcam_renderer->AddRef();
	}

	SAFE_RELEASE(pSrc);
	SAFE_RELEASE(pMoniker);
	SAFE_RELEASE(pDevEnum);
	SAFE_RELEASE(pClassEnum);

	// get [IVCamRender] interface from VCam Renderer filter
	if (FAILED(hr = m_vcam_renderer->QueryInterface(IID_PPV_ARGS(&m_propertySet)))) {
		MessageBox(_T("Avshws driver not installed!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	/* Check the State property */
	DWORD supportFlags = 0;
	hr = m_propertySet->QuerySupported(GUID_PROP_CLASS, PROP_STATE_ID, &supportFlags);
	if (!SUCCEEDED(hr))
	{
		MessageBox(_T("The STATE property of Avshws driver not supported!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	if ((supportFlags & KSPROPERTY_SUPPORT_SET) != KSPROPERTY_SUPPORT_SET)
	{
		MessageBox(_T("The STATE property of Avshws driver not set!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	DWORD state = 0;
	hr = m_propertySet->Get(GUID_PROP_CLASS, PROP_STATE_ID, NULL, 0, &state, sizeof(DWORD), NULL);
	if (!SUCCEEDED(hr))
	{
		MessageBox(_T("It can't get the STATE property of the Avshws driver!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	/* Check the Process property */
	supportFlags = 0;
	hr = m_propertySet->QuerySupported(GUID_PROP_CLASS, PROP_PROCESS_ID, &supportFlags);
	if (!SUCCEEDED(hr))
	{
		MessageBox(_T("The PROCESS property of Avshws driver not supported!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	if ((supportFlags & KSPROPERTY_SUPPORT_SET) != KSPROPERTY_SUPPORT_SET)
	{
		MessageBox(_T("The PROCESS property of Avshws driver not set!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	DWORD process = 0;
	hr = m_propertySet->Get(GUID_PROP_CLASS, PROP_PROCESS_ID, NULL, 0, &process, sizeof(DWORD), NULL);
	if (!SUCCEEDED(hr))
	{
		MessageBox(_T("It can't get the PROCESS property of the Avshws driver!"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	// create a thread to detect VCam usage.
	if (m_propertySet) {
		DetectCameraUsageForAvshws();
	}

	return hr;
}

//
// When dialog quit, release camera resource
//
void CCameraWatcherDlg::CleanCameraForVCam()
{
	// set an Empty Event Handle for VCam usage 
	m_notification_monitor = FALSE;
	if (m_vcam)	m_vcam->SetConnectionNotificationEvent(reinterpret_cast<__int64>(nullptr));

	if (m_vcam_renderer) m_vcam_renderer->Release(), m_vcam_renderer = nullptr;
	if (m_vcam) m_vcam->Release(), m_vcam = nullptr;
}

void CCameraWatcherDlg::CleanCamerasForAvshws()
{
	// set an Empty Event Handle for VCam usage 
	m_notification_monitor = FALSE;
	SAFE_RELEASE(m_vcam_renderer);
	SAFE_RELEASE(m_propertySet);
}

void CCameraWatcherDlg::DetectCameraUsageForVCam()
{
	ShowUsingInfoForVCam();

	// create a thread to detect vcam usage
	m_notification_monitor = TRUE;
	m_thread = CreateThread(nullptr, 0, notification_usage__proc_vcam, this, 0, nullptr);
}

DWORD WINAPI CCameraWatcherDlg::notification_usage__proc_vcam(LPVOID data)
{
	CCameraWatcherDlg* impl = reinterpret_cast<CCameraWatcherDlg*>(data);
	impl->usage_proc_vcam();
	return 0;
}

void CCameraWatcherDlg::usage_proc_vcam()
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
			ShowUsingInfoForVCam();
	}
}

void CCameraWatcherDlg::ShowUsingInfoForVCam()
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

void CCameraWatcherDlg::DetectCameraUsageForAvshws()
{
	ShowUsingInfoForAvshws();

	// create a thread to detect vcam usage
	m_notification_monitor = TRUE;
	m_thread = CreateThread(nullptr, 0, notification_usage__proc_avshws, this, 0, nullptr);
}

DWORD WINAPI CCameraWatcherDlg::notification_usage__proc_avshws(LPVOID data)
{
	CCameraWatcherDlg* impl = reinterpret_cast<CCameraWatcherDlg*>(data);
	impl->usage_proc_avshws();
	return 0;
}

void CCameraWatcherDlg::usage_proc_avshws()
{
	DWORD old_state = 0;
	m_propertySet->Get(GUID_PROP_CLASS, PROP_STATE_ID, NULL, 0, &old_state, sizeof(DWORD), NULL);

	HRESULT hr = S_OK;
	DWORD current_state = old_state;
	while (m_notification_monitor) {
		::Sleep(10);

		if (m_propertySet == NULL) {
			break;
		}

		hr = m_propertySet->Get(GUID_PROP_CLASS, PROP_STATE_ID, NULL, 0, &current_state, sizeof(DWORD), NULL);

		if (!SUCCEEDED(hr)) {
			break;
		}

		// update using information
		if (current_state != old_state) {
			ShowUsingInfoForAvshws();
			old_state = current_state;
		}
	}
}

void CCameraWatcherDlg::ShowUsingInfoForAvshws()
{
	if (m_propertySet == nullptr)
		return;

	DWORD state = HardwareStopped;
	m_propertySet->Get(GUID_PROP_CLASS, PROP_STATE_ID, NULL, 0, &state, sizeof(DWORD), NULL);

	CString tt = _T("No application is ");
	if (state == HardwareRunning) tt = _T("An application (running) is ");
	else if (state == HardwarePaused) tt = _T("An application (pause) is ");

	CString message = tt + _T("using our camera.");
	SetDlgItemText(IDC_STATIC_VCAM_USAGE, message);
}
