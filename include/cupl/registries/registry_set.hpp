#ifndef cupl_registries_registry_set_hpp
#define cupl_registries_registry_set_hpp

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
	template <typename Key, typename Hash = std::hash<Key>>
	class registry_set
	{
		public:
			using key_type = Key;

			registry_set();
			registry_set(const registry_set&) = delete;
			registry_set(registry_set&&) = default;

			explicit operator std::unordered_set<Key, Hash>() const;

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
						boost::concurrent_flat_map<key_type, size_t, Hash> mRegistrationsCount;
						boost::signals2::signal<void (const key_type &)> mAdded;
						boost::signals2::signal<void (const key_type &)> mRemoved;

						virtual void increment(const key_type &key) override final;
						virtual void decrement(const key_type &key) override final;
			};

			std::shared_ptr<Impl> mImpl;
	};

	template <typename Key, typename Hash>
	inline void registry_set<Key, Hash>::Impl::increment(const key_type &key)
	{
		if (mRegistrationsCount.try_emplace_or_visit(key, 1, [](auto& entry) { ++entry.second; }))
			mAdded(key);
	}

	template <typename Key, typename Hash>
	inline void registry_set<Key, Hash>::Impl::decrement(const key_type &key)
	{
		if (mRegistrationsCount.erase_if(key, [](auto& entry) { return --entry.second <= 0; }))
			mRemoved(key);
	}

	template <typename Key, typename Hash>
	inline registry_set<Key, Hash>::registry_set() :
		mImpl(new Impl())
	{
	}

	template <typename Key, typename Hash>
	inline registry_set<Key, Hash>::operator std::unordered_set<Key, Hash>() const
	{
		std::unordered_set<Key, Hash> result;

		mImpl->mRegistrationsCount.visit_all([&result](auto& entry) { result.insert(entry.first); });
		return result;
	}

	template <typename Key, typename Hash>
	inline bool registry_set<Key, Hash>::empty() const
	{
		return mImpl->mRegistrationsCount.empty();
	}

	template <typename Key, typename Hash>
	inline size_t registry_set<Key, Hash>::size() const
	{
		return mImpl->mRegistrationsCount.size();
	}

	template <typename Key, typename Hash>
	template <typename F>
	inline void registry_set<Key, Hash>::visit(const key_type& key, F&& function)
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const key_type&>())), void>);

		mImpl->mRegistrationsCount.visit(key, [function { std::forward<F>(function) }](const auto& entry) { function(entry.first); });
	}

	template <typename Key, typename Hash>
	template <typename F>
	inline void registry_set<Key, Hash>::visit(const key_type& key, F&& function) const
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const key_type&>())), void>);

		mImpl->mRegistrationsCount.visit(key, [function { std::forward<F>(function) }](const auto& entry) { function(entry.first); });
	}

	template <typename Key, typename Hash>
	template <typename F>
	inline void registry_set<Key, Hash>::cvisit(const key_type& key, F&& function) const
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const key_type&>())), void>);

		mImpl->mRegistrationsCount.cvisit(key, [function { std::forward<F>(function) }](const auto& entry) { function(entry.first); });
	}

	template <typename Key, typename Hash>
	template <typename F>
	inline void registry_set<Key, Hash>::visit_all(F&& function)
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const key_type&>())), void>);

		mImpl->mRegistrationsCount.visit_all([function { std::forward<F>(function) }](const auto& entry) { function(entry.first); });
	}

	template <typename Key, typename Hash>
	template <typename F>
	inline void registry_set<Key, Hash>::visit_all(F&& function) const
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const key_type&>())), void>);

		mImpl->mRegistrationsCount.visit_all([function { std::forward<F>(function) }](const auto& entry) { function(entry.first); });
	}

	template <typename Key, typename Hash>
	template <typename F>
	inline void registry_set<Key, Hash>::cvisit_all(F&& function) const
	{
		static_assert(std::is_same_v<decltype(function(std::declval<const key_type&>())), void>);

		mImpl->mRegistrationsCount.cvisit_all([function { std::forward<F>(function) }](const auto& entry) { function(entry.first); });
	}

	template <typename Key, typename Hash>
	template <typename F>
	inline void registry_set<Key, Hash>::visit_while(F&& function)
	{
		mImpl->mRegistrationsCount.visit_while([function { std::forward<F>(function) }](const auto& entry) { return function(entry.first); });
	}

	template <typename Key, typename Hash>
	template <typename F>
	inline void registry_set<Key, Hash>::visit_while(F&& function) const
	{
		mImpl->mRegistrationsCount.visit_while([function { std::forward<F>(function) }](const auto& entry) { return function(entry.first); });
	}

	template <typename Key, typename Hash>
	template <typename F>
	inline void registry_set<Key, Hash>::cvisit_while(F&& function) const
	{
		mImpl->mRegistrationsCount.cvisit_while([function { std::forward<F>(function) }](const auto& entry) { return function(entry.first); });
	}

	template <typename Key, typename Hash>
	inline registration<Key> registry_set<Key, Hash>::insert(const key_type& key)
	{
		return mImpl->insert(key);
	}

	template <typename Key, typename Hash>
	inline bool registry_set<Key, Hash>::contains(const key_type& key) const
	{
		return mImpl->mRegistrationsCount.contains(key);
	}

	template <typename Key, typename Hash>
	template <typename Handler>
	inline boost::signals2::connection registry_set<Key, Hash>::connect_added(Handler &&handler)
	{
		return mImpl->mAdded.connect(std::forward<Handler>(handler));
	}

	template <typename Key, typename Hash>
	template <typename Handler>
	inline boost::signals2::connection registry_set<Key, Hash>::connect_removed(Handler &&handler)
	{
		return mImpl->mRemoved.connect(std::forward<Handler>(handler));
	}
}

#endif
