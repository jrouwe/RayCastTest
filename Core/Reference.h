// Classes to facilitate reference counting
//
// Reference counting classes keep an integer which indicates how many references
// to the object are active. Reference counting objects are derived from RefTarget
// and staT & their life with a reference count of zero. They can then be assigned
// to equivalents of pointers (Ref) which will increase the reference count immediately.
// If the destructor of Ref is called or another object is assigned to the reference
// counting pointer it will decrease the reference count of the object again. If this
// reference count becomes zero, the object is destroyed.
//
// This provides a very powerful mechanism to prevent memory leaks, but also gives
// some responsibility to the programmer. The most notable point is that you cannot
// have one object reference another and have the other reference the first one
// back, because this way the reference count of both objects will never become
// lower than 1, resulting in a memory leak. By carefully designing your classses
// (and paT &icularly identifying who owns who in the class hierarchy) you can avoid
// these problems.

#pragma once

#include <atomic>

// Forward declares
template <class T> class Ref;
template <class T> class RefConst;

// Simple class	to facilitate reference counting / releasing
// Derive your class from RefTarget and you can reference it by using Ref<classname> or RefConst<classname>
template <class T>
class RefTarget		
{	
public:
	// Constructor
	inline					RefTarget()										: mRefCount(0) { }
	inline					RefTarget(const RefTarget &)					: mRefCount(0) { }
	inline					~RefTarget()									{ assert(mRefCount == 0); } // assert no one is referencing us

	// Assignment operator
	inline RefTarget &		operator = (const RefTarget &inRHS)				{ return *this; }

	// Get current refcount of this object
	uint32					GetRefCount() const								{ return mRefCount; }

	// INTERNAL HELPER FUNCTION USED BY SERIALIZATION
	static int				sInternalGetRefCountOffset()					{ return offsetof(T, mRefCount); }

protected:
	template <class T2> friend class Ref;
	template <class T2> friend class RefConst;

	// Add or release a reference to this object
	inline void				AddRef()										{ ++mRefCount; }
	inline void				Release()										{ if (--mRefCount == 0) delete static_cast<const T *>(this); }

	atomic<uint32>			mRefCount;										// Current reference count
};							

// Pure virtual version of RefTarget
class RefTargetVirtual
{
public:
	// Virtual destructor
	virtual					~RefTargetVirtual()								{ }

	// Virtual add/remove ref
	virtual void			AddRef() = 0;
	virtual void			Release() = 0;
};

// Class for automatic referencing, this is the equivalent of a pointer to type T
// if you assign a value to this class it will increment the reference count by one
// of this object, and if you assign something else it will decrease the reference
// count of the first object again. If it reaches a reference count of zero it will
// be deleted
template <class T>
class Ref
{
public:
	// Constructors
	inline					Ref()											: mPtr(nullptr) { }
	inline					Ref(T *inRHS)									: mPtr(inRHS) { AddRef(); }
	inline					Ref(const Ref<T> &inRHS)						: mPtr(inRHS.mPtr) { AddRef(); }
	inline					Ref(Ref<T> &&inRHS)								: mPtr(inRHS.mPtr) { inRHS.mPtr = nullptr; }
	inline					~Ref()											{ Release(); }
						
	// Assignment operators
	inline Ref<T> &			operator = (T *inRHS) 							{ if (mPtr != inRHS) { Release(); mPtr = inRHS; AddRef(); } return *this; }
	inline Ref<T> &			operator = (const Ref<T> &inRHS)				{ if (mPtr != inRHS.mPtr) { Release(); mPtr = inRHS.mPtr; AddRef(); } return *this; }
	inline Ref<T> &			operator = (Ref<T> &&inRHS)						{ if (mPtr != inRHS.mPtr) { Release(); mPtr = inRHS.mPtr; inRHS.mPtr = nullptr; } return *this; }
						
	// Casting operators
	inline					operator T * const () const						{ return mPtr; }
	inline					operator T *()									{ return mPtr; }
						
	// Access like a normal pointer
	inline T * const 		operator -> () const							{ return mPtr; }
	inline T *				operator -> ()									{ return mPtr; }
	inline T &				operator * () const								{ return *mPtr; }

	// Comparison
	inline bool				operator == (const T * inRHS) const				{ return mPtr == inRHS; }
	inline bool				operator == (const Ref<T> &inRHS) const			{ return mPtr == inRHS.mPtr; }
	inline bool				operator != (const T * inRHS) const				{ return mPtr != inRHS; }
	inline bool				operator != (const Ref<T> &inRHS) const			{ return mPtr != inRHS.mPtr; }

	// Get pointer
	inline T * 				GetPtr() const									{ return mPtr; }
	inline T *				GetPtr()										{ return mPtr; }

	// INTERNAL HELPER FUNCTION USED BY SERIALIZATION
	void **					InternalGetPointer()							{ return reinterpret_cast<void **>(&mPtr); }

private:
	template <class T2> friend class RefConst;

	// Use "variable = nullptr;" to release an object, do not call these functions
	inline void				AddRef()										{ if (mPtr != nullptr) mPtr->AddRef(); }
	inline void				Release()										{ if (mPtr != nullptr) mPtr->Release(); }
	
	T *						mPtr;											// Pointer to object that we are reference counting
};						

// Class for automatic referencing, this is the equivalent of a CONST pointer to type T
// if you assign a value to this class it will increment the reference count by one
// of this object, and if you assign something else it will decrease the reference
// count of the first object again. If it reaches a reference count of zero it will
// be deleted
template <class T>
class RefConst
{
public:
	// Constructors
	inline					RefConst()										: mPtr(nullptr) { }
	inline					RefConst(const T * inRHS)						: mPtr(inRHS) { AddRef(); }
	inline					RefConst(const RefConst<T> &inRHS)				: mPtr(inRHS.mPtr) { AddRef(); }
	inline					RefConst(RefConst<T> &&inRHS)					: mPtr(inRHS.mPtr) { inRHS.mPtr = nullptr; }
	inline					RefConst(const Ref<T> &inRHS)					: mPtr(inRHS.mPtr) { AddRef(); }
	inline					RefConst(Ref<T> &&inRHS)						: mPtr(inRHS.mPtr) { inRHS.mPtr = nullptr; }
	inline					~RefConst()										{ Release(); }
						
	// Assignment operators
	inline RefConst<T> &	operator = (const T * inRHS) 					{ if (mPtr != inRHS) { Release(); mPtr = inRHS; AddRef(); } return *this; }
	inline RefConst<T> &	operator = (const RefConst<T> &inRHS)			{ if (mPtr != inRHS.mPtr) { Release(); mPtr = inRHS.mPtr; AddRef(); } return *this; }
	inline RefConst<T> &	operator = (RefConst<T> &&inRHS)				{ if (mPtr != inRHS.mPtr) { Release(); mPtr = inRHS.mPtr; inRHS.mPtr = nullptr; } return *this; }
	inline RefConst<T> &	operator = (const Ref<T> &inRHS)				{ if (mPtr != inRHS.mPtr) { Release(); mPtr = inRHS.mPtr; AddRef(); } return *this; }
	inline RefConst<T> &	operator = (Ref<T> &&inRHS)						{ if (mPtr != inRHS.mPtr) { Release(); mPtr = inRHS.mPtr; inRHS.mPtr = nullptr; } return *this; }
						
	// Casting operators
	inline					operator const T * () const						{ return mPtr; }
						
	// Access like a normal pointer
	inline const T * 	 	operator -> () const							{ return mPtr; }
	inline const T &		operator * () const								{ return *mPtr; }

	// Comparison
	inline bool				operator == (const T * inRHS) const				{ return mPtr == inRHS; }
	inline bool				operator == (const RefConst<T> &inRHS) const	{ return mPtr == inRHS.mPtr; }
	inline bool				operator == (const Ref<T> &inRHS) const			{ return mPtr == inRHS.mPtr; }
	inline bool				operator != (const T * inRHS) const				{ return mPtr != inRHS; }
	inline bool				operator != (const RefConst<T> &inRHS) const	{ return mPtr != inRHS.mPtr; }
	inline bool				operator != (const Ref<T> &inRHS) const			{ return mPtr != inRHS.mPtr; }

	// Get pointer
	inline const T * 		GetPtr() const									{ return mPtr; }

	// INTERNAL HELPER FUNCTION USED BY SERIALIZATION
	void **					InternalGetPointer()							{ return const_cast<void **>(reinterpret_cast<const void **>(&mPtr)); }

private:
	// Use "variable = nullptr;" to release an object, do not call these functions
	inline void				AddRef()										{ if (mPtr != nullptr) const_cast<T *>(mPtr)->AddRef(); }
	inline void				Release()										{ if (mPtr != nullptr) const_cast<T *>(mPtr)->Release(); }
	
	const T *				mPtr;											// Pointer to object that we are reference counting
};						

// Declare std::hash for Ref and RefConst
namespace std
{
	template <class T> 
	struct hash<Ref<T>>
	{
		size_t operator () (const Ref<T> &inRHS) const
		{
			return hash<T *> { }(inRHS.GetPtr());
		}
	};

	template <class T> 
	struct hash<RefConst<T>>
	{
		size_t operator () (const RefConst<T> &inRHS) const
		{
			return hash<const T *> { }(inRHS.GetPtr());
		}
	};
}
