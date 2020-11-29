#pragma once

// 서버 구성하기 위해 붙인 것
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#ifdef _DEBUG
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif


#include "fmod.h"
#include "fmod.hpp"
#include "fmod_dsp.h"
#include "fmod_errors.h"

#pragma comment (lib, "fmodex_vc.lib")

#include <Windows.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <algorithm>
#include <io.h>

#define MAX_PLAYER 4

using namespace std;
using namespace FMOD;

class CStringCmp
{
private:
	const TCHAR* m_pName;

public:
	explicit CStringCmp(const TCHAR* pKey)
		:m_pName(pKey) {}

public:
	template<typename T>
	bool operator () (T data)
	{
		return (!lstrcmp(data.first, m_pName));
	}
};