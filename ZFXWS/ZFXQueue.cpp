#include "ZFXQueue.h"
#include <cstddef>
#include <memory>

ZFXQueue::ZFXQueue()
{
	m_pTail = NULL;
	m_pHead = NULL;
	m_Count = 0;
}

ZFXQueue::~ZFXQueue()
{
	while (m_pHead)
	{
		Dequeue();
	}
	m_pTail = NULL;
	m_pHead = NULL;
	m_Count = 0;
}

void ZFXQueue::Enqueue(const void* pData, unsigned int nSize)
{
	ZFXQueueElem* pNew = new ZFXQueueElem((const char*)pData, nSize);

	// this is the first element
	if (m_Count == 0) {
		m_pHead = pNew;
		m_pTail = pNew;
	}
	// there is already one element
	else if (m_Count == 1) {
		m_pHead->m_pNext = pNew;
		m_pTail = pNew;
	}
	else {
		m_pTail->m_pNext = pNew;
		m_pTail = pNew;
	}
	++m_Count;
}	//	Enqueue

void ZFXQueue::Dequeue(void)
{
	ZFXQueueElem* pTemp;

	// already empty
	if (m_Count == 0) return;

	// one element left
	else if (m_Count == 1) {
		delete m_pHead;
		m_pHead = NULL;
		m_pTail = NULL;
	}
	else {
		pTemp = m_pHead;
		m_pHead = m_pHead->m_pNext;
		delete pTemp;
	}
	--m_Count;
}	//	Dequeue

void ZFXQueue::Front(void* pData, bool bDequeue)
{
	if (pData) {
		if (m_pHead) {
			memcpy(pData, m_pHead->m_pData, m_pHead->m_nSize);
		}
	}
	if (bDequeue)
		Dequeue();
}	//	Front

ZFXQueueElem::ZFXQueueElem(const char* pData, unsigned int nSize)
{
	m_pData = NULL;
	m_pNext = NULL;
	m_pData = new char[nSize];
	m_nSize = nSize;
	memcpy(m_pData, pData, nSize);
}	//	constructor
/*---------------------------------------------------------*/

ZFXQueueElem::~ZFXQueueElem(void)
{
	if (m_pData) {
		delete[] m_pData;
		m_pData = NULL;
	}
	m_pNext = NULL;
}	//	destructor