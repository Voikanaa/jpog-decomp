#pragma once

namespace Toshi
{

// Growable array; the original defines it inline (no exports). Elements are value-initialised
// on allocation. Layout: grow size, element count, capacity, data.
template <class T> class TArray
{
public:
	TArray(int growSize, int size) : m_iGrowSize(growSize), m_iNumElements(0), m_iSize(size), m_pData(nullptr)
	{
		if (size > 0)
			m_pData = new T[size]();
	}
	TArray(const TArray& other) : m_pData(nullptr) { *this = other; }
	~TArray() { delete[] m_pData; }

	// The copy's capacity is the source's element count.
	TArray& operator=(const TArray& other)
	{
		if (this == &other)
			return *this;
		delete[] m_pData;
		m_iNumElements = other.m_iNumElements;
		m_iGrowSize = other.m_iGrowSize;
		m_iSize = other.m_iNumElements;
		m_pData = new T[m_iSize]();
		for (int i = 0; i < m_iNumElements; ++i)
			m_pData[i] = other.m_pData[i];
		return *this;
	}

	int GetNumElements() const { return m_iNumElements; }
	int GetSize() const { return m_iSize; }
	// The storage, including slots beyond the element count.
	T* Data() { return m_pData; }
	const T* Data() const { return m_pData; }
	T& operator[](int index) { return m_pData[index]; }
	const T& operator[](int index) const { return m_pData[index]; }

	// Grows by the grow size, or to the needed size if that is larger.
	T& Push(const T& value)
	{
		const int needed = m_iNumElements + 1;
		if (m_iSize < needed)
			Resize(m_iSize + m_iGrowSize < needed ? needed : m_iSize + m_iGrowSize);
		m_pData[m_iNumElements] = value;
		return m_pData[m_iNumElements++];
	}

	// Drops the last element (nothing when empty).
	void Pop()
	{
		if (m_iNumElements > 0)
			--m_iNumElements;
	}

	// Shrinking keeps the element count, as in the original; size 0 empties the array.
	void Resize(int size)
	{
		if (size == 0)
		{
			delete[] m_pData;
			m_pData = nullptr;
			m_iSize = 0;
			m_iNumElements = 0;
			return;
		}
		T* data = new T[size]();
		const int count = size < m_iNumElements ? size : m_iNumElements;
		for (int i = 0; i < count; ++i)
			data[i] = m_pData[i];
		m_iSize = size;
		delete[] m_pData;
		m_pData = data;
	}

private:
	int m_iGrowSize;
	int m_iNumElements;
	int m_iSize;
	T* m_pData;
};

} // namespace Toshi
