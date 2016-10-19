#ifndef FUNDAMENTAL_LIB_LINQ_H
#define FUNDAMENTAL_LIB_LINQ_H

#include "List.h"

namespace CoreLib
{
	namespace Basic
	{

		template <typename T>
		T ConstructT();

		template <typename T>
		class RemoveReference
		{
		public:
			typedef T Type;
		};

		template <typename T>
		class RemoveReference<T&>
		{
		public:
			typedef T Type;
		};

		template <typename T>
		class RemoveReference<T&&>
		{
		public:
			typedef T Type;
		};

		template<typename T>
		struct RemovePointer
		{
			typedef T Type;
		};

		template<typename T>
		struct RemovePointer<T*>
		{
			typedef T Type;
		};

		template <typename TQueryable, typename TEnumerator, typename T, typename TFunc>
		class WhereQuery
		{
		private:
			TQueryable items;
			const TFunc & func;
		public:
			WhereQuery(const TQueryable & queryable, const TFunc & f)
				: items(queryable), func(f)
			{}
			class Enumerator
			{
			private:
				TEnumerator ptr;
				TEnumerator end;
				const TFunc & func;
			public:
				Enumerator(const Enumerator &) = default;
				Enumerator(TEnumerator ptr, TEnumerator end, const TFunc & f)
					: ptr(ptr), end(end), func(f)
				{}
				T operator *() const
				{
					return *(ptr);
				}
				Enumerator& operator ++()
				{
					++ptr;
					while (ptr != end)
					{
						if (func(*ptr))
							break;
						else
							++ptr;
					}
					return *this;
				}
				Enumerator operator ++(int)
				{
					Enumerator rs = *this;
					while (rs.ptr != end)
					{
						if (func(*rs.ptr))
							break;
						++rs.ptr;
					}
					return rs;
				}
				bool operator != (const Enumerator & iter) const
				{
					return ptr != iter.ptr;
				}
				bool operator == (const Enumerator & iter) const
				{
					return ptr == iter.ptr;
				}
			};
			Enumerator begin() const
			{
				auto ptr = items.begin();
				auto end = items.end();
				while (ptr != end)
				{
					if (func(*ptr))
						break;
					++ptr;
				}
				return Enumerator(ptr, end, func);
			}
			Enumerator end() const
			{
				return Enumerator(items.end(), items.end(), func);
			}
		};

		template <typename TQueryable, typename TEnumerator, typename T>
		class SkipQuery
		{
		private:
			TQueryable items;
			int count = 0;
		public:
			SkipQuery(const TQueryable & queryable, int pCount)
				: items(queryable), count(pCount)
			{}
			class Enumerator
			{
			private:
				TEnumerator ptr;
				TEnumerator end;
			public:
				Enumerator(const Enumerator &) = default;
				Enumerator(TEnumerator pptr, TEnumerator pend)
					: ptr(pptr), end(pend)
				{
				}
				T operator *() const
				{
					return *(ptr);
				}
				Enumerator& operator ++()
				{
					++ptr;
					return *this;
				}
				Enumerator operator ++(int)
				{
					Enumerator rs = *this;
					++ptr;
					return rs;
				}
				bool operator != (const Enumerator & iter) const
				{
					return ptr != iter.ptr;
				}
				bool operator == (const Enumerator & iter) const
				{
					return ptr == iter.ptr;
				}
			};
			Enumerator begin() const
			{
				auto ptr = items.begin();
				auto end = items.end();
				for (int i = 0; i < count; i++)
					if (ptr != end)
						++ptr;
				return Enumerator(ptr, end);
			}
			Enumerator end() const
			{
				return Enumerator(items.end(), items.end());
			}
		};

		template <typename TQueryable, typename TEnumerator, typename T, typename TFunc>
		class SelectQuery
		{
		private:
			TQueryable items;
			const TFunc & func;
		public:
			SelectQuery(const TQueryable & queryable, const TFunc & f)
				: items(queryable), func(f)
			{}
			class Enumerator
			{
			private:
				TEnumerator ptr;
				TEnumerator end;
				const TFunc &func;
			public:
				Enumerator(const Enumerator &) = default;
				Enumerator(TEnumerator ptr, TEnumerator end, const TFunc & f)
					: ptr(ptr), end(end), func(f)
				{}
				auto operator *() const -> decltype(func(*ptr))
				{
					return func(*ptr);
				}
				Enumerator& operator ++()
				{
					++ptr;
					return *this;
				}
				Enumerator operator ++(int)
				{
					Enumerator rs = *this;
					++rs;
					return rs;
				}
				bool operator != (const Enumerator & iter) const
				{
					return !(ptr == iter.ptr);
				}
				bool operator == (const Enumerator & iter) const
				{
					return ptr == iter.ptr;
				}
			};
			Enumerator begin() const
			{
				return Enumerator(items.begin(), items.end(), func);
			}
			Enumerator end() const
			{
				return Enumerator(items.end(), items.end(), func);
			}
		};

		template <typename TQueryable, typename TEnumerator, typename T, typename TFunc>
		class SelectManyQuery
		{
		private:
			TQueryable items;
			const TFunc & func;
			SelectManyQuery()
			{}
		public:
			SelectManyQuery(const TQueryable & queryable, const TFunc & f)
				: items(queryable), func(f)
			{}
			template<typename TItems, typename TItemPtr>
			class Enumerator
			{
			private:
				TEnumerator ptr;
				TEnumerator end;
				const TFunc &func;
				TItems items;
				TItemPtr subPtr;
			public:
				Enumerator(const Enumerator &) = default;
				Enumerator(TEnumerator ptr, TEnumerator end, const TFunc & f)
					: ptr(ptr), end(end), func(f)
				{
					if (ptr != end)
					{
						items = f(*ptr);
						subPtr = items.begin();
					}
				}
				auto operator *() const -> decltype(*subPtr)
				{
					return *subPtr;
				}
				Enumerator& operator ++()
				{
					++subPtr;
					while (subPtr == items.end() && ptr != end)
					{
						++ptr;
						if (ptr != end)
						{
							items = func(*ptr);
							subPtr = items.begin();
						}
						else
							break;
					}

					return *this;
				}
				Enumerator operator ++(int)
				{
					Enumerator rs = *this;
					++rs;
					return rs;
				}
				bool operator != (const Enumerator & iter) const
				{
					return !operator==(iter);
				}
				bool operator == (const Enumerator & iter) const
				{
					if (ptr == iter.ptr)
					{
						if (ptr == end)
							return true;
						else
							return subPtr == iter.subPtr;
					}
					else
						return false;
				}
			};
			auto begin()->Enumerator<decltype(func(ConstructT<T>())), decltype(func(ConstructT<T>()).begin())> const
			{
				return Enumerator<decltype(func(ConstructT<T>())), decltype(func(ConstructT<T>()).begin())>(items.begin(), items.end(), func);
			}
			auto end()->Enumerator<decltype(func(ConstructT<T>())), decltype(func(ConstructT<T>()).begin())> const
			{
				return Enumerator<decltype(func(ConstructT<T>())), decltype(func(ConstructT<T>()).begin())>(items.end(), items.end(), func);
			}
		};

		template <typename T>
		struct EnumeratorType
		{
			typedef decltype(ConstructT<T>().begin()) Type;
		};

		template <typename TFunc, typename TArg>
		class ExtractReturnType
		{
		public:
			static TFunc * f;
			static TArg ConstructArg() {};
			typedef decltype((*f)(ConstructArg())) ReturnType;
		};

		template <typename T>
		class ExtractItemType
		{
		public:
			typedef typename RemovePointer<decltype(ConstructT<T>().begin())>::Type Type;
		};

		template <typename TQueryable, typename TEnumerator, typename T>
		class Queryable
		{
		private:
			TQueryable items;
		public:
			auto begin() -> decltype(items.begin()) const
			{
				return items.begin();
			}
			auto end() -> decltype(items.end()) const
			{
				return items.end();
			}
		public:
			Queryable(const TQueryable & items)
				: items(items)
			{}

			Queryable<SkipQuery<TQueryable, TEnumerator, T>, typename SkipQuery<TQueryable, TEnumerator, T>::Enumerator, T> Skip(int count) const
			{
				return Queryable<SkipQuery<TQueryable, TEnumerator, T>, typename SkipQuery<TQueryable, TEnumerator, T>::Enumerator, T>(SkipQuery<TQueryable, TEnumerator, T>(items, count));
			}

			template<typename TFunc>
			Queryable<WhereQuery<TQueryable, TEnumerator, T, TFunc>, typename WhereQuery<TQueryable, TEnumerator, T, TFunc>::Enumerator, T> Where(const TFunc & f) const
			{
				return Queryable<WhereQuery<TQueryable, TEnumerator, T, TFunc>, typename WhereQuery<TQueryable, TEnumerator, T, TFunc>::Enumerator, T>(WhereQuery<TQueryable, TEnumerator, T, TFunc>(items, f));
			}

			template<typename TFunc>
			Queryable<SelectQuery<TQueryable, TEnumerator, T, TFunc>, typename SelectQuery<TQueryable, TEnumerator, T, TFunc>::Enumerator, typename RemoveReference<typename ExtractReturnType<TFunc, T>::ReturnType>::Type> Select(const TFunc & f) const
			{
				return Queryable<SelectQuery<TQueryable, TEnumerator, T, TFunc>, typename SelectQuery<TQueryable, TEnumerator, T, TFunc>::Enumerator, typename RemoveReference<typename ExtractReturnType<TFunc, T>::ReturnType>::Type>(SelectQuery<TQueryable, TEnumerator, T, TFunc>(items, f));
			}

			template<typename TFunc>
			auto SelectMany(const TFunc & f) ->Queryable<SelectManyQuery<TQueryable, TEnumerator, T, TFunc>, typename EnumeratorType<SelectManyQuery<TQueryable, TEnumerator, T, TFunc>>::Type, typename ExtractItemType<decltype(f(ConstructT<T>()))>::Type> const
			{
				return Queryable<SelectManyQuery<TQueryable, TEnumerator, T, TFunc>, typename EnumeratorType<SelectManyQuery<TQueryable, TEnumerator, T, TFunc>>::Type, typename ExtractItemType<decltype(f(ConstructT<T>()))>::Type>(SelectManyQuery<TQueryable, TEnumerator, T, TFunc>(items, f));
			}

			template<typename TAggregateResult, typename TFunc>
			auto Aggregate(const TAggregateResult & initial, const TFunc & f) -> decltype(f(initial, *items.begin())) const
			{
				TAggregateResult rs = initial;
				for (auto && x : items)
					rs = f(rs, x);
				return rs;
			}

			template<typename TFunc>
			T & First(const TFunc & condition) const
			{
				for (auto && x : items)
					if (condition(x))
						return x;
			}

			template <typename TFunc>
			T Max(const TFunc & selector) const
			{
				return Aggregate(*items.begin(), [&](const T & v0, const T & v1)
				{
					return selector(v0) > selector(v1) ? v0 : v1;
				});
			}

			template <typename TFunc>
			T Min(const TFunc & selector) const
			{
				return Aggregate(*items.begin(), [&](const T & v0, const T & v1)
				{
					return selector(v0) < selector(v1) ? v0 : v1;
				});
			}

			template <typename TFunc>
			auto Sum(const TFunc & selector) -> decltype(selector(ConstructT<T>())) const
			{
				decltype(selector(ConstructT<T>())) rs(0);
				for (auto && x : items)
					rs = rs + selector(x);
				return rs;
			}

			T Max() const
			{
				return Aggregate(*items.begin(), [](const T & v0, const T & v1) {return v0 > v1 ? v0 : v1; });
			}

			T Min() const
			{
				return Aggregate(*items.begin(), [](const T & v0, const T & v1) {return v0 < v1 ? v0 : v1; });
			}

			T Sum() const
			{
				T rs = T(0);
				for (auto && x : items)
					rs = rs + x;
				return rs;
			}

			T Avg() const
			{
				T rs = T(0);
				int count = 0;
				for (auto && x : items)
				{
					rs = rs + x;
					count++;
				}
				return rs / count;
			}

			int Count() const
			{
				int rs = 0;
				for (auto && x : items)
					rs++;
				return rs;
			}

			List<T> ToList() const
			{
				List<T> rs;
				for (auto && val : items)
					rs.Add(val);
				return rs;
			}
		};


		template<typename T, typename TAllocator>
		inline Queryable<ArrayView<T>, T*, T> From(const List<T, TAllocator> & list)
		{
			return Queryable<ArrayView<T>, T*, T>(list.GetArrayView());
		}

		template<typename T>
		inline Queryable<ArrayView<T>, T*, T> From(const ArrayView<T> & list)
		{
			return Queryable<ArrayView<T>, T*, T>(list);
		}

		template<typename T>
		struct LinkedListView
		{
			typename LinkedList<T>::Iterator start, last;
			typename LinkedList<T>::Iterator begin()
			{
				return start;
			}
			typename LinkedList<T>::Iterator end()
			{
				return last;
			}
		};

		template<typename T>
		inline Queryable<LinkedListView<T>, LinkedNode<T>, T> From(const LinkedList<T> & list)
		{
			LinkedListView<T> view;
			view.start = list.begin();
			view.last = list.end();
			return Queryable<LinkedListView<T>, LinkedNode<T>, T>(view);
		}

		template<typename TKey, typename TValue>
		struct EnumerableDictView
		{
			typename EnumerableDictionary<TKey, TValue>::Iterator start, last;
			typename EnumerableDictionary<TKey, TValue>::Iterator begin()
			{
				return start;
			}
			typename EnumerableDictionary<TKey, TValue>::Iterator end()
			{
				return last;
			}
		};

		template<typename TKey, typename TValue>
		inline Queryable<EnumerableDictView<TKey, TValue>, typename EnumerableDictionary<TKey, TValue>::Iterator, KeyValuePair<TKey, TValue>> From(const EnumerableDictionary<TKey, TValue> & dict)
		{
			EnumerableDictView<TKey, TValue> view;
			view.start = dict.begin();
			view.last = dict.end();
			return Queryable<EnumerableDictView<TKey, TValue>, typename EnumerableDictionary<TKey, TValue>::Iterator, KeyValuePair<TKey, TValue>>(view);
		}

		template<typename TKey>
		struct EnumerableHashSetView
		{
			typename HashSetBase<TKey, EnumerableDictionary<TKey, _DummyClass>>::Iterator start, last;
			typename EnumerableHashSet<TKey>::Iterator begin()
			{
				return start;
			}
			typename EnumerableHashSet<TKey>::Iterator end()
			{
				return last;
			}
		};

		template<typename TKey>
		inline Queryable<EnumerableHashSetView<TKey>, typename HashSetBase<TKey, EnumerableDictionary<TKey, _DummyClass>>::Iterator, TKey> From(const EnumerableHashSet<TKey> & dict)
		{
			EnumerableHashSetView<TKey> view;
			view.start = dict.begin();
			view.last = dict.end();
			return Queryable<EnumerableHashSetView<TKey>, typename HashSetBase<TKey, EnumerableDictionary<TKey, _DummyClass>>::Iterator, TKey>(view);
		}
	}
}

#endif