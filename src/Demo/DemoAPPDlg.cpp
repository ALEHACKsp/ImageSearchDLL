#include "stdafx.h"
#include "DemoAPP.h"
#include "DemoAPPDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// DemoAppDlg �Ի���



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


// DemoAppDlg ��Ϣ�������

BOOL DemoAppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void DemoAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR DemoAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CString * SplitString(CString str, char split, int& iSubStrs)
{
	int iPos = 0; //�ָ��λ��
	int iNums = 0; //�ָ��������
	CString strTemp = str;
	CString strRight;
	//�ȼ������ַ���������
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
	if (iNums == 0) //û���ҵ��ָ��
	{
		//���ַ����������ַ�������
		iSubStrs = 1;
		return NULL;
	}
	//���ַ�������
	iSubStrs = iNums + 1; //�Ӵ������� = �ָ������ + 1
	CString* pStrSplit;
	pStrSplit = new CString[iSubStrs];
	strTemp = str;
	CString strLeft;
	for (int i = 0; i < iNums; i++)
	{
		iPos = strTemp.Find(split);
		//���Ӵ�
		strLeft = strTemp.Left(iPos);
		//���Ӵ�
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
