#ifndef cupl_registries_detail_registry_impl_hpp
#define cupl_registries_detail_registry_impl_hpp

#include "../registration.hpp"

namespace cupl
{
	namespace detail
	{
		template <typename Key, typename T = void>
		class registry_impl;

		template <typename Key>
		class registry_impl<Key> :
			public std::enable_shared_from_this<registry_impl<Key>>
		{
			template <typename K, typename T>
			friend class registry_impl;

			friend registration<Key>;

			protected:
				registration<Key> insert(const Key &key);
			private:
				virtual void increment(const Key &key) = 0;
				virtual void decrement(const Key &key) = 0;
		};

		template <typename Key, typename T>
		class registry_impl :
		public registry_impl<Key, void>
		{
			protected:
				registration<Key> insert(const Key &key, const T& mapped);
			private:
				virtual void increment(const Key &key, const T& mapped) = 0;
		};

		template <typename Key>
		inline registration<Key> detail::registry_impl<Key>::insert(const Key &key)
		{
			increment(key);
			return registration<Key>(std::enable_shared_from_this<registry_impl<Key>>::shared_from_this(), key);
		}

		template <typename Key, typename T>
		inline registration<Key> detail::registry_impl<Key, T>::insert(const Key &key, const T& mapped)
		{
			increment(key, mapped);
			return registration<Key>(std::enable_shared_from_this<registry_impl<Key>>::shared_from_this(), key);
		}
	}
}

#endif
