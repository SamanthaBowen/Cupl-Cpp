#ifndef cupl_registries_registry_set_hpp
#define cupl_registries_registry_set_hpp

#include <atomic>
#include <unordered_set>
#include <boost/signals2/signal.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>

#include "registration.hpp"

namespace cupl
{
	/*!
	 	\ingroup Cupl_Registries
	 	\brief A collection type similar to unordered_set, but where registration objects must be kept to maintain key membership.

	 	This uses RAII, where the key is removed when the last registration of the key is destructed.

	 	This also has an added and a removed event that can be handled.

	 	\tparam Key The key type.
	 */
	template <typename Key>
	class registry_set
	{
		public:
			using key_type = Key;

			registry_set();
			registry_set(const registry_set&) = delete;
			registry_set(registry_set&&) = default;

			explicit operator std::unordered_set<Key>() const;

			// Capacity
			bool empty() const;
			size_t size() const;

			/*!
				\brief Insert key into this set.

				This returns a registration that represents the keys membership within the set.
				This function will still work even if the key already exists in the set, but it will produce a new registration.
				Multiple registrations of the same key can exist, and they must all be destructed for the key to be removed.

				\returns A registration that must be kept for as long as the key should remain.
			*/
			[[nodiscard]] registration<key_type> insert(const key_type &key);

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
				public detail::registry_impl<Key>
				{
					friend registry_set<Key>;

					private:
						boost::concurrent_flat_map<key_type, size_t> mRegistrationsCount;
						boost::signals2::signal<void (const key_type &)> mAdded;
						boost::signals2::signal<void (const key_type &)> mRemoved;

						virtual void increment(const key_type &key) override;
						virtual void decrement(const key_type &key) override;
			};

			std::shared_ptr<Impl> mImpl;
	};

	template <typename Key>
	inline void registry_set<Key>::Impl::increment(const key_type &key)
	{
		if (mRegistrationsCount.try_emplace_or_visit(key, 1, [](auto& entry) { ++entry.second; }))
			mAdded(key);
	}

	template <typename Key>
	inline void registry_set<Key>::Impl::decrement(const key_type &key)
	{
		if (mRegistrationsCount.erase_if(key, [](auto& entry) { return --entry.second <= 0; }))
			mRemoved(key);
	}

	template <typename Key>
	inline registry_set<Key>::registry_set() :
		mImpl(new Impl())
	{
	}

	template <typename Key>
	inline registry_set<Key>::operator std::unordered_set<Key>() const
	{
		return mImpl->mRegistrationsCount;
	}

	template <typename Key>
	inline bool registry_set<Key>::empty() const
	{
		return mImpl->mRegistrationsCount.empty();
	}

	template <typename Key>
	inline size_t registry_set<Key>::size() const
	{
		return mImpl->mRegistrationsCount.size();
	}

	template <typename Key>
	inline registration<Key> registry_set<Key>::insert(const key_type& key)
	{
		return mImpl->insert(key);
	}

	template <typename Key>
	inline bool registry_set<Key>::contains(const key_type& key) const
	{
		return mImpl->mRegistrationsCount.contains(key);
	}

	template <typename Key>
	template <typename Handler>
	inline boost::signals2::connection registry_set<Key>::connect_added(Handler &&handler)
	{
		return mImpl->mAdded.connect(std::forward<Handler>(handler));
	}

	template <typename Key>
	template <typename Handler>
	inline boost::signals2::connection registry_set<Key>::connect_removed(Handler &&handler)
	{
		return mImpl->mRemoved.connect(std::forward<Handler>(handler));
	}
}

#endif
