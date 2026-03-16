#ifndef cupl_registries_registration_hpp
#define cupl_registries_registration_hpp

#include <memory>

namespace cupl
{
	template <typename Key = void>
	class registration;

	namespace detail
	{
		template <typename Key, typename T>
		class registry_impl;

		class registration_typeerased
		{
			public:
				virtual void detach_copy() const = 0;
		};

		template <typename T>
		class registration_typeerased_impl final :
			public registration_typeerased
		{
			public:
				registration_typeerased_impl(registration<T> registration);

				virtual void detach_copy() const override final;
			private:
				registration<T> mRegistration;
		};
	}

	template <>
	class registration<void>
	{
		template <typename Key>
		friend class registration;

		public:
			constexpr registration() noexcept = default;

			template <typename T>
			registration(registration<T> other);

			void reset() noexcept;
			void detach_copy() const;
			void detach();
		private:
			std::shared_ptr<detail::registration_typeerased> typeErased;
	};

	/*!
		\ingroup Cupl_Registries                                                                     *
		\brief An object that maintains a key's membership within a registry.

		This uses RAII, where the key is removed when the last registration of the key is destructed.

		\tparam Key The key type.
	*/
	template <typename Key>
	class registration
	{
		template <typename K, typename T>
		friend class detail::registry_impl;

		friend class detail::registration_typeerased_impl<Key>;

		public:
			/*!
				\brief Construct an empty registration of no key and no registry.
			*/
			constexpr registration() noexcept = default;

			registration(const registration &other);

			/*!
				\brief Move Construct

				This will reset other.
			*/
			registration(registration &&other);

			/*!
				\brief Destruct

				If this is not an empty registration, and this is the last registration of its key with its registry,
				the key will be removed from the registry.
			*/
			~registration();

			registration &operator =(const registration &rhs);

			/*!
				\brief Move Assign

				This will reset rhs.
			*/
			registration &operator =(registration &&rhs);

			/*!
				\brief Reset this registration so that it is empty with no key and no registry.
			*/
			void reset() noexcept;

			const Key& get_key() const;

			/*!
				\brief Detach a copy of this registration, ensuring the key's membership within the registry is permanent.
			*/
			void detach_copy() const;

			/*!
				\brief Detach this registration, ensuring the key's membership within the registry is permanent.

				This will reset this.
			*/
			void detach();
		private:
			std::weak_ptr<detail::registry_impl<Key, void>> mRegistry;
			// TODO: Untagged Optional
			std::optional<Key> mKey;

			registration(std::shared_ptr<detail::registry_impl<Key, void>> registry, const Key& key);
	};

	template <typename Key>
	inline registration<Key>::registration(std::shared_ptr<detail::registry_impl<Key, void>> registry, const Key& key) :
		mRegistry(registry),
		mKey(key)
	{
	}

	template <typename Key>
	inline registration<Key>::registration(const registration &other) :
		mRegistry(other.mRegistry),
		mKey(other.mKey)
	{
		if (auto registry = mRegistry.lock())
			registry.increment(*mKey);
	}

	template <typename Key>
	inline registration<Key>::registration(registration &&other) :
		mRegistry(std::exchange(other.mRegistry, nullptr)),
		mKey(std::exchange(other.mKey, std::nullopt))
	{
	}

	template <typename Key>
	inline registration<Key>::~registration()
	{
		if (auto registry = mRegistry.lock())
			registry->decrement(*mKey);
	}

	template <typename Key>
	inline registration<Key> &registration<Key>::operator =(const registration &rhs)
	{
		if (auto registry = mRegistry.lock())
			registry->decrement(*mKey);

		mRegistry = rhs.mRegistry;
		mKey = rhs.mKey;

		if (const auto registry = mRegistry.lock())
			registry->increment(*mKey);

		return *this;
	}

	template <typename Key>
	inline registration<Key> &registration<Key>::operator =(registration &&rhs)
	{
		if (auto registry = mRegistry.lock())
			registry->decrement(*mKey);

		mRegistry = std::exchange(rhs.mRegistry, nullptr);
		mKey = std::exchange(rhs.mKey, std::nullopt);

		return *this;
	}

	template <typename Key>
	inline void registration<Key>::reset() noexcept
	{
		if (auto registry = mRegistry.lock())
			registry->decrement(*mKey);

		mRegistry.reset();
		mKey.reset();
	}

	template <typename Key>
	inline void registration<Key>::detach_copy() const
	{
		if (const auto registry = mRegistry.lock())
			registry->increment(*mKey);
	}

	template <typename Key>
	inline void registration<Key>::detach()
	{
		mRegistry.reset();
		mKey.reset();
	}
}

#include "detail/registry_impl.hpp"

#endif
