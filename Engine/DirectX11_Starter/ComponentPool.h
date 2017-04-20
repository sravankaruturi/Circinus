#pragma once

#include <utility>
#include <assert.h>

template <class T>
class ComponentPool
{
public:
	ComponentPool(int initialSize, int resizeAmount);
	~ComponentPool();

	int GetInvalidCount();
	int GetValidCount();

	template <class... Args>
	T* AddComponent(Args&& ...args);

	//void ReturnComponent(T component);

private:
	int m_ResizeAmount;
	int m_InvalidCount;

	T* components;
	int length;
};

template<class T>
ComponentPool<T>::ComponentPool(int initialSize, int resizePool)
{
	components = new T[initialSize];

	length = sizeof(components) / sizeof(*components);
	m_InvalidCount = length;
}

template<class T>
ComponentPool<T>::~ComponentPool()
{
	assert(components != nullptr);
	delete[] components;
}

template<class T>
int ComponentPool<T>::GetInvalidCount()
{
	return m_InvalidCount;
}

template<class T>
int ComponentPool<T>::GetValidCount()
{
	return length - this->GetInvalidCount();
}

template<class T>
template<class ...Args>
inline T * ComponentPool<T>::AddComponent(Args && ...args)
{
	//T[] newComponents = new T[length + m_ResizeAmount];

	if (this->GetInvalidCount() == 0)
	{
		//// If we can't resize, then we can not give the user back any instance.
		//if (!this.isResizeAllowed)
		//{
		//	throw new Exception("Limit Exceeded " + this.components.Length + ", and the pool was set to not resize.");
		//}

		// Create a new array with some more slots and copy over the existing components.
		T* newComponents = new T[length + m_ResizeAmount];

		for (int index = length - 1; index >= 0; --index)
		{
			/*if (index >= this->GetInvalidCount())
			{
			this.components[index].PoolId = index + this.ResizeAmount;
			}*/

			newComponents[index + m_ResizeAmount] = components[index];
		}

		components = newComponents;

		// move the invalid count based on our resize amount
		m_InvalidCount += m_ResizeAmount;
	}

	// decrement the counter
	--m_InvalidCount;

	//components[0] = T{ std::forward<Args>(args)... };
	//components[m_InvalidCount] = T{ std::forward<Args>(args)... };

	return &components[m_InvalidCount];

	//// get the next component in the list
	//T result = components[m_InvalidCount];

	//// if the component is null, we need to allocate a new instance
	//if (result == null)
	//{
	//	result = this.allocate(this.innerType);

	//	if (result == null)
	//	{
	//		throw new InvalidOperationException("The pool's allocate method returned a null object reference.");
	//	}

	//	this.components[this.InvalidCount] = result;
	//}

	//result.PoolId = this.InvalidCount;

	// Initialize the object if a delegate was provided.
	//result.Initialize();

	//return result;

	//return nullptr;
}