#pragma once

#include <Windows.h>
#include <new>


#define VALIDCODE (DWORD)(0x77777777)

#define dfERR_NOTFREE 2080
template <class T>
class CLinkedList
{
public:
	struct Node
	{
		T _Data;
		Node *prev;
		Node *next;
	};

	class iterator
	{
	private:
		Node * pos;

	public:
		iterator(Node *_node = nullptr)
		{
			pos = _node;
		}

		iterator& operator++(int)
		{
			pos = pos->next;
			return *this;
		}

		iterator& operator--()
		{
			pos = pos->prev;
			return *this;
		}

		bool operator!=(iterator& iter)
		{
			if (this->pos != iter.pos)
				return true;
			return false;
		}

		bool operator==(iterator& iter)
		{
			if (this->pos == iter.pos)
				return true;
			return false;
		}

		T& operator*()
		{
			if (pos != nullptr)
			{
				return pos->_Data;
			}
		}
	};

	CLinkedList()
	{
		_size = 0;

		head.next = nullptr;
		head.next = &tail;
		tail.next = nullptr;
		tail.prev = &head;
	}

public:
	iterator begin()
	{
		iterator iter(head.next);
		return iter;
	}

	iterator end()
	{
		iterator iter(&tail);
		return iter;
	}

	/*
	- ���ͷ������� �� ��带 ����.
	- �׸��� ���� ����� ���� ��带 ī��Ű�� ���ͷ����� ����
	���� �� �Լ��� ����׸� ���� ����.
	*/
	iterator& erase(iterator &iter)
	{
		Delete(*iter);
		iter++;
		return iter;
	}


	void push_back(T Data)
	{
		Node *newNode = new Node;
		newNode->_Data = Data;

		newNode->prev = tail.prev;
		newNode->next = &tail;

		newNode->prev->next = newNode;
		tail.prev = newNode;

		_size++;
	}

	void push_front(T Data)
	{
		Node *newNode = (Node *)malloc(sizeof(Node));
		newNode->_Data = Data;

		newNode->prev = &head;
		newNode->next = head.next;

		newNode->next->prev = newNode;
		Head.prev = newNode;

		_size++;
	}

	T& GetLastData()
	{
		return tail.prev->_Data;
	}

	T pop_back()
	{
		T ret = tail.prev->_Data;
		Delete(tail.prev);
		return ret;
	}
	T pop_front()
	{
		T ret = head.next->_Data;
		Delete(head->next);
		return ret;
	}

	int size()
	{
		return _size;
	}
	void clear()
	{
		Node *pStart = head.next;

		while (pStart != &tail)
		{
			Node *delNode = pStart;

			pStart->prev->next = pStart->next;
			pStart->next->prev = pStart->prev;

			_size--;

			pStart = pStart->next;
			delete delNode;
		}
	}

private:
	int _size;
	Node head;
	Node tail;

	void Delete(T Data)
	{
		Node *pStart = head.next;

		while (pStart->next != nullptr) // pStart != &Tail
		{
			if (pStart->_Data == Data)
			{
				Node *delNode = pStart;

				pStart->prev->next = pStart->next;
				pStart->next->prev = pStart->prev;
				_size--;
				delete delNode;
				return;
			}
			else
				pStart = pStart->next;

		}
	}
};

template <class DATA>
class CMemoryPool
{
private:
#pragma pack(push, 1)
	struct st_BLOCK_NODE
	{
		DWORD ValidCode;
		DATA Data;
		bool bAlloc;

		st_BLOCK_NODE *pNextBlock;
	};
#pragma pack(pop)
public:
	//////////////////////////////////
	// ������
	// int - �� ����
	// bool - ��� ������ ȣ�⿩��(�⺻�� = FALSE)
	//////////////////////////////////
	CMemoryPool(int blockSize = 10000, bool bConst = false);
	virtual ~CMemoryPool();


	//////////////////////////////////
	// ��� �ϳ��� �Ҵ����ִ� �Լ� -> new ��������� �Ѵٸ� �Ѵ�.
	// ���� : Ư�� ����� ���� ������ ����
	//////////////////////////////////
	DATA* Alloc(void);

	//////////////////////////////////
	// ������� ����� ��ȯ�ϴ� �Լ�
	// �Ķ���� : ������� ������ �ּҰ�-> �Ҹ��� ȣ���ؾ� �ȴٸ� �ϰ� ���Ѵٸ� �׳� ��ȯ
	// ���� : ��������
	//////////////////////////////////
	bool Free(DATA *pData); // �׷��ٸ� �ܺο��� �� �Լ��� ���� ��ȯ�ϰ�, ���߿� �� �ּҰ��� ����Ϸ��� �Ѵٸ�? -> ����


	//////////////////////////////////
	// �� Ȯ���� ����� ���� ����
	//////////////////////////////////
	int GetBlockCount(void);



	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// �Ķ����: ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int GetAllocCount(void);


private:
	// ������ �Ҵ緮
	int m_iBlockSize;
	bool m_bUseConstruct;

	int m_iAllocCount;



	st_BLOCK_NODE *pTop;
	st_BLOCK_NODE *pTail;

	CLinkedList<st_BLOCK_NODE *> List;

	CRITICAL_SECTION m_CrticalSection;

};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class DATA>
CMemoryPool<DATA>::CMemoryPool(int blockSize, bool bConst)
	:m_iBlockSize(blockSize), m_bUseConstruct(bConst)
{
	InitializeCriticalSection(&m_CrticalSection);

	m_iAllocCount = 0;
	pTop = nullptr;

	st_BLOCK_NODE *pNewNode = nullptr;
	st_BLOCK_NODE *pOldNode = nullptr;

	pNewNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE));
	memset(pNewNode, 0, sizeof(st_BLOCK_NODE));
	pNewNode->ValidCode = VALIDCODE;
	List.push_back(pNewNode);
	pOldNode = pNewNode;

	for (int i = 1; i < blockSize; i++)
	{
		pNewNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE));
		pNewNode->bAlloc = false;
		pOldNode->pNextBlock = pNewNode;
		pNewNode->pNextBlock = nullptr;
		pNewNode->ValidCode = VALIDCODE;

		List.push_back(pNewNode);

		pOldNode = pNewNode;
	}

	pTail = pNewNode;
}

template <class DATA>
CMemoryPool<DATA>::~CMemoryPool()
{
	DeleteCriticalSection(&m_CrticalSection);

	auto iter = List.begin();

	for (; iter != List.end();)
	{
		st_BLOCK_NODE *pNode = (*iter);
		List.erase(iter);
		free(pNode);
	}

}

template <class DATA>
DATA* CMemoryPool<DATA>::Alloc(void)
{
	EnterCriticalSection(&m_CrticalSection);

	if (pTop == nullptr)
		pTop = *List.begin();  // �� �ִ� �� ó�� Alloc���������� �۵��ϰԲ� �Ѵ�.


	/////////////////////////////////////////////////////////////////////
	// ������
	// AllocCount�� ���ī��Ʈ �� ��
	/////////////////////////////////////////////////////////////////////
	if (m_iAllocCount >= m_iBlockSize - 5)
	{
		// ��带 ���� �����Ѵ�.
		// ������ ��ġ��?  pTail;
		for (int i = 0; i < 5; i++)
		{
			st_BLOCK_NODE *pNewNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE));
			pNewNode->ValidCode = VALIDCODE;
			pNewNode->pNextBlock = nullptr;

			pTail->pNextBlock = pNewNode;
			pTail = pNewNode;

			m_iBlockSize++;
		}
	}

	// pTop�� �����������͸� �����Ҽ� �ְ� �Ѵ�.
	DATA *ret = &pTop->Data;

	pTop->bAlloc = true;
	// pTop�� �������� �ѱ��.
	pTop = pTop->pNextBlock;

	m_iAllocCount++;

	LeaveCriticalSection(&m_CrticalSection);

	return ret;
}
// DATA pData�� st_BLOCK_NODE pTop���� ó���ؾߵȴ�.
// 1. pData�� not null�϶�
// 2. pTop�� next�� pData��
// 3. pData�� next�� pTop�� next��
// 4. ġȯ
template <class DATA>
bool CMemoryPool<DATA>::Free(DATA *pData)
{
	st_BLOCK_NODE *pDel = (st_BLOCK_NODE *)((DWORD *)pData - 1);

	if (m_iAllocCount == 0)
		return true;

	if (pDel->ValidCode != VALIDCODE)
		return false;


	EnterCriticalSection(&m_CrticalSection);

	if (pTop == pDel->pNextBlock || pTop == pDel)
		pTop = pDel;

	else
	{
		st_BLOCK_NODE *T = pTop->pNextBlock;


		pDel->pNextBlock = T;
		pTop->pNextBlock = pDel;
	}

	pDel->bAlloc = false;


	m_iAllocCount--;
	LeaveCriticalSection(&m_CrticalSection);


	return true;
}

template <class DATA>
int CMemoryPool<DATA>::GetBlockCount(void)
{
	return m_iBlockSize;
}

template <class DATA>
int CMemoryPool<DATA>::GetAllocCount(void)
{
	return m_iAllocCount;
}