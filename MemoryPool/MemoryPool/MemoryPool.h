#pragma once

#include <Windows.h>
#include <new>
/*
Object Pool (FreeList����) �� Memory Pool

�����̴� �˰��� ��ü�� Stack�� �����.
Lock-Free ����
*/

#define VALIDCODE (DWORD)(0x77777777)

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

		st_BLOCK_NODE *pNextBlock;
	};

	struct st_Top
	{
		st_BLOCK_NODE *_TopNode;
		__int64		   UniqueCount;
	};

#pragma pack(pop)
public:
	//////////////////////////////////
	// ������
	// int - �� ����
	// bool - ��� ������ ȣ�⿩��(�⺻�� = FALSE)
	//////////////////////////////////
	CMemoryPool(int blockSize = 0, bool bConst = false);
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
	bool m_bUseConstruct;
	bool m_Fixed;

	LONG64 m_iBlockSize;
	LONG64 m_iAllocCount;
	LONG64 m_iUnique;

	//st_BLOCK_NODE *pTop;
	st_Top *pTop;
	st_BLOCK_NODE *pTail;

	CLinkedList<st_BLOCK_NODE *> List;

};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class DATA>
CMemoryPool<DATA>::CMemoryPool(int blockSize, bool bConst)
	: m_iBlockSize(blockSize), m_bUseConstruct(bConst)
{
	//InitializeCriticalSection(&m_CrticalSection);

	m_iAllocCount = 0;

	st_BLOCK_NODE *pNewNode = nullptr;
	st_BLOCK_NODE *pOldNode = nullptr;

	if (blockSize > 1)
		m_Fixed = true;
	else
		blockSize = 5;

	pNewNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE));
	memset(pNewNode, 0, sizeof(st_BLOCK_NODE));
	pNewNode->ValidCode = VALIDCODE;
	List.push_back(pNewNode);
	pOldNode = pNewNode;


	pTop = (st_Top *)_aligned_malloc(128, 16);
	pTop->_TopNode = pNewNode;
	pTop->UniqueCount = 0;

	for (int i = 1; i <= blockSize; i++)
	{
		pNewNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE));
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
	//DeleteCriticalSection(&m_CrticalSection);

	auto iter = List.begin();

	for (; iter != List.end();)
	{
		st_BLOCK_NODE *pNode = (*iter);
		List.erase(iter);
		free(pNode);
	}

	_aligned_free(pTop);

}
// ������ ABA������ ���� ������ �Ȼ����.
// 
template <class DATA>
DATA* CMemoryPool<DATA>::Alloc(void)
{
	//EnterCriticalSection(&m_CrticalSection);

	/////////////////////////////////////////////////////////////////////////////////////////////
	//DATA *ret = &pTop->Data;
	//pTop->bAlloc = true;
	//pTop = pTop->pNextBlock;
	// �� ������ �����庰 �ѹ��� �����Ҽ� �ְ� ó���ؾ� �ȴ�.
	st_Top OldTop;
	st_BLOCK_NODE *pNewNode = nullptr;
	DATA *ret = nullptr;
	LONG64 Unique = InterlockedIncrement64(&m_iUnique);

	while (1)
	{
		OldTop._TopNode = pTop->_TopNode;
		OldTop.UniqueCount = pTop->UniqueCount;

		if (m_iAllocCount > m_iBlockSize)
		{
			if (m_Fixed)
				continue;

			for (int i = 0; i < 5; i++)
			{
				st_BLOCK_NODE *pNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE));
				pNode->ValidCode = VALIDCODE;
				pNode->pNextBlock = nullptr;

				pTail->pNextBlock = pNode;
				pTail = pNode;

				//m_iBlockSize++;
				InterlockedIncrement64(&m_iBlockSize);
			}
		}

		pNewNode = OldTop._TopNode->pNextBlock;
		
		// �ٸ� ������� �浹�ߴ°�? -> ���н� �õ������� ���ư���.
		// �浹�� ���� = OldPop�� pPop�� �� ������ ����°�? -> �̿��� ���� ������� �ʰԲ� �ؾߵȴ�.
		// ���� Alloc�Ȱ� �ٽ� Alloc�ǰ� �ִ�.
		if (InterlockedCompareExchange128((LONG64 *)pTop, (LONG64)Unique, (LONG64)pNewNode, (LONG64 *)&OldTop))
		{
			InterlockedIncrement64(&m_iAllocCount);
			return &OldTop._TopNode->Data;
			break;
		}
		else
			OldTop = { 0 };
		
		/////////////////////////////////////////////////////////////////////////////////////////////
	}
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

	// �� ������ �����庰 �ѹ��� �����Ҽ� �ְ� ó���ؾ� �ȴ�.
	st_Top OldTop;
	LONG64 Unique = InterlockedIncrement64(&m_iUnique);

	while(1)
	{
		// �ڷᱸ���� �����Ѵ�.
		//Unique = InterlockedIncrement64(&pPop->UniqueCount);
		OldTop._TopNode = pTop->_TopNode;
		OldTop.UniqueCount = pTop->UniqueCount;

		if (OldTop._TopNode != pDel->pNextBlock && OldTop._TopNode != pDel)
			pDel->pNextBlock = OldTop._TopNode;
		
		// �ٸ� ������� �浹�ߴ°�? -> ���н� �õ������� ���ư���.
		// �浹�� ���� = OldPop�� pPop�� �� ������ ����°�? -> �̿��� ���� ������� �ʰԲ� �ؾߵȴ�. => Lock Free ȿ���� ���ؼ�
		// ���� ������� Wait-Free��������.
		if (InterlockedCompareExchange128((LONG64 *)pTop, (LONG64)Unique, (LONG64)pDel, (LONG64 *)&OldTop))
		{
			InterlockedDecrement64(&m_iAllocCount);
			return true;
		}
		else
			OldTop = { 0 };
	}
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