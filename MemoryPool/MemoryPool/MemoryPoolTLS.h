#pragma once

#include <Windows.h>
#include "MemoryPool.h"


// dynamic TLS Alloc �� ����Ͽ� �����庰�� ChunkBlock �� �Ҵ��ϸ�
// �̸����ؼ� �޸𸮸� �Ҵ� ����.
//
// - CMemoryPool : ���� �޸� ������ (CChunkBlock �� ������)
// - CChunkBlock : ���� ����ڿ� ��ü�� �����ϴ� �����庰 �Ҵ� ���
//
//
// 1. MemoryPool �ν��Ͻ��� �����Ǹ� CChunkBlock �� ������ Ȯ���Ͽ� ���� �غ� ��.
// 2. MemoryPool �����ڿ��� TlsAlloc �� ���ؼ� �޸�Ǯ ���� TLS �ε����� Ȯ��.
// 3. �� TLS �ε��� ������ �� ������� CChunkBlock �� �������.
//    ���� �� ���μ������� �������� MemoryPool �� ����ϰ� �ȴٸ� ������ TLS Index �� ������ ��.
// 4. �����忡�� MemoryPool.Alloc ȣ��
// 5. TLS Index �� CChunkBlock �� �ִ��� Ȯ��.
// 6. ûũ����� ���ٸ� (NULL �̶��)  MemoryPool ���� ChunkBlock �� �Ҵ��Ͽ� TLS �� ���.
// 7. TLS �� ��ϵ� ChunkBlock ���� DATA Alloc �� ��ȯ.
// 8. ChunkBlock �� �� �̻� DATA �� ���ٸ� TLS �� NULL �� �ٲپ� ���� ����.
//
// 1. MemoryPool.Free ȣ���
// 2. �Է� �����͸� �������� ChunkBlock �� ����.
// 3. ChunkBlock Ref �� ����.
// 4. ChunkBlock Ref �� 0 �̶�� MemoryPool �� ChunkBlock ��ȯ.
// -. Free �� ���� �������� ������ �����Ƿ� ������ ������ �ϵ��� ����.
//
// Alloc �Ҵ��� TLS �� �Ҵ�� CChunkBlock ���� �����帶�� ���������� ����
// Free ������ CChunkBlock ���۷��� ī���͸� ���� �� �ش� ûũ��Ͽ� ������� ������ 0 �ΰ�� CMemoryPool �� ��ȯ.
//
// ChunkBlock �� DATA �� �Ҵ縸 �����ϸ�, ��ȯ �� ������ ������� ����.
// ������ ��ȯ�� ��� � �����忡���� �����ؾ� �ϱ� ������ ����ȭ ������ ��ȯ�� ����.
//
// ChunkBlock �� 10 ���� DATA �� �ִٸ� �� 10���� DATA �� ��� �Ҵ� �� ��ȯ �ϰ� �� �� 
// ChunkBlock ��ü�� ���� �޸�Ǯ�� ��ȯ�Ǿ� ûũ ��ü�� �� ����� ��ٸ��� �ȴ�.
//////////////////////////////////////////////////////////////////





template <class DATA>
class CMemoryPoolTLS
{
private:
	class CChunkBlock
	{
	private:
#pragma pack(push, 1)
		struct st_ChunkDATA
		{
			CChunkBlock *pThisChunk;		// 64��Ʈ
			DATA	Data;
			CMemoryPool<CChunkBlock>* pMemoryPool;
		};
#pragma pack(pop)
	public:
		CChunkBlock(CMemoryPool<CChunkBlock> *pMemoryPool ,int BlockSize = 1000)
		{

			if(m_lInit != 0x777778888899999A)
				pArrayChunk = (st_ChunkDATA *)malloc(sizeof(st_ChunkDATA) * BlockSize);
		
			for (int i = 0; i < BlockSize; i++)
			{
				pArrayChunk[i].pThisChunk = this;
				pArrayChunk[i].pMemoryPool = pMemoryPool;
			}

			m_lAllocCount = 0;
			m_lReferenceCount = BlockSize;
		}

		DATA* Alloc()
		{
			DATA *ret = &pArrayChunk[m_lAllocCount].Data;

		//	if (Constructor)
		//		new (ret) DATA();

			m_lAllocCount++;

			return ret;
		}
		bool Free(DATA *pData, st_ChunkDATA *pBlock)
		{
			if (InterlockedDecrement64(&m_lReferenceCount) == 0)
			{
				pBlock->pMemoryPool->Free(pBlock->pThisChunk);
				//free(pArrayChunk);
				m_lInit = 0x777778888899999A;
			}

	

			return true;
		}

	private:
		st_ChunkDATA *pArrayChunk;

		LONG64	m_lReferenceCount;
		LONG64	m_lAllocCount;
		LONG64	m_lInit; // ����

		template <class DATA>
		friend class CMemoryPoolTLS;
	};

public:
	CMemoryPoolTLS(int BlockSize = 0, int ChunkSize = 2000, bool mConstructor = false)
		:BlockSize(BlockSize), b_Constructor(mConstructor), ChunkSize(ChunkSize)
	{
		AllocCount = 0;
	
		pMemoryPool = new CMemoryPool<CChunkBlock>(BlockSize, false);

		DWORD TLSIndex = TlsAlloc();
		if (TLSIndex == TLS_OUT_OF_INDEXES)
		{
			int *p = nullptr;
			*p = 0;
		}
	}

	~CMemoryPoolTLS()
	{
		TlsFree(TLSIndex);
	}

	DATA* Alloc()
	{
		CChunkBlock *pBlock = (CChunkBlock *)TlsGetValue(TLSIndex);

		if (pBlock == nullptr)
		{
			pBlock = pMemoryPool->Alloc();
			new (pBlock) CChunkBlock(pMemoryPool, ChunkSize);
			TlsSetValue(TLSIndex, pBlock);
		}

		DATA *pRet = pBlock->Alloc();

		if (b_Constructor)
			new (ret) DATA();

		InterlockedAdd(&AllocCount, 1);

		if (pBlock->m_lAllocCount >= ChunkSize)
			TlsSetValue(TLSIndex, nullptr);

		return pRet;
	}
	bool Free(DATA *pData)
	{
		if (b_Constructor)
			pData->~DATA();

		CChunkBlock::st_ChunkDATA *pBlock = (CChunkBlock::st_ChunkDATA *)((__int64 *)pData - 1);

		pBlock->pThisChunk->Free(pData, pBlock);
		InterlockedAdd(&AllocCount, -1);
		return true;
	}

	long GetAllocCount()
	{
		return AllocCount;
	}
	void Mornitoring()
	{
		wprintf(L"ChunkPool BlockCount : %d  Alloc:%I64d \n", pMemoryPool->GetBlockCount(), pMemoryPool->GetAllocCount());
	}
private:
	CMemoryPool<CChunkBlock>* pMemoryPool;

	DWORD TLSIndex;
	long	AllocCount;
	int		BlockSize;
	int		ChunkSize;

	bool	b_Constructor;
};