#ifndef _VFP2CDATASTRUCTURE_H__
#define _VFP2CDATASTRUCTURE_H__

#include <malloc.h>

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

template<typename T, int nBlockSize>
class CUnboundBlockQueue
{
private:
	
	class Node {
		public:
			Node()
			{
				m_Top = m_Bottom = m_Block;
				memset(m_Block, 0, sizeof(m_Block));
				m_End = &m_Block[nBlockSize - 1 - sizeof(char*)];
			}
			
			bool Enqueue(T& pItem)
			{
				char* nextpos = AlignNextPosition(pItem.BlockSize(), __alignof(T));
				if (nextpos)
				{
					*reinterpret_cast<char**>(m_Top) = nextpos;
					pItem.ToBlock(m_Top + sizeof(char*));
					m_Top = nextpos;
					*reinterpret_cast<char**>(m_Top) = 0;
					return true;
				}
				return false;
			}
			
			bool Dequeue(T& pItem)
			{
				char* nextpos = *reinterpret_cast<char**>(m_Bottom);
				if (nextpos)
				{
					pItem.FromBlock(m_Bottom + sizeof(char*));
					m_Bottom = nextpos;
					return true;
				}
				return false;
			}
			Node* next;

		private:
			inline char* AlignNextPosition(unsigned int nSize, unsigned int nAlign)
			{
				unsigned int nNeededAlign = nAlign > sizeof(char*) ? nAlign : sizeof(char*);
				char* next = m_Top + nSize + sizeof(char*);
				next += (reinterpret_cast<UINT_PTR>(next) % nNeededAlign);
				if (next < m_End)
					return next;
				else
					return 0;
			}

			char* m_Top;
			char* m_Bottom;
			char* m_End;
			char m_Block[nBlockSize];
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
		return false;
	}

private:

	void Initialize()
	{
		Node* node = new Node();
		if (node == 0)
			throw E_INSUFMEMORY;
		node->next = 0;
		m_Top = node;
		m_Bottom = node;
	}

	Node* m_Top;
	Node* m_Bottom;
};

template<typename T, int nBlockSize>
class CUnboundBlockStack
{
private:

	class Node {
	public:
		Node()
		{
			m_Top = m_Block;
			memset(m_Block, 0, sizeof(m_Block));
			m_End = &m_Block[nBlockSize - 1 - sizeof(char*)];
		}

		bool Enqueue(T& pItem)
		{
			char* nextpos = AlignNextPosition(pItem.BlockSize(), __alignof(T));
			if (nextpos)
			{
				pItem.ToBlock(m_Top);
				*reinterpret_cast<char**>(nextpos) = m_Top;
				m_Top = nextpos + sizeof(char*);
				return true;
			}
			return false;
		}

		bool Dequeue(T& pItem)
		{
			if (m_Top > m_Block)
			{
				char* nextpos = *reinterpret_cast<char**>(m_Top - sizeof(char*));
				if (nextpos)
				{
					pItem.FromBlock(nextpos);
					m_Top = nextpos;
					return true;
				}
			}
			return false;
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
		char m_Block[nBlockSize];
	};

public:
	CUnboundBlockStack()
	{
		m_Top = 0;
	}

	~CUnboundBlockStack()
	{
		while (Node* node = m_Top)
		{
			m_Top = node->next;
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
		node->next = m_Top;
		m_Top = node;
		m_Top->Enqueue(pItem);
	}

	bool Dequeue(T& pItem)
	{
		if (m_Top == 0)
			return false;
		if (m_Top->Dequeue(pItem))
			return true;

		Node* node = m_Top->next;
		if (node)
		{
			delete m_Top;
			m_Top = node;
			return node->Dequeue(pItem);
		}
		return false;
	}

private:
	void Initialize()
	{
		Node* node = new Node();
		if (node == 0)
			throw E_INSUFMEMORY;
		node->next = 0;
		m_Top = node;
	}

	Node* m_Top;
};

// very simple array class, use only with blittable (struct's with custom constructor/destructor) types
template<typename T>
class CArray
{
public:
	CArray() : m_Index(-1), m_MaxElements(0), m_AutoGrow(0), m_Array(0) {}
	
	void AutoGrow(int nAutoGrow)
	{
		m_AutoGrow = nAutoGrow;
	}

	int Reserve(int nMaxElements, bool bZeroMemory = false, bool bSetMaxIndex = false)
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

	int Truncate(int nElements)
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

	int GetCount() const
	{
		return m_Index + 1;
	}

	int GetIndex() const
	{
		return m_Index;
	}

	void SetIndex(int nIndex)
	{
		assert(nIndex == -1 || (nIndex >= 0 && nIndex < m_MaxElements));
		m_Index = nIndex;
	}

	int Add(const T& element)
	{	
		if (m_Index + 1 >= m_MaxElements)
		{
			size_t nMaxElements = m_MaxElements + 
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
		assert(nIndex >= 0 && nIndex < m_MaxElements);
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
	int m_MaxElements;
	int m_AutoGrow;
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

	void Allocate(int nCount)
	{
		assert(m_Array == 0);
		m_Array = new T[nCount];
		if (m_Array == 0)
			throw E_INSUFMEMORY;
		m_Count = nCount;
	}

	T* operator[](int nIndex)
	{
		assert(nIndex >= 0 && nIndex < m_Count);
		return &m_Array[nIndex];
	}

	int GetCount() const
	{
		return m_Count;
	}

private:
	T* m_Array;
	int m_Count;
};

template<typename T>
class CDynamicPtrArray
{
public:
	CDynamicPtrArray() : m_Array(0), m_Size(0), m_Elements(0), m_GrowBy(4)  {}
	~CDynamicPtrArray()
	{
		for (int xj = 0; xj < m_Elements; xj++)
			delete m_Array[xj];
		delete[] m_Array;
	}

	void Free()
	{
		assert(m_Array == 0);
		for (int xj = 0; xj < m_Elements; xj++)
			delete m_Array[xj];
		delete[] m_Array;
		m_Array = 0;
		m_Size = 0;
		m_Elements = 0;
	}

	void SetCount(int nCount)
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
		int xj;
		int nextIndex = m_Elements;
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
		assert(nIndex >= 0 && nIndex < m_Size);
		return m_Array[nIndex];
	}

	bool IsEmpty()
	{
		return m_Elements == 0;
	}

	int GetCount()
	{
		return m_Elements;
	}

	int GetSize()
	{
		return m_Size;
	}

private:
	T** m_Array;
	int m_Elements;
	int m_Size;
	int m_GrowBy;
};

#endif // _VFP2CDATASTRUCTURE_H__