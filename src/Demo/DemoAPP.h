#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// DemoApp: 
// �йش����ʵ�֣������ MFCApplication2.cpp
//

class DemoApp : public CWinApp
{
public:
	DemoApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern DemoApp theApp;