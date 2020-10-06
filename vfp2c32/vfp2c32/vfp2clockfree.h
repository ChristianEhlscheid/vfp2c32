#ifndef _VFP2CLOCKFREE_H__
#define _VFP2CLOCKFREE_H__

#include <malloc.h>

extern "C" void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)

#define CACHELINE_ALIGN __declspec(align(128))

template<typename T>
class Atomic
{
public:
	Atomic() {
		assert(sizeof(T) == 4);
	};
	Atomic(T val) { Store(val); };
	
	inline T Load()
	{ 
		T val = m_Val;
		_ReadWriteBarrier();
		return val;
	};

	inline void Store(T val)
	{ 
		_ReadWriteBarrier();
		m_Val = val;
	};

	inline T Increment() { return (T)InterlockedIncrement(&m_Val); }
	inline T Decrement() { return (T)InterlockedDecrement(&m_Val); }
	inline T Add(T val) { return (T)InterlockedExchangeAdd(&m_Val, val); }
	inline T Subtract(T val) { return (T)InterlockedExchangeAdd(&m_Val, -val); }
	inline T Exchange(T val) { return (T)InterlockedExchange(&m_Val, val); }

protected:
	volatile T m_Val;
};

template<class T>
struct CBoundSPSCQueueDestructor
{
	static void release(T &item) { }
};

template<class T, int nCount>
class CBoundSPSCQueue
{
public:
	CBoundSPSCQueue() : m_Head(0), m_Tail(0), m_HeadCache(0), m_TailCache(0)
	{ 

	}
	~CBoundSPSCQueue()
	{
		T cb;
		while(Pop(cb))
		{
			CBoundSPSCQueueDestructor<T>::release(cb);
		}
	}

	bool Push(const T& pItem)
	{
		LONG head = m_Head.Load();
		LONG next = increment(head);
		if (next == m_TailCache)
		{
			m_TailCache = m_Tail.Load();
		}

		if (next != m_TailCache)
		{
			m_Items[head] = pItem;
			m_Head.Store(next);
			return true;
		}
		return false;
	}

	bool Pop(T& pItem)
	{
		LONG tail = m_Tail.Load();
		if (tail == m_HeadCache)
		{
			m_HeadCache = m_Head.Load();
		}

		if (tail != m_HeadCache)
		{
			pItem = m_Items[tail];
			m_Tail.Store(increment(tail));
			return true;
		}
		return false;
	}

	bool Peek()
	{
		LONG tail = m_Tail.Load();
		if (tail == m_HeadCache)
		{
			m_HeadCache = m_Head.Load();
		}
		else
			return true;
		return tail != m_HeadCache;
	}

	void* operator new(size_t sz)
	{
		return _aligned_malloc(sz, 128);
	}

	void operator delete(void* p)
	{
		_aligned_free(p);
	}

private:
	inline LONG increment(LONG index)
	{
		return (index + 1) % (nCount + 1);
	}

	CACHELINE_ALIGN Atomic<LONG> m_Head;
	LONG m_TailCache;
	CACHELINE_ALIGN Atomic<LONG> m_Tail;
	LONG m_HeadCache;
	CACHELINE_ALIGN T m_Items[nCount];
};

template<class T>
class CUnboundSPSCQueue
{
private:
	struct Node {
		T value;
		Atomic<struct Node*> next;
	};

public:
	CUnboundSPSCQueue()
	{ 
		Node* node = new Node()
		pNode->next.Store(0);
		m_Head.Store(node);
		m_Tail.Store(node);
	}

	~CUnboundSPSCQueue() {
		while(Node* node = m_Head.Load())
		{
			m_Head.Store(node->next.Load());
			delete node;
		}	
	}

	void Push(const T& pItem)
	{
		Node* node = new Node();
		node->value = pItem;
		Node* tail = m_Tail.Load();
		tail->next.Store(node);
		m_Tail.Store(pNode);
	}

	bool Pop(T& pItem)
	{
		Node* head = m_Head.Load();
		if (Node* next = head->next.Load())
		{
			m_Head.Store(next);
			pItem = head->value;
			delete head;
			return true;
		}
		return false;
	}
	
	bool Peek()
	{
		return m_Head.next.Load() > 0;
	}

	void* operator new(size_t sz)
	{
		return _aligned_malloc(sz, 128);
	}

	void operator delete(void* p)
	{
		_aligned_free(p);
	}

private:

	CACHELINE_ALIGN Atomic<Node*> m_Head;
	CACHELINE_ALIGN Atomic<Node*> m_Tail;
};


template<class T, int nBlockSize>
class CUnboundBlockSPSCQueue
{
private:
	struct Node {
		CBoundSPSCQueue<T, (nBlockSize - sizeof(struct Node*) -
			(sizeof(CBoundSPSCQueue<T,1>) - sizeof(T))) / sizeof(T)> queue;
		Atomic<struct Node*> next;

		void* operator new(size_t sz)
		{
			return _aligned_malloc(sz, 128);
		}

		void operator delete(void* p)
		{
			_aligned_free(p);
		}
	};

public:
	CUnboundBlockSPSCQueue()
	{ 
		Node* node = new Node();
		node->next.Store(0);
		m_Head.Store(node);
		m_Tail.Store(node);
	}

	~CUnboundBlockSPSCQueue()
	{
		while(Node* node = m_Head.Load())
		{
			m_Head.Store(node->next.Load());
			delete node;
		}
	}

	void Push(const T& pItem)
	{
		Node* tail = m_Tail.Load();
		if (tail->queue.Push(pItem))
			return;

		Node* node = new Node();
		node->next.Store(0);
		node->queue.Push(pItem);

		tail->next.Store(node);
		m_Tail.Store(node);
	}

	bool Pop(T& pItem)
	{
		Node* head = m_Head.Load();
		if (head->queue.Pop(pItem))
			return true;
		Node* node = head->next.Load();
		if (node)
		{
			m_Head.Store(node);
			delete head;
			return node->queue.Pop(pItem);
		}
		return false;
	}
	
	bool Peek()
	{
		Node* head = m_Head.Load();
		return head->queue.Peek() || head->next.Load() > 0;
	}

	void* operator new(size_t sz)
	{
		return _aligned_malloc(sz, 128);
	}

	void operator delete(void* p)
	{
		_aligned_free(p);
	}

private:

	CACHELINE_ALIGN Atomic<Node*> m_Head;
	CACHELINE_ALIGN Atomic<Node*> m_Tail;
};

#endif // _VFP2CLOCKFREE_H__