#include "stdafx.h"
#include "DemoAPP.h"
#include "DemoAPPDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// DemoAppDlg 对话框



DemoAppDlg::DemoAppDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MFCAPPLICATION2_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void DemoAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_logEdit);
}

BEGIN_MESSAGE_MAP(DemoAppDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &DemoAppDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// DemoAppDlg 消息处理程序

BOOL DemoAppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void DemoAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR DemoAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CString * SplitString(CString str, char split, int& iSubStrs)
{
	int iPos = 0; //分割符位置
	int iNums = 0; //分割符的总数
	CString strTemp = str;
	CString strRight;
	//先计算子字符串的数量
	while (iPos != -1)
	{
		iPos = strTemp.Find(split);
		if (iPos == -1)
		{
			break;
		}
		strRight = strTemp.Mid(iPos + 1, str.GetLength());
		strTemp = strRight;
		iNums++;
	}
	if (iNums == 0) //没有找到分割符
	{
		//子字符串数就是字符串本身
		iSubStrs = 1;
		return NULL;
	}
	//子字符串数组
	iSubStrs = iNums + 1; //子串的数量 = 分割符数量 + 1
	CString* pStrSplit;
	pStrSplit = new CString[iSubStrs];
	strTemp = str;
	CString strLeft;
	for (int i = 0; i < iNums; i++)
	{
		iPos = strTemp.Find(split);
		//左子串
		strLeft = strTemp.Left(iPos);
		//右子串
		strRight = strTemp.Mid(iPos + 1, strTemp.GetLength());
		strTemp = strRight;
		pStrSplit[i] = strLeft;
	}
	pStrSplit[iNums] = strTemp;
	return pStrSplit;
}


void DemoAppDlg::OnBnClickedButton1()
{
	HMODULE hModule = LoadLibrary(TEXT("ImageSearchDLL.dll"));
	
	typedef  char*  (WINAPI *ImageSearchFunc)(__int32 x1, __int32 y1, __int32 right, __int32 buttom, char* filename ,__int32 pHBMP);
	ImageSearchFunc ImageSearch = (ImageSearchFunc)GetProcAddress(hModule, "ImageSearch");
	int err = GetLastError();

	char* res = ImageSearch(0, 0, 1366, 768, "*32 test.bmp", NULL);
	
	int substrCount = 0;
	CString *result = SplitString(CString(res),'|', substrCount);

	m_logStr += L"Search test.bmp from Screen ... \r\n";
	m_logStr +=  L"Function return : ";
	m_logStr += res;
	m_logStr += L"\r\nThe return means that : ";
	if (NULL == result) {
		m_logStr += L"test.bmp Not Found In the Screen";
	}else {
		m_logStr += L"test.bmp Found In the Screen. \r\nX coordinates:";
		m_logStr += result[1];
		m_logStr += L"\r\nY coordinates:";
		m_logStr += result[2];
		m_logStr += L"\r\nImage width:";
		m_logStr += result[3];
		m_logStr += L"\r\nImage height:";
		m_logStr += result[4];
		m_logStr += L"\r\n";
		delete[] result;
	}
	m_logEdit.SetWindowText(m_logStr.GetBuffer());
	m_logEdit.LineScroll(m_logEdit.GetLineCount());

}
