#include "StdAfx.h"
#include "SoundMgr.h"

CSoundMgr* CSoundMgr::m_pInstance = NULL;

CSoundMgr::CSoundMgr(void)
{
	System = NULL;
	m_iVersion = 0;

	Initialize();
}

CSoundMgr::~CSoundMgr(void)
{
}


void CSoundMgr::Initialize(void)
{
	Result = FMOD_System_Create(&System);
	ErrorCheck(Result);

	Result = FMOD_System_GetVersion(System, &m_iVersion);
	ErrorCheck(Result);

	Result = FMOD_System_Init(System, 32, FMOD_INIT_NORMAL, NULL);
	ErrorCheck(Result);
}

void CSoundMgr::ErrorCheck(FMOD_RESULT _result)
{
	if (_result != FMOD_OK)
	{
		cout << "오류" << endl;
		return;
	}
}


void CSoundMgr::LoadSoundFile(void)
{
	_finddata_t fd;

	long handle;
	int iResult = 1;

	handle = _findfirst("Sound\\*.*", &fd);		//해당경로 모두 읽어라.

	if (handle == -1)
	{
		return;
	}

	while (iResult != -1)
	{
		TCHAR szName[256];

		ZeroMemory(szName, sizeof(szName));	//메크로 함수.

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
			fd.name, strlen(fd.name), szName, 256);
		//멀티바이트를 유니코드로 변경.

		TCHAR* pName = new TCHAR[256];
		ZeroMemory(pName, sizeof(TCHAR) * 256);
		lstrcpy(pName, szName);

		char szFullName[256];

		strcpy_s(szFullName, "Sound\\");
		strcat_s(szFullName, fd.name);


		FMOD_SOUND* pSound;

		Result = FMOD_System_CreateSound(System, szFullName, FMOD_HARDWARE, 0, &pSound);

		if (Result == FMOD_OK)
		{
			m_mapSound.insert(make_pair(pName, pSound));
		}
		else
		{
			delete[] pName;
		}

		iResult = _findnext(handle, &fd);		//모든탐색을 한다.

	}

	_findclose(handle);

	Result = FMOD_System_Update(System);

	ErrorCheck(Result);
}

void CSoundMgr::PlaySound(TCHAR* pSoundKey)
{
	map<TCHAR*, FMOD_SOUND*>::iterator iter;

	iter = find_if(m_mapSound.begin(), m_mapSound.end(), CStringCmp(pSoundKey));

	if (iter == m_mapSound.end())
		return;

	Result = FMOD_System_PlaySound(System, FMOD_CHANNEL_REUSE,
		iter->second, 0, &Effect);

	ErrorCheck(Result);
}

void CSoundMgr::PlayBGM(TCHAR* pSoundKey)
{
	map<TCHAR*, FMOD_SOUND*>::iterator iter;

	iter = find_if(m_mapSound.begin(), m_mapSound.end(), CStringCmp(pSoundKey));

	if (iter == m_mapSound.end())
		return;

	FMOD_Sound_SetMode(iter->second, FMOD_LOOP_NORMAL);

	Result = FMOD_System_PlaySound(System, FMOD_CHANNEL_REUSE,
		iter->second, 0, &BGM);

	ErrorCheck(Result);
}


void CSoundMgr::PlayEffectSound(TCHAR* pSoundKey)
{
	map<TCHAR*, FMOD_SOUND*>::iterator iter;

	iter = find_if(m_mapSound.begin(), m_mapSound.end(), CStringCmp(pSoundKey));

	if (iter == m_mapSound.end())
		return;

	Result = FMOD_System_PlaySound(System, FMOD_CHANNEL_REUSE,
		iter->second, 0, &Effect);

	ErrorCheck(Result);
}

void CSoundMgr::PlayEffectSound2(TCHAR* pSoundKey)
{
	map<TCHAR*, FMOD_SOUND*>::iterator iter;

	iter = find_if(m_mapSound.begin(), m_mapSound.end(), CStringCmp(pSoundKey));

	if (iter == m_mapSound.end())
		return;

	Result = FMOD_System_PlaySound(System, FMOD_CHANNEL_REUSE,
		iter->second, 0, &Effect2);

	ErrorCheck(Result);
}


void CSoundMgr::StopSoundAll(void)
{
	Result = FMOD_Channel_Stop(BGM);
	ErrorCheck(Result);

	Result = FMOD_Channel_Stop(Effect);
	ErrorCheck(Result);

	Result = FMOD_Channel_Stop(Skill);
	ErrorCheck(Result);


}

void CSoundMgr::StopBGMSound(void)
{
	Result = FMOD_Channel_Stop(BGM);
	ErrorCheck(Result);
}

