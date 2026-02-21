#ifndef _VFP2CDATASTRUCTURE_H__
#define _VFP2CDATASTRUCTURE_H__

#include <malloc.h>

#if _MSC_VER < 1400
extern "C" unsigned char _BitScanForward(unsigned long* Index, unsigned long Mask);
#pragma intrinsic(_BitScanForward)

__inline int __popcnt(int n)
{
    n = n - ((n >>1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    n = (n + (n >> 4)) & 0x0F0F0F0F;
    return (n * 0x01010101) >> 24;
}
#else
#include <intrin.h>
#endif

template<class T, int nCount>
class CBoundQueue
{
public:
	CBoundQueue() : m_Head(-1), m_Tail(-1)
	{

	}

	void Clear() {
		m_Top = -1;
		m_Bottom = -1;
	}

	bool Enqueue(const T& pItem)
	{
		LONG next = m_Top + 1;
		if (next != nCount)
		{
			m_Items[next] = pItem;
			m_Top = next;
			return true;
		}
		return false;
	}

	bool Dequeue(T& pItem)
	{
		if (!Empty())
		{
			LONG next = m_Bottom + 1;
			// reset if queue is empty after dequeue - so it can be reused
			if (next == m_Top)
			{
				m_Top = -1;
				m_Bottom = -1;
			}
			else
			{
				m_Top = next;
			}
			pItem = m_Items[next];
			return true;
		}
		return false;
	}
	
	inline bool Empty()
	{
		return m_Top == m_Bottom;
	}

	void* operator new(size_t sz)
	{
		return _aligned_malloc(sz, __alignof(T));
	}

	void operator delete(void* p)
	{
		_aligned_free(p);
	}

private:
	LONG m_Top;
	LONG m_Bottom;
	T m_Items[nCount];
};

template<class T, int nBlockCount>
class CUnboundQueue
{
private:
	struct Node {
		CBoundQueue<T, (nBlockCount - sizeof(struct Node*) -
			(sizeof(CBoundQueue<T, 1>) - sizeof(T))) / sizeof(T)> queue;
		
		struct Node* next;

		void* operator new(size_t sz)
		{
			return _aligned_malloc(sz, __alignof(Node));
		}

		void operator delete(void* p)
		{
			_aligned_free(p);
		}
	};

public:
	CUnboundQueue()
	{
		m_Top = 0;
		m_Bottom = 0;
	}

	~CUnboundQueue()
	{
		while (Node* node = m_Bottom)
		{
			m_Bottom = node->next;
			delete node;
		}
	}

	void Enqueue(const T& pItem)
	{
		if (m_Top == 0)
			Initialize();
		if (m_Top->queue.Enqueue(pItem))
			return;

		Node* node = new Node();
		node->next = 0;
		node->queue.Enqueue(pItem);

		m_Top->next = node;
		m_Top = node;
	}

	bool Dequeue(T& pItem)
	{
		if (m_Bottom == 0)
			return false;
		if (m_Bottom->queue.Dequeue(pItem))
			return true;
		Node* node = m_Bottom->next;
		if (node)
		{
			delete m_Bottom;
			m_Bottom = node;
			return node->queue.Dequeue(pItem);
		}
		return false;
	}

	bool Empty()
	{
		return m_Bottom == 0 || m_Bottom->queue.Empty() || m_Bottom->next > 0;
	}

	void* operator new(size_t sz)
	{
		return _aligned_malloc(sz, __alignof(Node));
	}

	void operator delete(void* p)
	{
		_aligned_free(p);
	}

private:

	void Initialize()
	{
		Node* node = new Node();
		if (node == 0)
			throw E_INSUFMEMORY;
		node->next = 0;
		m_Top = m_Bottom = node;
	}

	Node* m_Top;
	Node* m_Bottom;
};

template<typename T, int BlockSize, int QueueCount>
class CUnboundBlockQueue
{
private:
	
	class Node {
		private:
			int m_QueueTop;
			int m_QueueBottom;
			char* m_Queue[QueueCount];

			static const int AdjustedBlockSize = BlockSize - sizeof(Node*) - sizeof(Node*) - sizeof(int) - sizeof(int) - sizeof(int) - (sizeof(char*) * QueueCount);
			char m_Block[AdjustedBlockSize];

			inline char* AlignNextPosition(unsigned int nSize, unsigned int nAlign)
			{
				unsigned int nNeededAlign = nAlign > sizeof(char*) ? nAlign : sizeof(char*);
				char* pos;
				if (m_QueueTop + 1 < QueueCount)
				{
					pos = (m_QueueTop >= 0 ? m_Queue[m_QueueTop] : m_Block) + nSize;
					pos += (reinterpret_cast<INT_PTR>(pos) % nNeededAlign);
					if (pos > &m_Block[AdjustedBlockSize])
					{
						if (m_QueueTop >= 0)
							pos = 0;
						else
							throw E_INSUFMEMORY;
					}
				}
				else
					pos = 0;

				return pos;
			}

		public:
			Node()
			{
				m_QueueTop = -1;
				m_QueueBottom = -1;
			}
			
			bool Enqueue(T& pItem)
			{
				char* pos = AlignNextPosition(pItem.BlockSize(), __alignof(T));
				if (pos)
				{
					m_QueueTop++;
					m_Queue[m_QueueTop] = pos;
					pItem.ToBlock(m_QueueTop > 0 ? m_Queue[m_QueueTop - 1] : m_Block);
					return true;
				}
				return false;
			}
			
			bool Dequeue(T& pItem)
			{
				if (m_QueueBottom < m_QueueTop)
				{
					m_QueueBottom++;
					char* pos = m_QueueBottom > 0 ? m_Queue[m_QueueBottom - 1] : m_Block;
					pItem.FromBlock(pos);
					return true;
				}
				return false;
			}

			bool Discard()
			{
				if (m_QueueBottom < m_QueueTop)
				{
					m_QueueBottom++;
					return true;
				}
				return false;
			}

			void Reset()
			{
				m_QueueTop = -1;
				m_QueueBottom = -1;
			}

			T* Peek()
			{
				if (m_QueueBottom >= 0)
				{
					T* item = m_QueueBottom > 0 ? (T*)m_Queue[m_QueueBottom - 1] : (T*)m_Block;
					return item;
				}
				return 0;
			}

			T* PeekLast()
			{
				if (m_QueueTop >= 0)
				{
					T* item = m_QueueTop > 0 ? (T*)m_Queue[m_QueueTop - 1] : (T*)m_Block;
					return item;
				}
				return 0;
			}

			Node* next;
			Node* prev;
	};

public:
	CUnboundBlockQueue()
	{
		m_Top = 0;
		m_Bottom = 0;
	}

	~CUnboundBlockQueue()
	{
		while (Node* node = m_Bottom)
		{
			m_Bottom = node->next;
			delete node;
		}
	}

	void Enqueue(T& pItem)
	{
		if (m_Top == 0)
			Initialize();

		if (m_Top->Enqueue(pItem))
			return;

		Node* node = new Node();
		if (node == 0)
			throw E_INSUFMEMORY;
		
		node->prev = m_Top;
		node->next = 0;

		m_Top->next = node;
		m_Top = node;
		m_Top->Enqueue(pItem);
	}

	bool Dequeue(T& pItem)
	{
		if (m_Bottom == 0)
			return false;
		if (m_Bottom->Dequeue(pItem))
			return true;

		Node* node = m_Bottom->next;
		if (node)
		{
			delete m_Bottom;
			m_Bottom = node;
			return node->Dequeue(pItem);
		}
		else
			m_Bottom->Reset();

		return false;
	}

	void Discard()
	{
		if (m_Bottom == 0 || m_Bottom->Discard())
			return;

		Node* node = m_Bottom->next;
		if (node)
		{
			delete m_Bottom;
			m_Bottom = node;
			node->Discard();
		}
		return;
	}

	T* Peek()
	{
		if (m_Bottom == 0)
			return 0;
		return m_Bottom->Peek();
	}

	T* PeekLast()
	{
		if (m_Top == 0)
			return 0;
		return m_Top->PeekLast();
	}

private:

	void Initialize()
	{
		Node* node = new Node();
		if (node == 0)
			throw E_INSUFMEMORY;
		node->next = 0;
		node->prev = 0;
		m_Top = node;
		m_Bottom = node;
	}

	Node* m_Top;
	Node* m_Bottom;
};

template<typename T, int BlockSize, int StackCount>
class CUnboundBlockStack
{
private:

	class Node {
	private:
		int m_WalkIndex;
		int m_StackIndex;
		char* m_Stack[StackCount];

		static const int AdjustedBlockSize = BlockSize - sizeof(Node*) - sizeof(Node*) - sizeof(int) - sizeof(int) - (sizeof(char*) * StackCount);
		char m_Block[AdjustedBlockSize];

		inline char* AlignNextPosition(unsigned int nSize, unsigned int nAlign)
		{
			unsigned int nNeededAlign = nAlign > sizeof(char*) ? nAlign : sizeof(char*);
			char* pos;
			if (m_StackIndex + 1 < StackCount)
			{
				pos = (m_StackIndex >= 0 ? m_Stack[m_StackIndex] : m_Block) + nSize;
				pos += (reinterpret_cast<INT_PTR>(pos) % nNeededAlign);
				if (pos > &m_Block[AdjustedBlockSize])
				{
					if (m_StackIndex >= 0)
						pos = 0;
					else
						throw E_OUTOFMEMORY;
				}
			}
			else
				pos = 0;

			return pos;
		}

	public:
		Node* prev;
		Node* next;

		Node()
		{
			m_StackIndex = -1;
		}

		bool Enqueue(T& pItem)
		{
			char* pos = AlignNextPosition(pItem.BlockSize(), __alignof(T));
			if (pos)
			{
				m_StackIndex++;
				m_Stack[m_StackIndex] = pos;
				pItem.ToBlock(m_StackIndex > 0 ? m_Stack[m_StackIndex - 1] : m_Block);
				return true;
			}
			return false;
		}

		bool Dequeue(T& pItem)
		{
			if (m_StackIndex >= 0)
			{
				m_StackIndex--;
				char* pos = m_StackIndex >= 0 ? m_Stack[m_StackIndex] : m_Block;
				pItem.FromBlock(pos);
				return true;
			}
			return false;
		}

		bool Dequeue(T** pItem)
		{
			if (m_StackIndex >= 0)
			{
				m_StackIndex--;
				T* item = m_StackIndex > 0 ? (T*)m_Stack[m_StackIndex] : (T*)m_Block;
				*pItem = item;
				return true;
			}
			return false;
		}


		T* WalkFirst()
		{
			m_WalkIndex = m_StackIndex;
			if (m_WalkIndex >= 0)
			{
				T* item = m_WalkIndex > 0 ? (T*)m_Stack[m_WalkIndex - 1] : (T*)m_Block;
				return item;
			}
			return 0;
		}

		T* WalkLast()
		{
			m_WalkIndex = 0;
			if (m_StackIndex >= 0)
			{
				T* item = (T*)m_Block;
				return item;
			}
			return 0;
		}

		T* WalkPrev()
		{
			if (m_WalkIndex >= 0)
			{
				m_WalkIndex--;
				T* item = m_WalkIndex > 0 ? (T*)m_Stack[m_WalkIndex - 1] : (T*)m_Block;
				return item;
			}
			return 0;
		}

		T* WalkNext()
		{
			if (m_WalkIndex < m_StackIndex)
			{
				m_WalkIndex++;
				T* item = m_WalkIndex > 0 ? (T*)m_Stack[m_WalkIndex - 1] : (T*)m_Block;
				return item;
			}
			return 0;
		}
	};

public:
	CUnboundBlockStack()
	{
		m_Top = 0;
		m_WalkTop = 0;
	}

	~CUnboundBlockStack()
	{
		while (Node* node = m_Top)
		{
			m_Top = node->prev;
			delete node;
		}
	}

	void Enqueue(T& pItem)
	{
		if (m_Top == 0)
			Initialize();

		if (m_Top->Enqueue(pItem))
			return;

		Node* node = new Node();
		if (node == 0)
			throw E_INSUFMEMORY;

		node->next = 0;
		m_Top->next = node;
		node->prev = m_Top;
		m_Top = node;
		m_Top->Enqueue(pItem);
	}

	bool Dequeue(T& pItem)
	{
		if (m_Top == 0)
			return false;
		if (m_Top->Dequeue(pItem))
			return true;

		Node* node = m_Top->prev;
		if (node)
		{
			delete m_Top;
			m_Top = node;
			return node->Dequeue(pItem);
		}
		return false;
	}

	bool Dequeue(T** pItem)
	{
		if (m_Top == 0)
			return false;
		if (m_Top->Dequeue(pItem))
			return true;

		Node* node = m_Top->prev;
		if (node)
		{
			delete m_Top;
			m_Top = node;
			return node->Dequeue(pItem);
		}
		return false;
	}

	T* WalkFirst()
	{
		if (m_Top == 0)
			return 0;
		m_WalkTop = m_Top;
		return m_WalkTop->WalkFirst();
	}

	T* WalkPrev()
	{
		T* prev = m_WalkTop->WalkPrev();
		if (prev)
			return prev;
		
		if (m_WalkTop->prev)
		{
			m_WalkTop = m_WalkTop->prev;
			return m_WalkTop->WalkPrev();
		}
		return 0;
	}

	T* WalkNext()
	{
		T* next = m_WalkTop->WalkNext();
		if (next)
			return next;

		if (m_WalkTop->next)
		{
			m_WalkTop = m_WalkTop->next;
			return m_WalkTop->WalkLast();
		}
		return 0;
	}


private:
	void Initialize()
	{
		Node* node = new Node();
		if (node == 0)
			throw E_INSUFMEMORY;
		node->next = 0;
		node->prev = 0;
		m_Top = node;
	}
	
	Node* m_WalkTop;
	Node* m_Top;
};

// very simple array class, use only with blittable (struct's with custom constructor/destructor) types
template<typename T>
class CArray
{
public:
	CArray() : m_Index(-1), m_MaxElements(0), m_AutoGrow(0), m_Array(0) {}
	~CArray()
	{
		if (m_Array)
			free(m_Array);
	}

	void AutoGrow(int nAutoGrow)
	{
		m_AutoGrow = nAutoGrow;
	}

	int Reserve(unsigned int nMaxElements, bool bZeroMemory = false, bool bSetMaxIndex = false)
	{
		if (nMaxElements <= m_MaxElements)
			return m_MaxElements;

		T* newArray = static_cast<T*>(calloc(nMaxElements, sizeof(T)));
		if (!newArray)
			throw E_INSUFMEMORY;
		if (m_Array != 0)
		{
			memcpy(newArray, m_Array, sizeof(T) * m_MaxElements);
			if (bZeroMemory)
				memset(newArray + m_MaxElements, 0, sizeof(T) * (nMaxElements - m_MaxElements));
			free(m_Array);
		}
		else if (bZeroMemory)
		{
			memset(newArray, 0, sizeof(T) * nMaxElements);
		}
		m_Array = newArray;
		m_MaxElements = nMaxElements;
		if (bSetMaxIndex)
			m_Index = nMaxElements - 1;
		return m_MaxElements;
	}

	unsigned int Truncate(int nElements)
	{
		if (nElements >= m_MaxElements)
			return m_MaxElements;

		T* newArray = static_cast<T*>(calloc(nElements, sizeof(T)));
		if (!newArray)
			throw E_INSUFMEMORY;
		if (m_Array != 0)
		{
			memcpy(newArray, m_Array, sizeof(T) * nElements);
			free(m_Array);
		}
		m_Array = newArray;
		m_MaxElements = nElements;
		return m_MaxElements;
	}

	bool Empty() const
	{
		return m_Index == -1;
	}

	unsigned int GetCount() const
	{
		return m_Index + 1;
	}

	int GetIndex() const
	{
		return m_Index;
	}

	void SetIndex(int nIndex)
	{
		assert(nIndex == -1 || (nIndex >= 0 && (unsigned int)nIndex < m_MaxElements));
		m_Index = nIndex;
	}

	int Add(const T& element)
	{	
		if ((unsigned int)(m_Index + 1) >= m_MaxElements)
		{
			int nMaxElements = m_MaxElements + 
				(m_AutoGrow == 0 && m_MaxElements > 0 ? m_MaxElements : m_AutoGrow > 0 ? m_AutoGrow : 16);
			m_MaxElements = Reserve(nMaxElements);
		}
		m_Array[++m_Index] = element;
		return m_Index;
	}

	void Remove(int nIndex)
	{
		assert(nIndex >= 0 && nIndex <= m_Index);
		if (m_Index == nIndex)
		{
			m_Index--;
			return;
		}
		void* pDest = (void*)&m_Array[nIndex];
		void* pSource = (void*)&m_Array[nIndex + 1];
		size_t nSize = (m_Index - (nIndex + 1)) * sizeof(T);
		memmove(pDest, pSource, nSize);
		m_Index--;
	}

	void Remove(const T& element)
	{
		int nIndex = Find(element);
		if (nIndex != -1)
			Remove(nIndex);
	}

	T& operator[](int nIndex) const
	{
		assert(nIndex >= 0 && (unsigned int)nIndex < m_MaxElements);
		return m_Array[nIndex];
	}

	int Find(const T& element)
	{
		int nCount = GetCount();
		for (int xj = 0; xj < nCount; xj++)
		{
			if (m_Array[xj] == element)
				return xj;
		}
		return -1;
	}

private:
	int m_Index;
	unsigned int m_MaxElements;
	unsigned int m_AutoGrow;
	T* m_Array;
};

// very simple fixed array class, use only with blittable (struct's with custom constructor/destructor) types
template<typename T, int nSize>
class CFixedArray
{
public:
	CFixedArray() {}

	T& operator[](int nIndex)
	{
		assert(nIndex >= 0 && nIndex < nSize);
		return m_Array[nIndex];
	}

	int Find(const T& element)
	{
		for (int xj = 0; xj < nSize; xj++)
		{
			if (m_Array[xj] == element)
				return xj;
		}
		return -1;
	}

	void Copy(const T* pArray, int nCount)
	{
		memcpy(&m_Array[0], pArray, nCount * sizeof(T));
	}

	int GetCount()
	{
		return nSize;
	}

private:
	T m_Array[nSize];
};

template<typename T>
class CPtrArray
{
public:
	CPtrArray() : m_Array(0), m_Count(0) {}
	~CPtrArray()
	{
		if (m_Array)
			delete[] m_Array;
	}

	void Free()
	{
		if (m_Array)
		{
			delete[] m_Array;
			m_Array = 0;
		}
	}

	void Allocate(unsigned int nCount)
	{
		assert(m_Array == 0);
		m_Array = new T[nCount];
		if (m_Array == 0)
			throw E_INSUFMEMORY;
		m_Count = nCount;
	}

	T* operator[](int nIndex)
	{
		assert(nIndex >= 0 && (unsigned int)nIndex < m_Count);
		return &m_Array[nIndex];
	}

	unsigned int GetCount() const
	{
		return m_Count;
	}

private:
	T* m_Array;
	unsigned int m_Count;
};

template<typename T>
class CDynamicPtrArray
{
public:
	CDynamicPtrArray() : m_Array(0), m_Size(0), m_Elements(0), m_GrowBy(4)  {}
	~CDynamicPtrArray()
	{
		for (unsigned int xj = 0; xj < m_Elements; xj++)
			delete m_Array[xj];
		delete[] m_Array;
	}

	void Free()
	{
		assert(m_Array == 0);
		for (unsigned int xj = 0; xj < m_Elements; xj++)
			delete m_Array[xj];
		delete[] m_Array;
		m_Array = 0;
		m_Size = 0;
		m_Elements = 0;
	}

	void SetCount(unsigned int nCount)
	{
		assert(m_Array == 0);
		int xj;
		m_Array = new T*[nCount];
		if (m_Array == 0)
			throw E_INSUFMEMORY;
		m_Size = nCount;
		for (xj = 0; xj < m_Size; xj++)
			m_Array[xj] = 0;
		for (xj = 0; xj < m_Size; xj++)
		{
			T *newobj = new T();
			if (newobj == 0)
				throw E_INSUFMEMORY;
			m_Array[xj] = newobj;
			m_Elements++;
		}
	}

	T* Add()
	{
		unsigned int xj;
		unsigned int nextIndex = m_Elements;
		if (nextIndex >= m_Size)
		{
			T** newarray = new T*[m_Size + m_GrowBy];
			if (newarray == 0)
				throw E_INSUFMEMORY;
			for (xj = 0; xj < m_Elements; xj++)
			{
				newarray[xj] = m_Array[xj];
				m_Array[xj] = 0;
			}
			for (xj = m_Elements; xj < m_Size + m_GrowBy; xj++)
			{
				newarray[xj] = 0;
			}
			delete[] m_Array;
			m_Array = newarray;
			m_Size = m_Size + m_GrowBy;
		}
		T* newobj = new T();
		if (newobj == 0)
			throw E_INSUFMEMORY;
		m_Array[nextIndex] = newobj;
		m_Elements = nextIndex + 1;
		return newobj;
	}

	T* operator[](int nIndex)
	{
		assert(nIndex >= 0 && (unsigned int)nIndex < m_Size);
		return m_Array[nIndex];
	}

	bool IsEmpty()
	{
		return m_Elements == 0;
	}

	unsigned int GetCount()
	{
		return m_Elements;
	}

	unsigned int GetSize()
	{
		return m_Size;
	}

	void AutoGrow(unsigned int nGrowBy)
	{
		m_GrowBy = nGrowBy;
	}

private:
	T** m_Array;
	unsigned int m_Elements;
	unsigned int m_Size;
	unsigned int m_GrowBy;
};

template<class T, unsigned int nSize>
class CFastItemPool
{
public:
	CFastItemPool()
	{
		assert(nSize % 64 == 0);
#ifdef _WIN64
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
			m_Slots[xj].BitSet64 = ~0ui64;
#else
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			m_Slots[xj].BitSet32_1 = ~0u;
			m_Slots[xj].BitSet32_2 = ~0u;
		}
#endif
	}

	unsigned int Size()
	{
		return nSize;
	}

	unsigned int FreeCount()
	{
#ifdef _WIN64
		unsigned __int64 nCount = 0;
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
			nCount += __popcnt64(m_Slots[xj].BitSet64);
		return (unsigned int)(nCount);
#else
		unsigned int nCount = 0;
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			nCount += __popcnt(m_Slots[xj].BitSet32_1);
			nCount += __popcnt(m_Slots[xj].BitSet32_2);
		}
		return (unsigned int)(nCount);
#endif
	}

	T* GetItem()
	{
		DWORD nIndex, nOffset = 0;
#ifdef _WIN64
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			if (_BitScanForward64(&nIndex, m_Slots[xj].BitSet64))
			{
				m_Slots[xj].BitSet64 &= ~(1ui64 << nIndex);
				return &m_Elements[nIndex + nOffset];
			}
			nOffset += 64;
		}
		return 0;
#else
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			if (_BitScanForward(&nIndex, m_Slots[xj].BitSet32_1))
			{
				m_Slots[xj].BitSet32_1 &= ~(1 << nIndex);
				return &m_Elements[nIndex + nOffset];
			}
			nOffset += 32;
			if (_BitScanForward(&nIndex, m_Slots[xj].BitSet32_2))
			{
				m_Slots[xj].BitSet32_2 &= ~(1 << nIndex);
				return &m_Elements[nIndex + nOffset];
			}
			nOffset += 32;
		}
		return 0;
#endif
	}

	typedef bool(_stdcall comparefunc)(T* item, DWORD compare);
	T* FindUsedItem(comparefunc func, DWORD compare)
	{
		DWORD nIndex, nOffset = 0;
#ifdef _WIN64
		unsigned __int64 slots;
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			slots = ~m_Slots[xj].BitSet64;
			for (unsigned int xt = 0; xt < 64; xt++)
			{
				if (_BitScanForward64(&nIndex, slots))
				{
					if (func(&m_Elements[nIndex + nOffset], compare))
						return &m_Elements[nIndex + nOffset];
					else
						slots &= ~(1ui64 << nIndex);
				}
				else
					break;
			}
			nOffset += 64;
		}
		return 0;
#else
		unsigned int slots;
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			slots = ~m_Slots[xj].BitSet32_1;
			for (unsigned int xt = 0; xt < 32; xt++)
			{
				if (_BitScanForward(&nIndex, slots))
				{
					if (func(&m_Elements[nIndex + nOffset], compare))
						return &m_Elements[nIndex + nOffset];
					else
						slots &= ~(1 << nIndex);
				}
				else
					break;
			}
			nOffset += 32;
			slots = ~m_Slots[xj].BitSet32_2;
			for (unsigned int xt = 0; xt < 32; xt++)
			{
				if (_BitScanForward(&nIndex, slots))
				{
					if (func(&m_Elements[nIndex + nOffset], compare))
						return &m_Elements[nIndex + nOffset];
					else
						slots &= ~(1 << nIndex);
				}
				else
					break;
			}
			nOffset += 32;
		}
		return 0;
#endif
	}

	void ReleaseItem(T* element)
	{
		unsigned int nOffset;
#ifdef _WIN64
		__int64 nIndex = (element - &m_Elements[0]), nAdjustedIndex;
		bool expr = countof(m_Slots) == 1;
		if (expr)
		{
			m_Slots[0].BitSet64 |= (1ui64 << nIndex);
		}
		else
		{
			nOffset = (unsigned int)(nIndex / 64);
			nAdjustedIndex = nIndex - nOffset * 64;
			m_Slots[nOffset].BitSet64 |= (1ui64 << nAdjustedIndex);
		}
#else
		unsigned int nIndex = (element - &m_Elements[0]);
		bool expr = countof(m_Slots) == 1;
		if (expr)
		{
			if (nIndex < 32)
				m_Slots[0].BitSet32_1 |= (1 << nIndex);
			else
				m_Slots[0].BitSet32_2 |= (1 << (nIndex - 32));
		}
		else
		{
			nOffset = (unsigned int)(nIndex / 64);
			nIndex -= nOffset * 64;
			if (nIndex < 32)
				m_Slots[nOffset].BitSet32_1 |= (1 << nIndex);
			else
				m_Slots[nOffset].BitSet32_2 |= (1 << (nIndex - 32));
		}
#endif
	}

	T* operator[](unsigned int nIndex)
	{
		assert(nIndex < nSize);
		return &m_Elements[nIndex];
	}

private:
	typedef union Slots
	{
		struct {
			unsigned int BitSet32_1;
			unsigned int BitSet32_2;
		};
		unsigned __int64 BitSet64;
	};
	Slots m_Slots[nSize / 64];
	T m_Elements[nSize];
};

template<class T, class TCompare, unsigned int nSize>
class CFastQueue
{
public:
	CFastQueue()
	{
#ifdef _WIN64
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
			m_Slots[xj].BitSet64 = ~0ui64;
#else
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			m_Slots[xj].BitSet32_1 = ~0u;
			m_Slots[xj].BitSet32_2 = ~0u;
		}
#endif
	}

	unsigned int FreeCount()
	{
#ifdef _WIN64
		unsigned __int64 nCount = 0;
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
			nCount += __popcnt64(m_Slots[xj].BitSet64);
		return (unsigned int)(nCount);
#else
		unsigned int nCount = 0;
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			nCount += __popcnt(m_Slots[xj].BitSet32_1);
			nCount += __popcnt(m_Slots[xj].BitSet32_2);
		}
		return (unsigned int)(nCount);
#endif
	}

	bool Enqueue(T element)
	{
		DWORD nIndex, nOffset = 0;;
#ifdef _WIN64
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			if (_BitScanForward64(&nIndex, m_Slots[xj].BitSet64))
			{
				m_Slots[xj].BitSet64 &= ~(1ui64 << nIndex);
				m_Elements[nIndex + nOffset] = element;
				return true;
			}
			nOffset += 64;
		}
		return false;
#else
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			if (_BitScanForward(&nIndex, m_Slots[xj].BitSet32_1))
			{
				m_Slots[xj].BitSet32_1 &= ~(1 << nIndex);
				m_Elements[nIndex + nOffset] = element;
				return true;
			}
			nOffset += 32;
			if (_BitScanForward(&nIndex, m_Slots[xj].BitSet32_2))
			{
				m_Slots[xj].BitSet32_2 &= ~(1 << nIndex);
				m_Elements[nIndex + nOffset] = element;
				return true;
			}
			nOffset += 32;
		}
		return false;
#endif
	}

	typedef bool(_stdcall* comparefunc)(T elm, TCompare compare);
	T Dequeue(comparefunc func, TCompare compare)
	{
		DWORD nIndex, nOffset = 0;
		T element;
#ifdef _WIN64
		unsigned __int64 slots;
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			slots = ~m_Slots[xj].BitSet64;
			for (unsigned int xt = 0; xt < 64; xt++)
			{
				if (_BitScanForward64(&nIndex, slots))
				{
					if (func(m_Elements[nIndex + nOffset], compare))
					{
						element = m_Elements[nIndex + nOffset];
						m_Slots[xj].BitSet64 |= (1ui64 << nIndex);
						return element;
					}
					slots &= ~(1ui64 << nIndex);
				}
				else
					break;
			}
			nOffset += 64;
		}
		return 0;
#else
		unsigned int slots;
		for (unsigned int xj = 0; xj < countof(m_Slots); xj++)
		{
			slots = ~m_Slots[xj].BitSet32_1;
			for (unsigned int xt = 0; xt < 32; xt++)
			{
				if (_BitScanForward(&nIndex, slots))
				{
					if (func(m_Elements[nIndex + nOffset], compare))
					{
						element = m_Elements[nIndex + nOffset];
						m_Slots[xj].BitSet32_1 |= (1 << nIndex);
						return element;
					}
					slots &= ~(1 << nIndex);
				}
				else
					break;
			}
			nOffset += 32;
			slots = ~m_Slots[xj].BitSet32_2;
			for (unsigned int xt = 0; xt < 32; xt++)
			{
				if (_BitScanForward(&nIndex, slots))
				{
					if (func(m_Elements[nIndex + nOffset], compare))
					{
						element = m_Elements[nIndex + nOffset];
						m_Slots[xj].BitSet32_2 |= (1 << nIndex);
						return element;
					}
					slots &= ~(1 << nIndex);
				}
				else
					break;
			}
			nOffset += 32;
		}
		return 0;
#endif
	}

private:
	typedef union
	{
		struct {
			unsigned int BitSet32_1;
			unsigned int BitSet32_2;
		};
		unsigned __int64 BitSet64;
	} Slots;
	Slots m_Slots[nSize / 64];
	T m_Elements[nSize];
};

template<class T>
class CBlockPtrArray
{
public:
	CBlockPtrArray() : m_Elements(0), m_Size(0), m_Count(0) {}
	~CBlockPtrArray()
	{
		if (m_Elements)
			free(m_Elements);
	}

	void Add(T* item)
	{
		if (m_Count == m_Size)
		{
			m_Size = (m_Size ? m_Size * 2 : 64);
			m_Elements = (T**)_recalloc(m_Elements, m_Size, sizeof(T*));
			if (m_Elements == 0)
				throw E_INSUFMEMORY;
		}
		m_Elements[m_Count] = item;
		m_Count++;
	}

	unsigned int GetCount() { return m_Count; }
	void Reset() { m_Count = 0; }
	T* operator[](unsigned int nIndex) { return m_Elements[nIndex]; }

private:
	T** m_Elements;
	unsigned int m_Size;
	unsigned int m_Count;
};

template<typename T, int nPages>
class CUnboundBlockArray
{
private:
	enum _ { BlockSize = (nPages * 4096) - (nPages * (sizeof(void*) + sizeof(char*) * 2)) };

	class Node {
	public:
		Node()
		{
			next = 0;
			m_Top = m_Block;
			memset(m_Block, 0, sizeof(m_Block));
			m_End = &m_Block[BlockSize - 1 - sizeof(char*)];
		}

		void Reset()
		{
			m_Top = m_Block;
			memset(m_Block, 0, sizeof(m_Block)); // TODO, not necessary
		}

		T* Add(T& pItem)
		{
			char* nextpos = AlignNextPosition(pItem.BlockSize(), __alignof(T));
			if (nextpos)
			{
				pItem.ToBlock(m_Top);
				*reinterpret_cast<char**>(nextpos) = m_Top;
				T* ptr = reinterpret_cast<T*>(m_Top);
				m_Top = nextpos + sizeof(char*);
				return ptr;
			}
			return 0;
		}
		Node* next;

	private:

		inline char* AlignNextPosition(unsigned int nSize, unsigned int nAlign)
		{
			unsigned int nNeededAlign = nAlign > sizeof(char*) ? nAlign : sizeof(char*);
			char* next = m_Top + nSize;
			next += (reinterpret_cast<INT_PTR>(next) % nNeededAlign);
			if (next < m_End)
				return next;
			else
				return 0;
		}

		char* m_Top;
		char* m_End;
		char m_Block[BlockSize];
	};

public:
	CUnboundBlockArray()
	{
		m_Top = 0;
		m_First = 0;
	}

	~CUnboundBlockArray()
	{
		m_Top = m_First;
		while (Node* node = m_Top)
		{
			m_Top = node->next;
			delete node;
		}
	}

	void Add(T& pItem)
	{
		if (m_Top == 0)
			Initialize();

		T* item = m_Top->Add(pItem);
		if (item)
		{
			m_Elements.Add(item);
			return;
		}

		Node* node = m_Top->next;
		if (node == 0)
		{
			node = new Node();
			if (node == 0)
				throw E_INSUFMEMORY;
			m_Top->next = node;
			m_Top = node;
		}
		else
			m_Top = node;

		item = m_Top->Add(pItem);
		m_Elements.Add(item);
	}

	unsigned int GetCount() { return m_Elements.GetCount(); }

	void Reset()
	{
		m_Top = m_First;
		m_Elements.Reset();
		while (Node* node = m_Top)
		{
			node->Reset();
			m_Top = node->next;
		}
		m_Top = m_First;
	}

	T* operator[](unsigned int nIndex) { return m_Elements[nIndex]; }

private:
	void Initialize()
	{
		Node* node = new Node();
		if (node == 0)
			throw E_INSUFMEMORY;
		m_Top = node;
		m_First = node;
	}

	CBlockPtrArray<T> m_Elements;
	Node* m_Top;
	Node* m_First;
};

template<class T, int nSize, int nMaxSize>
class StaggeredArray
{
public:
	StaggeredArray() 
	{ 
		memset(m_Array, 0, sizeof(m_Array));
	}

	~StaggeredArray()
	{
		for (int xj = 0; xj < countof(m_Array); xj++)
		{
			if (m_Array[xj])
				delete[] m_Array[xj];
		}
	}

	T& operator[](unsigned int nIndex)
	{
		assert(nIndex < nMaxSize);
		unsigned int nIdx = nIndex / nSize;
		unsigned int nIdx2 = nIndex % nSize;
		if (m_Array[nIdx])
			return m_Array[nIdx][nIdx2];

		m_Array[nIdx] = new T[nSize]();
		if (m_Array[nIdx] == 0)
			throw E_INSUFMEMORY;
		return m_Array[nIdx][nIdx2];
	}

	unsigned int Max()
	{
		return nMaxSize - 1;
	}

	private:
		T* m_Array[nMaxSize / nSize];
};

#endif // _VFP2CDATASTRUCTURE_H__