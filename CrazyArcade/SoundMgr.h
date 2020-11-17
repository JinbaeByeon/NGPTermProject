#pragma once

//객체를 n개만 생성하도록 제한을 하는 디자인 패턴입니다.
class CSoundMgr
{
private:
	static CSoundMgr* m_pInstance;

public:
	static CSoundMgr* GetInstance(void)
	{
		if (m_pInstance == NULL)
		{
			m_pInstance = new CSoundMgr;
		}
		return m_pInstance;
	}

	static void DestroyInstance(void)
	{
		if (m_pInstance)
		{
			delete m_pInstance;
			m_pInstance = NULL;
		}
	}

	//Fmod 인터페이스
private:
	FMOD_SYSTEM*	System;			//fmod 시스템 포인터
	FMOD_CHANNEL*	Effect;
	FMOD_CHANNEL*	Effect2;
	FMOD_CHANNEL*	BGM;
	FMOD_CHANNEL*	Skill;
	FMOD_RESULT		Result;		//실행잘되는지 확인하는 변수.

	unsigned int m_iVersion;

	map<TCHAR*, FMOD_SOUND*>		m_mapSound;		//사운드 관리(map)

public:
	void Initialize(void);
	void LoadSoundFile(void);
	void PlaySound(TCHAR* pSoundKey);
	void PlayBGM(TCHAR* pSoundKey);
	void PlayEffectSound(TCHAR* pSoundKey);
	void PlayEffectSound2(TCHAR* pSoundKey);
	void StopSoundAll(void);
	void StopBGMSound(void);
	void StopEffectSound(void);
	void StopEffectSound2(void);

public:
	map<TCHAR*, FMOD_SOUND*>* GetSoundMap(void)
	{
		return &m_mapSound;
	}

private:
	void ErrorCheck(FMOD_RESULT _result);

private:
	CSoundMgr(void);
public:
	~CSoundMgr(void);
};
