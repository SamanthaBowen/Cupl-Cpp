#ifndef cupl_registries_registry_map_hpp
#define cupl_registries_registry_map_hpp

#include <type_traits>
#include <unordered_map>
#include <boost/signals2/signal.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>

#include "registration.hpp"

namespace cupl
{
	/*!
	 	\ingroup Cupl_Registries
	 	\brief A collection type similar to unordered_map, but where registration objects must be kept to maintain key membership.

	 	This uses RAII, where the entry is removed when the last registration of the key is destructed.

	 	This also has an added and a removed event that can be handled.

	 	\tparam Key The key type.
	 	\tparam T The mapped type.
	 */
	template <typename Key, typename T, typename Hash = std::hash<Key>>
	class registry_map
	{
		public:
			using key_type = Key;
			using mapped_type = T;
			using value_type = std::pair<const Key, T>;

			registry_map();
			registry_map(const registry_map&) = delete;
			registry_map(registry_map&&) = default;

			explicit operator std::unordered_map<key_type, mapped_type, Hash>() const;

			// Capacity
			bool empty() const;
			size_t size() const;

			// Boost Unordered Concurrent Style Visitation
			template <typename F>
			void visit(const key_type& key, F&& function);
			template <typename F>
			void visit(const key_type& key, F&& function) const;
			template <typename F>
			void cvisit(const key_type& key, F&& function) const;
			template <typename F>
			void visit_all(F&& function);
			template <typename F>
			void visit_all(F&& function) const;
			template <typename F>
			void cvisit_all(F&& function) const;
			template <typename F>
			void visit_while(F&& function);
			template <typename F>
			void visit_while(F&& function) const;
			template <typename F>
			void cvisit_while(F&& function) const;

			/*!
				\brief Insert key into this set.

				This returns a registration that represents the keys membership within the set.
				This function will still work even if the key already exists in the set, but it will produce a new registration.
				Multiple registrations of the same key can exist, and they must all be destructed for the key to be removed.

				\returns A registration that must be kept for as long as the key should remain.
			*/
			[[nodiscard]] registration<key_type> insert(const value_type& value);

			// TODO: Document overload.
			[[nodiscard]] registration<key_type> insert(value_type&& value);

			template <typename... Args>
			[[nodiscard]] registration<key_type> emplace(Args&&... args);

			bool contains(const key_type& key) const;

			/*!
				\brief Add an event handler for when a key is added.

				\param handler A function object with signiture `void (const key_type &)`.
				\returns A boost::signals2::connection.
			*/
			template <typename Handler>
			boost::signals2::connection connect_added(Handler &&handler);

			/*!
				\brief Add an event handler for when a key is removed.

				\param handler A function object with signiture `void (const key_type &)`.
				\returns A boost::signals2::connection.
			*/
			template <typename Handler>
			boost::signals2::connection connect_removed(Handler &&handler);
		private:
			class Impl :
				public detail::registry_impl<Key, T>
				{
					friend registry_map<Key, T>;

					private:
						struct Entry
						{
							std::pair<const key_type, mapped_type> KeyValuePair;
							size_t RegistrationCount;

							inline Entry(const key_type &key, const mapped_type& mapped) :
								KeyValuePair(key, mapped),
								RegistrationCount(1)
							{
							}

							inline Entry(const key_type &key, mapped_type&& mapped) :
								KeyValuePair(key, std::move(mapped)),
								RegistrationCount(1)
							{
							}
						};

						boost::concurrent_flat_map<key_type, Entry, Hash> mEntries;
						boost::signals2::signal<void (const key_type &)> mAdded;
						boost::signals2::signal<void (const key_type &)> mRemoved;

						virtual void increment(const key_type &key) override final;
						void increment(const key_type &key, const mapped_type& mapped);
						virtual void increment(const key_type &key, mapped_type&& mapped) override final;
						virtual void decrement(const key_type &key) override final;
			};

			std::shared_ptr<Impl> mImpl;
	};

	template <typename Key, typename T, typename Hash>
	inline void registry_map<Key, T, Hash>::Impl::increment(const key_type &key)
	{
		mEntries.visit(key, [](auto& entry) { ++entry.second.RegistrationCount; });
	}

	template <typename Key, typename T, typename Hash>
	inline void registry_map<Key, T, Hash>::Impl::increment(const key_type &key, const mapped_type& mapped)
	{
		if
		(
			mEntries.try_emplace_or_visit
			(
				key, Entry(key, mapped),
				[mapped](auto& entry)
				{
					if (entry.second.KeyValuePair.second != mapped)
						throw std::logic_error("Can not register the same key with a different value.");

					++entry.second.RegistrationCount;
				}
			)
		)
			mAdded(key);
	}

	template <typename Key, typename T, typename Hash>
	inline void registry_map<Key, T, Hash>::Impl::increment(const key_type &key, mapped_type&& mapped)
	{
		if constexpr (std::is_copy_constructible_v<T>)
		{
			if
			(
				mEntries.try_emplace_or_visit
				(
					key, Entry(key, mapped),
					[mapped](auto& entry)
					{
						if (entry.second.KeyValuePair.second != mapped)
							throw std::logic_error("Can not register the same key with a different value.");

						++entry.second.RegistrationCount;
					}
				)
			)
				mAdded(key);
		}
		else
		{
			if
			(
				mEntries.try_emplace_or_visit
				(
					key, Entry(key, std::move(mapped)),
					[](auto& entry)
					{
						throw std::logic_error("Can not register the same key with a different value.");
					}
				)
			)
				mAdded(key);
		}
	}

	template <typename Key, typename T, typename Hash>
	inline void registry_map<Key, T, Hash>::Impl::decrement(const key_type &key)
	{
		if (mEntries.erase_if(key, [](auto& entry) { return --entry.second.RegistrationCount <= 0; }))
			mRemoved(key);
	}

	template <typename Key, typename T, typename Hash>
	inline registry_map<Key, T, Hash>::registry_map() :
		mImpl(new Impl())
	{
	}

	template <typename Key, typename T, typename Hash>
	inline registry_map<Key, T, Hash>::operator std::unordered_map<Key, T, Hash>() const
	{
		std::unordered_map<Key, T, Hash> result;

		mImpl->mEntries.visit_all([&result](auto& entry) { result.emplace(entry.first, entry.second.Mapped); });
		return result;
	}

	template <typename Key, typename T, typename Hash>
	inline bool registry_map<Key, T, Hash>::empty() const
	{
		return mImpl->mEntries.empty();
	}

	template <typename Key, typename T, typename Hash>
	inline size_t registry_map<Key, T, Hash>::size() const
	{
		return mImpl->mEntries.size();
	}

	template <typename Key, typename T, typename Hash>
	template <typename F>
	inline void registry_map<Key, T, Hash>::visit(const key_type& key, F&& function)
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const value_type&>())), void>);

		mImpl->mEntries.visit(key, [function { std::forward<F>(function) }](auto& entry) { function(entry.second.KeyValuePair); });
	}

	template <typename Key, typename T, typename Hash>
	template <typename F>
	inline void registry_map<Key, T, Hash>::visit(const key_type& key, F&& function) const
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const value_type&>())), void>);

		mImpl->mEntries.visit(key, [function { std::forward<F>(function) }](const auto& entry) { function(entry.second.KeyValuePair); });
	}

	template <typename Key, typename T, typename Hash>
	template <typename F>
	inline void registry_map<Key, T, Hash>::cvisit(const key_type& key, F&& function) const
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const value_type&>())), void>);

		mImpl->mEntries.cvisit(key, [function { std::forward<F>(function) }](const auto& entry) { function(entry.second.KeyValuePair); });
	}

	template <typename Key, typename T, typename Hash>
	template <typename F>
	inline void registry_map<Key, T, Hash>::visit_all(F&& function)
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const value_type&>())), void>);

		mImpl->mEntries.visit_all([function { std::forward<F>(function) }](auto& entry) { function(entry.second.KeyValuePair); });
	}

	template <typename Key, typename T, typename Hash>
	template <typename F>
	inline void registry_map<Key, T, Hash>::visit_all(F&& function) const
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const value_type&>())), void>);

		mImpl->mEntries.visit([function { std::forward<F>(function) }](const auto& entry) { function(entry.second.KeyValuePair); });
	}

	template <typename Key, typename T, typename Hash>
	template <typename F>
	inline void registry_map<Key, T, Hash>::cvisit_all(F&& function) const
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const value_type&>())), void>);

		mImpl->mEntries.cvisit_all([function { std::forward<F>(function) }](const auto& entry) { function(entry.second.KeyValuePair); });
	}

	template <typename Key, typename T, typename Hash>
	template <typename F>
	inline void registry_map<Key, T, Hash>::visit_while(F&& function)
	{
		mImpl->mEntries.visit_while([function { std::forward<F>(function) }](auto& entry) { return function(entry.second.KeyValuePair); });
	}

	template <typename Key, typename T, typename Hash>
	template <typename F>
	inline void registry_map<Key, T, Hash>::visit_while(F&& function) const
	{
		mImpl->mEntries.visit_while([function { std::forward<F>(function) }](auto& entry) { return function(entry.second.KeyValuePair); });
	}

	template <typename Key, typename T, typename Hash>
	template <typename F>
	inline void registry_map<Key, T, Hash>::cvisit_while(F&& function) const
	{
		mImpl->mEntries.cvisit_while([function { std::forward<F>(function) }](auto& entry) { return function(entry.second.KeyValuePair); });
	}

	template <typename Key, typename T, typename Hash>
	inline registration<Key> registry_map<Key, T, Hash>::insert(const value_type& value)
	{
		return mImpl->insert(value.first, value.second);
	}

	template <typename Key, typename T, typename Hash>
	inline registration<Key> registry_map<Key, T, Hash>::insert(value_type&& value)
	{
		return mImpl->insert(std::move(value.first), std::move(value.second));
	}

	template <typename Key, typename T, typename Hash>
	template <typename... Args>
	inline registration<Key> registry_map<Key, T, Hash>::emplace(Args&&... args)
	{
		return insert(value_type(std::forward<Args>(args)...));
	}

	template <typename Key, typename T, typename Hash>
	inline bool registry_map<Key, T, Hash>::contains(const key_type& key) const
	{
		return mImpl->mEntries.contains(key);
	}

	template <typename Key, typename T, typename Hash>
	template <typename Handler>
	inline boost::signals2::connection registry_map<Key, T, Hash>::connect_added(Handler &&handler)
	{
		return mImpl->mAdded.connect(std::forward<Handler>(handler));
	}

	template <typename Key, typename T, typename Hash>
	template <typename Handler>
	inline boost::signals2::connection registry_map<Key, T, Hash>::connect_removed(Handler &&handler)
	{
		return mImpl->mRemoved.connect(std::forward<Handler>(handler));
	}
}

#endif
