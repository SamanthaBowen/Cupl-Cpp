#ifndef cupl_registries_detail_aggregate_registry_hpp
#define cupl_registries_detail_aggregate_registry_hpp

#include <atomic>
#include <vector>

#include "registry_impl.hpp"

namespace cupl
{
	namespace detail
	{
		template <typename Key>
		class aggregate_registry :
			public registry_impl<Key>
		{
			friend registration<Key>;

			private:
				std::shared_ptr<aggregate_registry<Key>> mKeepAlive;
				std::vector<registration<void>> mSubregistrations;
				std::atomic<size_t> mRegistrationCount;

				aggregate_registry(std::vector<registration<void>> subregistrations);

				void initial_increment();
				virtual void increment(const Key &key) override final;
				virtual void decrement(const Key &key) override final;
		};

		template <typename Key>
		inline aggregate_registry<Key>::aggregate_registry(std::vector<registration<void>> subregistrations) :
			mSubregistrations(std::move(subregistrations)),
			mRegistrationCount(0)
		{
		}

		template <typename Key>
		inline void aggregate_registry<Key>::initial_increment()
		{
			mKeepAlive = std::static_pointer_cast<aggregate_registry<Key>>(std::enable_shared_from_this<registry_impl<Key>>::shared_from_this());
			++mRegistrationCount;
		}

		template <typename Key>
		inline void aggregate_registry<Key>::increment(const Key &key)
		{
			++mRegistrationCount;
		}

		template <typename Key>
		inline void aggregate_registry<Key>::decrement(const Key &key)
		{
			if (--mRegistrationCount <= 0)
				mKeepAlive.reset();
		}
	}
}

#endif
