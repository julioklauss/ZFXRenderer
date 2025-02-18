#pragma once

class ZFXQueueElem
{
	public:
		ZFXQueueElem(const char* pData, unsigned int nSize);
		~ZFXQueueElem(void);

		ZFXQueueElem*	m_pNext;
		char*			m_pData;
		unsigned int	m_nSize;
};

class ZFXQueue
{
	private:
		ZFXQueueElem*	m_pHead;
		ZFXQueueElem*	m_pTail;
		unsigned int	m_Count;

	public:
		ZFXQueue(void);
		~ZFXQueue(void);

		void Dequeue(void);
		void Enqueue(const void* pData, unsigned int nSize);
		void Front(void* pData, bool bDequeue);

		unsigned int GetCount(void) { return m_Count; }

		unsigned int GetFrontSize(void) { if (m_pHead) { return m_pHead->m_nSize; } else { return 0; } }
};