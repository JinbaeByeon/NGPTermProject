#pragma once

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

using namespace std;
using namespace FMOD;

class CStringCmp
{
private:
	const TCHAR*		m_pName;

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