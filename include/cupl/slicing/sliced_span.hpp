#ifndef cupl_slicing_sliced_span_hpp
#define cupl_slicing_sliced_span_hpp

#include <span>

#include "slice.hpp"

namespace cupl
{
	namespace detail
	{
		template <typename T>
		class sliced_span_const_iterator;

		template <typename T>
		class sliced_span_iterator;

		template <typename T, typename OutIter>
		constexpr OutIter copy
		(
			sliced_span_const_iterator<T> begin,
			sliced_span_const_iterator<T> end,
			OutIter destination
		);

		template <typename T, typename OutIter>
		constexpr OutIter copy
		(
			sliced_span_iterator<T> begin,
			sliced_span_iterator<T> end,
			OutIter destination
		);

		template <typename T>
		constexpr sliced_span_iterator<T> copy
		(
			const T *begin,
			const T *end,
			sliced_span_iterator<T> destination
		);
	}
	
	/*!
		\ingroup Cupl_Slicing
		\brief Represents a slice of an array created using a slice.
	*/
	template <typename T>
	class sliced_span
	{
		public:
			using const_iterator =	detail::sliced_span_const_iterator<T>;
			using iterator =		detail::sliced_span_iterator<T>;

			constexpr sliced_span(std::span<T> span, slice slice);

			constexpr iterator begin() const;
			constexpr iterator end() const;
			constexpr const_iterator cbegin() const;
			constexpr const_iterator cend() const;
			constexpr size_t size() const;
			constexpr bool is_contiguous() const;
		private:
			std::span<T> mSpan;
			slice mSlice;
	};

	namespace detail
	{
		template <typename T>
		class sliced_span_iterator
		{
			friend cupl::sliced_span<T>;

			template <typename U>
			friend constexpr sliced_span_iterator<U> copy
			(
				const U *begin,
				const U *end,
				sliced_span_iterator<U> destination
			);

			public:
				constexpr explicit operator T *() const;

				constexpr bool operator ==(const sliced_span_iterator &rhs) const;
				constexpr bool operator !=(const sliced_span_iterator &rhs) const;

				constexpr T &operator *() const;
				constexpr T *operator ->() const;
				constexpr sliced_span_iterator operator +(size_t i);
				constexpr sliced_span_iterator &operator ++();
				constexpr sliced_span_iterator &operator +=(size_t i);
			private:
				T *mPtr;
				size_t mGap;
				size_t mCurrentSegmentRemaining;
				size_t mSegment;

				constexpr sliced_span_iterator(T *ptr, size_t gap, size_t currentSegmentRemaining, size_t segment);
		};

		template <typename T>
		class sliced_span_const_iterator
		{
			template <typename U, typename OutIter>
			friend constexpr OutIter copy
			(
				sliced_span_const_iterator<U> begin,
				sliced_span_const_iterator<U> end,
				OutIter destination
			);

			public:
				constexpr sliced_span_const_iterator(sliced_span_iterator<T> other);

				constexpr explicit operator const T *() const;

				constexpr bool operator ==(const sliced_span_const_iterator &rhs) const;
				constexpr bool operator !=(const sliced_span_const_iterator &rhs) const;

				constexpr const T &operator *() const;
				constexpr const T *operator ->() const;
				constexpr sliced_span_const_iterator &operator ++();
				constexpr sliced_span_const_iterator &operator +=(size_t i);
			private:
				sliced_span_iterator<T> mIter;
		};

		template <typename T>
		inline constexpr sliced_span_iterator<T>::sliced_span_iterator
		(T *ptr, size_t gap, size_t currentSegmentRemaining, size_t segment) :
			mPtr(ptr),
			mGap(gap),
			mCurrentSegmentRemaining(currentSegmentRemaining),
			mSegment(segment)
		{
		}

		template <typename T>
		inline constexpr sliced_span_const_iterator<T>::sliced_span_const_iterator(sliced_span_iterator<T> other) :
			mIter(other)
		{
		}

		template <typename T>
		constexpr sliced_span_iterator<T>::operator T *() const
		{
			return mPtr;
		}

		template <typename T>
		constexpr bool sliced_span_iterator<T>::operator ==(const sliced_span_iterator &rhs) const
		{
			return
				mPtr == rhs.mPtr and
				mGap == rhs.mGap and
				mCurrentSegmentRemaining == rhs.mCurrentSegmentRemaining and
				mSegment == rhs.mSegment;
		}

		template <typename T>
		constexpr bool sliced_span_iterator<T>::operator !=(const sliced_span_iterator &rhs) const
		{
			return
				mPtr != rhs.mPtr or
				mGap != rhs.mGap or
				mCurrentSegmentRemaining != rhs.mCurrentSegmentRemaining or
				mSegment != rhs.mSegment;
		}

		template <typename T>
		inline constexpr T &sliced_span_iterator<T>::operator *() const
		{
			return *mPtr;
		}

		template <typename T>
		inline constexpr T *sliced_span_iterator<T>::operator ->() const
		{
			return mPtr;
		}

		template <typename T>
		inline constexpr sliced_span_iterator<T> sliced_span_iterator<T>::operator +(size_t i)
		{
			auto iter = *this;

			iter += i;
			return iter;
		}

		template <typename T>
		inline constexpr sliced_span_iterator<T> &sliced_span_iterator<T>::operator ++()
		{
			++mPtr;

			if (mGap != 0)
			{
				if (--mCurrentSegmentRemaining <= 0)
				{
					mPtr += mGap;
					mCurrentSegmentRemaining = mSegment;
				}
			}

			return *this;
		}

		template <typename T>
		inline constexpr sliced_span_iterator<T> &sliced_span_iterator<T>::operator +=(size_t i)
		{
			if (mGap == 0)
				mPtr += i;
			else
			{
				// Increment within segment or snap to beginning of the next segment.
				{
					const auto dist = std::min(mCurrentSegmentRemaining, i);

					mPtr += dist;
					mCurrentSegmentRemaining -= dist;
					i -= dist;

					if (mCurrentSegmentRemaining == 0)
					{
						mPtr += mGap;
						mCurrentSegmentRemaining = mSegment;
					}
				}

				size_t stridesIncrement = i / mSegment;
				size_t indexWithinSegment = i % mSegment;

				mPtr += stridesIncrement*(mSegment + mGap);
				mPtr += indexWithinSegment;
				mCurrentSegmentRemaining = mSegment - indexWithinSegment;
			}

			return *this;
		}

		template <typename T, typename OutIter>
		inline constexpr OutIter copy
		(
			sliced_span_iterator<T> begin,
			sliced_span_iterator<T> end,
			OutIter destination
		)
		{
			if ((begin.mGap == 0) and (end.mGap == 0))
				return copy(begin.mPtr, end.mPtr, destination);
			else
			{
				while (begin.mPtr < end.mPtr)
				{
					const auto dist = std::min(begin.mCurrentSegmentRemaining, size_t(end.mPtr - begin.mPtr));

					destination = copy(begin.mPtr, begin.mPtr + dist, destination);
					begin += dist;
				}

				return destination;
			}
		}

		template <typename T>
		constexpr sliced_span_iterator<T> copy
		(
			const T *begin,
			const T *end,
			sliced_span_iterator<T> destination
		)
		{
			if (destination.mGap == 0)
				return sliced_span_iterator<T>(std::copy(begin, end, destination.mPtr), 0, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
			else
			{
				while (begin != end)
				{
					const auto dist = std::min(destination.mCurrentSegmentRemaining, size_t(end - begin));

					std::copy(begin, begin + dist, destination.mPtr);
					begin += dist;
					destination += dist;
				}

				return destination;
			}
		}

		template <typename T>
		constexpr sliced_span_const_iterator<T>::operator const T *() const
		{
			return *mIter;
		}

		template <typename T>
		constexpr bool sliced_span_const_iterator<T>::operator ==(const sliced_span_const_iterator &rhs) const
		{
			return mIter == rhs.mIter;
		}

		template <typename T>
		constexpr bool sliced_span_const_iterator<T>::operator !=(const sliced_span_const_iterator &rhs) const
		{
			return mIter != rhs.mIter;
		}

		template <typename T>
		inline constexpr const T &sliced_span_const_iterator<T>::operator *() const
		{
			return *mIter;
		}

		template <typename T>
		inline constexpr const T *sliced_span_const_iterator<T>::operator ->() const
		{
			return mIter.operator ->();
		}

		template <typename T>
		inline constexpr sliced_span_const_iterator<T> &sliced_span_const_iterator<T>::operator ++()
		{
			++mIter;
			return *this;
		}

		template <typename T>
		inline constexpr sliced_span_const_iterator<T> &sliced_span_const_iterator<T>::operator +=(size_t i)
		{
			mIter += i;
			return *this;
		}

		template <typename T, typename OutIter>
		inline constexpr OutIter copy
		(
			sliced_span_const_iterator<T> begin,
			sliced_span_const_iterator<T> end,
			OutIter destination
		)
		{
			return copy(begin.mIter, end.mIter, destination);
		}
	}

	template <typename T>
	inline constexpr sliced_span<T>::sliced_span(std::span<T> span, slice slice) :
		mSpan(span),
		mSlice(slice)
	{
	}

	template <typename T>
	inline constexpr typename sliced_span<T>::iterator sliced_span<T>::begin() const
	{
		return iterator(mSlice.begin(mSpan.data()), mSlice.gap(), mSlice.segment(), mSlice.segment());
	}

	template <typename T>
	inline constexpr typename sliced_span<T>::iterator sliced_span<T>::end() const
	{
		auto iter = begin();
		iter += size();
		return iter;
	}

	template <typename T>
	inline constexpr typename sliced_span<T>::const_iterator sliced_span<T>::cbegin() const
	{
		return const_iterator(begin());
	}

	template <typename T>
	inline constexpr typename sliced_span<T>::const_iterator sliced_span<T>::cend() const
	{
		return const_iterator(end());
	}

	template <typename T>
	inline constexpr size_t sliced_span<T>::size() const
	{
		const auto effectiveUnboundSpanSize = mSpan.size() - mSlice.begin_index();
		const auto unboundSize =
			(effectiveUnboundSpanSize/mSlice.stride())*mSlice.segment() +
			std::min((effectiveUnboundSpanSize % mSlice.stride()), mSlice.segment());

		return std::min(unboundSize, mSlice.size());
	}

	template <typename T>
	inline constexpr bool sliced_span<T>::is_contiguous() const
	{
		return mSlice.is_contiguous();
	}
}

#endif
