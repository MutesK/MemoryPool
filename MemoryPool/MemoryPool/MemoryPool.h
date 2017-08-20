#pragma once

#include <Windows.h>
#include <new>
/*
Object Pool (FreeList형식) 의 Memory Pool

움직이는 알고리즘 자체는 Stack과 비슷함.
Lock-Free 도입
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
	// 생성자
	// int - 블럭 갯수
	// bool - 블록 생성자 호출여부(기본값 = FALSE)
	//////////////////////////////////
	CMemoryPool(int blockSize = 0, bool bConst = false);
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
// 삽입은 ABA문제에 거의 문제가 안생긴다.
// 
template <class DATA>
DATA* CMemoryPool<DATA>::Alloc(void)
{
	//EnterCriticalSection(&m_CrticalSection);

	/////////////////////////////////////////////////////////////////////////////////////////////
	//DATA *ret = &pTop->Data;
	//pTop->bAlloc = true;
	//pTop = pTop->pNextBlock;
	// 이 구간은 스레드별 한번만 수행할수 있게 처리해야 된다.
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
		
		// 다른 스레드와 충돌했는가? -> 실패시 시도전으로 돌아간다.
		// 충돌의 기준 = OldPop과 pPop의 값 변경이 생겼는가? -> 이외의 값이 변경되지 않게끔 해야된다.
		// 현재 Alloc된게 다시 Alloc되고 있다.
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
// DATA pData와 st_BLOCK_NODE pTop으로 처리해야된다.
// 1. pData가 not null일때
// 2. pTop의 next을 pData로
// 3. pData의 next를 pTop의 next로
// 4. 치환
template <class DATA>
bool CMemoryPool<DATA>::Free(DATA *pData)
{
	st_BLOCK_NODE *pDel = (st_BLOCK_NODE *)((DWORD *)pData - 1);

	if (m_iAllocCount == 0)
		return true;

	if (pDel->ValidCode != VALIDCODE)
		return false;

	// 이 구간은 스레드별 한번만 수행할수 있게 처리해야 된다.
	st_Top OldTop;
	LONG64 Unique = InterlockedIncrement64(&m_iUnique);

	while(1)
	{
		// 자료구조를 변경한다.
		//Unique = InterlockedIncrement64(&pPop->UniqueCount);
		OldTop._TopNode = pTop->_TopNode;
		OldTop.UniqueCount = pTop->UniqueCount;

		if (OldTop._TopNode != pDel->pNextBlock && OldTop._TopNode != pDel)
			pDel->pNextBlock = OldTop._TopNode;
		
		// 다른 스레드와 충돌했는가? -> 실패시 시도전으로 돌아간다.
		// 충돌의 기준 = OldPop과 pPop의 값 변경이 생겼는가? -> 이외의 값이 변경되지 않게끔 해야된다. => Lock Free 효과를 위해서
		// 위의 연산들은 Wait-Free하지않음.
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