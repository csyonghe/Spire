#ifndef FUNDAMENTAL_LIB_SMART_POINTER_H
#define FUNDAMENTAL_LIB_SMART_POINTER_H

namespace CoreLib
{
	namespace Basic
	{
		class RefPtrDefaultDestructor
		{
		public:
			template<typename T>
			void operator ()(T * ptr)
			{
				delete ptr;
			}
		};

		class RefPtrArrayDestructor
		{
		public:
			template<typename T>
			void operator() (T * ptr)
			{
				delete [] ptr;
			}
		};

		template<typename T, typename Destructor = RefPtrDefaultDestructor>
		class RefPtr
		{
			template<typename T1, typename Destructor1>
			friend class RefPtr;
		private:
			T * pointer;
			int * refCount;
			
		public:
			RefPtr()
			{
				pointer = 0;
				refCount = 0;
			}
			RefPtr(T * ptr)
				: pointer(0), refCount(0)
			{
				this->operator=(ptr);
			}
			template<typename T1>
			RefPtr(T1 * ptr)
				: pointer(0), refCount(0)
			{
				this->operator=(ptr);
			}
			RefPtr(const RefPtr<T, Destructor> & ptr)
				: pointer(0), refCount(0)
			{
				this->operator=(ptr);
			}
			RefPtr(RefPtr<T, Destructor> && str)
				: pointer(0), refCount(0)
			{
				this->operator=(static_cast<RefPtr<T, Destructor> &&>(str));
			}
			RefPtr<T,Destructor>& operator=(T * ptr)
			{
				Dereferance();

				pointer = ptr;
				if(ptr)
				{
					refCount = new int;
					(*refCount) = 1;
				}
				else
					refCount = 0;
				return *this;
			}
			template<typename T1>
			RefPtr<T,Destructor>& operator=(T1 * ptr)
			{
				Dereferance();

				pointer = dynamic_cast<T*>(ptr);
				if(ptr)
				{
					refCount = new int;
					(*refCount) = 1;
				}
				else
					refCount = 0;
				return *this;
			}
			RefPtr<T,Destructor>& operator=(const RefPtr<T, Destructor> & ptr)
			{
				if(ptr.pointer != pointer)
				{
					Dereferance();
					pointer = ptr.pointer;
					refCount = ptr.refCount;
					if (refCount)
						(*refCount)++;
				}
				return *this;
			}

			template<typename T1>
			RefPtr(const RefPtr<T1> & ptr)
				: pointer(0), refCount(0)
			{
				this->operator=(ptr);
			}
			template<typename T1>
			RefPtr<T,Destructor> & operator = (const RefPtr<T1, Destructor> & ptr)
			{
				if(ptr.pointer != pointer)
				{
					Dereferance();
					pointer = dynamic_cast<T*>(ptr.pointer);
					if (ptr.pointer && !pointer)
						throw L"RefPtr assignment: type cast failed.";
					refCount = ptr.refCount;
					(*refCount)++;
				}
				return *this;
			}
			bool operator == (const T * ptr) const
			{
				return pointer == ptr;
			}
			bool operator != (const T * ptr) const
			{
				return pointer != ptr;
			}
			bool operator == (const RefPtr<T, Destructor> & ptr) const
			{
				return pointer == ptr.pointer;
			}
			bool operator != (const RefPtr<T, Destructor> & ptr) const
			{
				return pointer != ptr.pointer;
			}

			T* operator +(int offset) const
			{
				return pointer+offset;
			}
			T& operator [](int idx) const
			{
				return *(pointer + idx);
			}
			RefPtr<T,Destructor>& operator=(RefPtr<T, Destructor> && ptr)
			{
				if(ptr.pointer != pointer)
				{
					Dereferance();
					pointer = ptr.pointer;
					refCount = ptr.refCount;
					ptr.pointer = 0;
					ptr.refCount = 0;
				}
				return *this;
			}
			T* Release()
			{
				if(pointer)
				{
					if((*refCount) > 1)
					{
						(*refCount)--;
					}
					else
					{
						delete refCount;
					}
				}
				auto rs = pointer;
				refCount = 0;
				pointer = 0;
				return rs;
			}
			~RefPtr()
			{
				Dereferance();
			}

			void Dereferance()
			{
				if(pointer)
				{
					if((*refCount) > 1)
					{
						(*refCount)--;
					}
					else
					{
						Destructor destructor;
						destructor(pointer);
						delete refCount;
					}
				}
			}
			T & operator *() const
			{
				return *pointer;
			}
			T * operator->() const
			{
				return pointer;
			}
			T * Ptr() const
			{
				return pointer;
			}
		public:
			explicit operator bool() const 
			{
				if (pointer)
					return true;
				else
					return false;
			}
		};
	}
}

#endif