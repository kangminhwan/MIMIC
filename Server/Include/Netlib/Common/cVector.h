#pragma once

BEGIN_NETLIB

template<typename T>
class cVector
{
private:
	typedef cVector<T>						this_type;

public:
	typedef T								value_type;
	typedef T*								pointer;
	typedef T&								reference;
	typedef T*								iterator;
	typedef const T*						const_iterator;


private:
	pointer m_pbegin;
	pointer m_pend;
	pointer m_pinternalcapacityptr;

public:
	void push_back(value_type&& value)
	{
		if (m_pend >= m_pinternalcapacityptr)
		{
			reassignment();
		}

		::new((void*)m_pend++) value_type(value);
	}

	void push_back(const value_type& value)
	{
		if (m_pend >= m_pinternalcapacityptr)
		{
			reassignment();
		}

		::new((void*)m_pend++) value_type(value);
	}

	void push_back(const this_type& value)
	{
		size_t remainingSize = static_cast<size_t>((m_pinternalcapacityptr - m_pbegin) - (m_pend - m_pbegin));
		size_t presize = static_cast<size_t>(m_pend - m_pbegin);

		size_t valueSize = static_cast<size_t>(value.m_pend - value.m_pbegin);

		if (remainingSize <= valueSize)
		{
			reassignment(remainingSize + valueSize);
		}

		m_pend = static_cast<pointer>(memcpy(m_pbegin + presize, value.m_pbegin, static_cast<size_t>(reinterpret_cast<uintptr_t>(value.m_pend) - reinterpret_cast<uintptr_t>(value.m_pbegin)))) + valueSize;

		m_pinternalcapacityptr = m_pbegin + presize + remainingSize + valueSize;
	}

	void addSize(size_t _addSize)
	{
		size_t remainingSize = static_cast<size_t>((m_pinternalcapacityptr - m_pbegin) - (m_pend - m_pbegin));

		reassignment(remainingSize + _addSize);
	}

	reference pop_back()
	{
		if (m_pbegin < m_pend)
		{
			--m_pend;
		}
#ifdef _DEBUG
		else
		{
			assert(false && "cVector::pop_back -- empty vector");
		}
#endif

		return *m_pend;
	}

	reference back()
	{
#ifdef _DEBUG
		if (m_pend <= m_pbegin)
		{
			assert(false && "cVector::back -- empty vector");
		}
#endif

		return *(m_pend - 1);
	}

	void clear()
	{
		m_pend = m_pbegin;
	}

	size_t size()
	{
		return static_cast<size_t>(m_pend - m_pbegin);
	}

	size_t capacity()
	{
		return static_cast<size_t>(m_pinternalcapacityptr - m_pbegin);
	}

	void swap(int niIndex, int njIndex)
	{
		T pTemp = NULL;

		pTemp = *(m_pbegin + niIndex);

		*(m_pbegin + niIndex) = *(m_pbegin + njIndex);

		*(m_pbegin + njIndex) = pTemp;
	}

	iterator begin()
	{
		return m_pbegin;
	}

	const_iterator begin() const
	{
		return m_pbegin;
	}

	iterator end()
	{
		return m_pend;
	}

	const_iterator end() const
	{
		return m_pend;
	}

	reference operator[](size_t n)
	{
		if (n < 0 && n >= static_cast<size_t>(m_pend - m_pbegin))
		{
			_Xran();
		}

		return *(m_pbegin + n);
	}

private:

	[[noreturn]] void _Xran() const
	{
		std::_Xout_of_range("invalid cVector<T> subscript");
	}

private:

	void reassignment(size_t addSize = 0)
	{
		const size_t nPrevSize = static_cast<size_t>(m_pend - m_pbegin);
		const size_t nNewSize = addSize == 0 ? nPrevSize > 0 ? 2 * nPrevSize : 1 : nPrevSize + addSize;
		pointer newStartData = new value_type[nNewSize];
		pointer newEndData = static_cast<pointer>(memcpy(newStartData, m_pbegin, static_cast<size_t>(reinterpret_cast<uintptr_t>(m_pend) - reinterpret_cast<uintptr_t>(m_pbegin)))) + static_cast<size_t>(m_pend - m_pbegin);

		destructor();

		m_pbegin = newStartData;
		m_pend = newEndData;
		m_pinternalcapacityptr = newStartData + nNewSize;
	}

	void destructor()
	{
		if (m_pbegin != nullptr)
		{
			delete[](char*)m_pbegin;
			m_pbegin = nullptr;
		}
	}

public:
	cVector(size_t stSize = 0) :
		m_pbegin(nullptr),
		m_pend(nullptr),
		m_pinternalcapacityptr(nullptr)
	{
		if (stSize > 0)
		{
			m_pbegin = new value_type[stSize];
			m_pend = m_pbegin;
			m_pinternalcapacityptr = m_pbegin + stSize;
		}
	}

	~cVector()
	{
		destructor();
	}
};

END_NETLIB