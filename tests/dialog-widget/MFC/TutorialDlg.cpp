#include "stdafx.h"
#include "TutorialApp.h"
#include "TutorialDlg.h"

// DEBUG_NEW macro allows MFC applications to determine memory leak locations in debug builds
#ifdef _DEBUG
  #define new DEBUG_NEW
#endif


CTutorialDlg::CTutorialDlg(CWnd* pParent)
: CDialog(CTutorialDlg::IDD, pParent)
, m_EchoText(L"")
, m_MouseEcho(L"")
, m_TimerEcho(L"")
, m_TimerCtrlSliders(TRUE)
, m_OkCount(0)
, m_Seconds(0)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTutorialDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_ECHO_AREA, m_EchoText);
  DDX_Control(pDX, IDC_V_SLIDER_BAR, m_VSliderBar);
  DDX_Control(pDX, IDC_H_SLIDER_BAR, m_HSliderBar);
  DDX_Text(pDX, IDC_MOUSEECHO, m_MouseEcho);
  DDX_Text(pDX, IDC_TIMERECHO, m_TimerEcho);
  DDX_Check(pDX, IDC_TIMER_CONTROL_SLIDERS, m_TimerCtrlSliders);
}

BEGIN_MESSAGE_MAP(CTutorialDlg, CDialog)
  ON_WM_PAINT()
  ON_BN_CLICKED(ID_BTN_ADD, OnBnClickedBtnAdd)
  ON_WM_TIMER()
  ON_WM_LBUTTONDOWN()
  ON_WM_MOUSEMOVE()
  ON_WM_RBUTTONDOWN()
  ON_BN_CLICKED(IDC_TIMER_CONTROL_SLIDERS, OnBnClickedTimerControlSliders)
END_MESSAGE_MAP()

// This is called when the dialog is first created and shown.
// It is a good spot to initialize member variables.
BOOL CTutorialDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);   // Set big icon
  SetIcon(m_hIcon, FALSE);  // Set small icon

  // Add extra initialization here.
  // We want to initialize the slider bars
  m_VSliderBar.Initialize(0.0f, 100.0f, 50.0f);
  m_HSliderBar.Initialize(0.0f, 10.0f, 5.0f);

  // Initialize the timer to go off every 1000 milliseconds (every second)
  // when timer "goes-off", our OnTimer() event handler function will be
  // called and it is upto us to decide what we want to do.
  SetTimer(0, 1000, NULL);

  UpdateData(false);
	
  return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
// to draw the icon.  For MFC applications using the document/view model,
// this is automatically done for you by the framework.
void CTutorialDlg::OnPaint() 
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
    CDialog::OnPaint();
  }
}

void CTutorialDlg::OnBnClickedBtnAdd()
{
  m_OkCount++;
  m_EchoText.Format(L"%d", m_OkCount);

  // Notice, without UpdateData() status area will _NOT_ be updated.
  UpdateData(FALSE);
}

void CTutorialDlg::OnTimer(UINT nIDEvent)
{
  m_Seconds++;

  if (m_TimerCtrlSliders) 
  {
    // Get ready to decrease the sliders ...
    float hvalue = m_HSliderBar.GetSliderValue();
    if (hvalue > 0) 
      m_HSliderBar.SetSliderValue(hvalue-1);

    float vvalue = m_VSliderBar.GetSliderValue();
    if (vvalue > 0) 
      m_VSliderBar.SetSliderValue(vvalue-1);
    
    if ( (hvalue<=0) && (vvalue<=0) )
      m_TimerCtrlSliders = false;
  }

  m_TimerEcho.Format(L"%d: Seconds have passed", m_Seconds);
  UpdateData(false);
}

void CTutorialDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
  CString prefix;
  if(nFlags & MK_CONTROL)
    prefix = L"[CTRL]";
  if(nFlags & MK_SHIFT)
    prefix+= L"[SHIFT]";
  m_MouseEcho.Format(L"%sLeft mouse down at %d,%d", prefix, point.x, point.y);
  UpdateData(false);
}

void CTutorialDlg::OnMouseMove(UINT nFlags, CPoint point)
{
  CString prefix;
  if(nFlags & MK_CONTROL)
    prefix = L"[CTRL]";
  if(nFlags & MK_SHIFT)
    prefix+= L"[SHIFT]";
  m_MouseEcho.Format(L"%sMouse move at %d,%d", prefix, point.x, point.y);
  UpdateData(false);
}

void CTutorialDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
  CString prefix;
  if(nFlags & MK_CONTROL)
    prefix = L"[CTRL]";
  if(nFlags & MK_SHIFT)
    prefix+= L"[SHIFT]";
  m_MouseEcho.Format(L"%sRight mouse down at %d,%d", prefix, point.x, point.y);
  UpdateData(false);
}

void CTutorialDlg::OnBnClickedTimerControlSliders()
{
  UpdateData(true);
  // This will fill all UI-connected variables with whatever 
  // value that is showing on the UI control objects.
  // 
  // In this case, we care most about the value for m_TimerCtrlSliders
}
