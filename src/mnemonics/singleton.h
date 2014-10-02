namespace Language
{
	template <class T>
	class Singleton
	{
		Singleton() {}
		Singleton(Singleton &s) {}
		Singleton& operator=(const Singleton&) {}
	public:
		static T* instance()
		{
			static T* obj = new T;
			return obj;
		}
	};
}
