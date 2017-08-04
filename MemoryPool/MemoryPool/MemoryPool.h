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
	- 이터레이터의 그 노드를 지움.
	- 그리고 지운 노드의 다음 노드를 카리키는 이터레이터 리턴
	아직 이 함수는 디버그를 하지 못함.
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
	struct st_BLOCK_NODE
	{
		DWORD ValidCode;
		DATA Data;

		st_BLOCK_NODE *pNextBlock;
	};
public:
	//////////////////////////////////
	// 생성자
	// int - 블럭 갯수
	// bool - 블록 생성자 호출여부(기본값 = FALSE)
	//////////////////////////////////
	CMemoryPool(int blockSize = 1, bool bConst = false);
	virtual ~CMemoryPool();


	//////////////////////////////////
	// 블록 하나를 할당해주는 함수 -> new 선언해줘야 한다면 한다.
	// 리턴 : 특정 블록의 공간 포인터 리턴
	//////////////////////////////////
	DATA* Alloc(void);

	//////////////////////////////////
	// 사용중인 블록을 반환하는 함수
	// 파라미터 : 사용중인 데이터 주소값-> 소멸자 호출해야 된다면 하고 안한다면 그냥 반환
	// 리턴 : 성공여부
	//////////////////////////////////
	bool Free(DATA *pData); // 그렇다면 외부에서 이 함수를 통해 반환하고, 나중에 이 주소값을 사용하려고 한다면? -> 주의


							//////////////////////////////////
							// 총 확보된 블록의 갯수 리턴
							//////////////////////////////////
	int GetBlockCount(void);



	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// 파라미터: 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int GetAllocCount(void);


private:
	// 생성시 할당량
	int m_iBlockSize;
	bool m_bUseConstruct;

	int m_iAllocCount;



	st_BLOCK_NODE *pTop;
	st_BLOCK_NODE *pTopOneBefore;
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
		memset(pNewNode, 0, sizeof(st_BLOCK_NODE));

		pOldNode->pNextBlock = pNewNode;
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

	if (m_bUseConstruct)
	{
		auto iter = List.begin();

		for (; iter != List.end();)
		{
			st_BLOCK_NODE *pNode = (*iter);
			pNode->Data.~DATA();
			List.erase(iter);
			free(pNode);
		}
	}
}

template <class DATA>
DATA* CMemoryPool<DATA>::Alloc(void)
{
	EnterCriticalSection(&m_CrticalSection);
	if (pTop == nullptr)
		pTop = *List.begin();


	if (m_iAllocCount >= m_iBlockSize - 500)
	{
		while (pTail->pNextBlock != nullptr)
			pTail = pTail->pNextBlock;

		for (int i = 0; i < 1000; i++)
		{
			st_BLOCK_NODE *pNewNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE));
			memset(pNewNode, 0, sizeof(pNewNode));
			pNewNode->ValidCode = VALIDCODE;
			pNewNode->pNextBlock = nullptr;
			pTail->pNextBlock = pNewNode;
			List.push_back(pNewNode);

			pTail = pNewNode;

			m_iBlockSize++;
		}

	}

	if (m_bUseConstruct) // New Placement 실행
		new (&pTop->Data) DATA();

	DATA* ret = &pTop->Data;

	pTopOneBefore = pTop;
	pTop = pTop->pNextBlock;

	m_iAllocCount++;

	LeaveCriticalSection(&m_CrticalSection);

	return ret;
}
// DATA pData와 st_BLOCK_NODE pTop으로 처리해야된다.
// 1. pData가 not null일때
// 2. pTop의 next을 pData로
// 3. pData의 next를 pTop의 next로
// 4. 치환
template <class DATA>
bool CMemoryPool<DATA>::Free(DATA *pData)
{
	st_BLOCK_NODE *pDel = (st_BLOCK_NODE *)((DWORD *)pData - 1); // DWORD는 4바이트이므로 데이터 보다 4바이트 위로 올리면 구조체를 가르킬수가 있다.

	if (pDel->ValidCode != VALIDCODE)
		return false;

	if (pTop == nullptr || pTop == pDel)
		pTop = pTopOneBefore;

	EnterCriticalSection(&m_CrticalSection);

	if (m_iAllocCount > 1)
	{
		pTopOneBefore->pNextBlock = pDel;
		pDel->pNextBlock = pTop;
	}

	pTop = pDel;

	if (m_bUseConstruct)
		pDel->Data.~DATA();

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