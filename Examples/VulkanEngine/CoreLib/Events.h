#ifndef GX_EVENTS_H
#define GX_EVENTS_H

#include "Common.h"
#include "List.h"
#include "Func.h"
namespace CoreLib
{
	namespace Basic
	{
	/***************************************************************************

	Events.h

	Usage:

		class A
		{
		public:
			void EventHandler(int a)
			{
				cout<<endl<<"function of object handler invoked. a*a = ";
				cout<<a*a<<endl;
			}
		};

		class B
		{
		public:
			typedef gxEvent1<int> gxOnEvent;
		public:
			gxOnEvent OnEvent;
			void DoSomething()
			{
				OnEvent.Invoke(4);
			}
		};

		void FuncHandler()
		{
			cout<<"Function invoked."<<endl;
		}

		void main()
		{
			A a;
			B b;
			b.OnEvent.Bind(&a,&A::EventHandler);	
			b.OnEvent.Bind(FuncHandler);			
			b.DoSomething();
			b.OnEvent.Unbind(FuncHandler);			
			b.OnEvent.Unbind(&a,&A::EventHandler);
			b.DoSomething();                       
		}

	***************************************************************************/
		template <typename... Arguments>
		class Event
		{
		private:
			List<RefPtr<FuncPtr<void, Arguments... >>> Handlers;
			void Bind(FuncPtr<void, Arguments...> * fobj)
			{
				Handlers.Add(fobj);
			}
			void Unbind(FuncPtr<void, Arguments...> * fobj)
			{
				int id = -1;
				for (int i = 0; i < Handlers.Count(); i++)
				{
					if ((*Handlers[i]) == fobj)
					{
						id = i;
						break;
					}
				}
				if (id != -1)
				{
					Handlers[id] = 0;
					Handlers.Delete(id);				
				}
			}
		public:
			Event()
			{
			}
			Event(const Event & e)
			{
				operator=(e);
			}
			Event & operator = (const Event & e)
			{
				for (int i = 0; i < e.Handlers.Count(); i++)
					Handlers.Add(e.Handlers[i]->Clone());
				return *this;
			}
			template <typename Class>
			Event(Class * Owner, typename MemberFuncPtr<Class, void, Arguments...>::FuncType handler)
			{
				Bind(Owner, handler);
			}
			Event(typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				Bind(f);
			}
			template <typename TFunctor>
			Event(const TFunctor & func)
			{
				Bind(func);
			}
			template <typename Class>
			void Bind(Class * Owner, typename MemberFuncPtr<Class, void, Arguments...>::FuncType handler)
			{
				Handlers.Add(new MemberFuncPtr<Class, void, Arguments...>(Owner, handler));
			}
			template <typename Class>
			void Unbind(Class * Owner, typename MemberFuncPtr<Class, void, Arguments...>::FuncType handler)
			{
				MemberFuncPtr<Class, void, Arguments...> h(Owner, handler);
				Unbind(&h);
			}
			void Bind(typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				Bind(new CdeclFuncPtr<void, Arguments...>(f));
			}
			void Unbind(typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				CdeclFuncPtr<void, Arguments...> h(f);
				Unbind(&h);
			}
			template <typename TFunctor>
			void Bind(const TFunctor & func)
			{
				Handlers.Add(new LambdaFuncPtr<TFunctor, void, Arguments...>(func));
			}
			template <typename TFunctor>
			void Unbind(const TFunctor & func)
			{
				LambdaFuncPtr<TFunctor, void, Arguments...> h(func);
				Unbind(&h);
			}
			Event & operator += (typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				Bind(f);
				return *this;
			}
			Event & operator -= (typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				Unbind(f);
				return *this;
			}
			template <typename TFunctor>
			Event & operator += (const TFunctor & f)
			{
				Bind(f);
				return *this;
			}
			template <typename TFunctor>
			Event & operator -= (const TFunctor & f)
			{
				Unbind(f);
				return *this;
			}
			void Invoke(Arguments... params) const
			{
				for (int i = 0; i < Handlers.Count(); i++)
					Handlers[i]->operator()(params...);
			}
			void operator ()(Arguments... params) const
			{
				Invoke(params...);
			}
		};
	}
}

#endif