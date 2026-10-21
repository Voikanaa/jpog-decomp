#pragma once

namespace Toshi
{

// Reference-counting pointer to a TObject that also derives from TRefCounted (inline in the
// original, e.g. ARootTask::GetRenderInterface returns one). Each holder adds a reference;
// dropping the last one deletes the object through TObject::Delete.
template <class T> class TManagedPtr
{
public:
	TManagedPtr() = default;
	TManagedPtr(T* object) { Assign(object); }
	TManagedPtr(const TManagedPtr& other) { Assign(other.m_pObject); }
	~TManagedPtr() { Release(); }

	TManagedPtr& operator=(const TManagedPtr& other)
	{
		if (m_pObject != other.m_pObject)
		{
			Release();
			Assign(other.m_pObject);
		}
		return *this;
	}

	T* Get() const { return m_pObject; }
	T* operator->() const { return m_pObject; }
	explicit operator bool() const { return m_pObject != nullptr; }

	// Drops this reference (deleting the object if it was the last).
	void Release()
	{
		if (m_pObject != nullptr && m_pObject->DecRefCount(nullptr) == 0)
			m_pObject->Delete();
		m_pObject = nullptr;
	}

private:
	void Assign(T* object)
	{
		m_pObject = object;
		if (m_pObject != nullptr)
			m_pObject->IncRefCount(nullptr);
	}

	T* m_pObject = nullptr;
};

} // namespace Toshi
