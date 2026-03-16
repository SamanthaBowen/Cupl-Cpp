#ifndef cupl_slicing_slice_hpp
#define cupl_slicing_slice_hpp

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <vector>
#include <boost/container_hash/hash.hpp>

namespace cupl
{
	template <typename T>
	class sliced_span;

	/*!
		\ingroup Cupl_Slicing
		\brief Represents a way to slice a sequence of elements into a new sequence.
	*/
	class slice
	{
		public:
			//! \brief Construct a contiguous slice that includes every element at and after begin.
			constexpr slice(size_t begin);

			//! \brief Construct a contiguous slice that includes every element at and after begin but before end.
			constexpr slice(size_t begin, size_t end);

			/*!
				\brief Construct a slice with a begin, end, and stride.

				The slice includes elements at indexes in the range [begin, end), skipping every stride elements after begin.
				If stride is 0, then the slice will be contiguous
			*/
			constexpr slice(size_t begin, size_t end, size_t stride);

			/*!
				\brief Construct a slice with a begin, end, stride, and segment.

				The slice includes elements at indexes in the range [begin, end) in segments of segment size,
				skipping every stride elements in between segments.
				If stride is 0, then the slice will be contiguous and the segment argument will be ignored.
			*/
			constexpr slice(size_t begin, size_t end, size_t stride, size_t segment);

			size_t hash_code() const;

			//! \brief Get a proxy represented the sequence of elements made by slicing the given span using this slice.
			template <typename T>
			constexpr sliced_span<T> operator()(std::span<T> span) const;

			template <typename T, size_t N>
			constexpr sliced_span<T> operator()(std::array<T, N>& span) const;

			template <typename T, size_t N>
			constexpr sliced_span<const T> operator()(const std::array<T, N>& span) const;

			template <typename T>
			constexpr sliced_span<T> operator()(std::vector<T>& span) const;

			template <typename T>
			constexpr sliced_span<const T> operator()(const std::vector<T>& span) const;

			constexpr bool operator ==(const slice &rhs) const;
			constexpr bool operator !=(const slice &rhs) const;
			constexpr slice operator *(size_t rhs) const;
			constexpr slice operator /(size_t rhs) const;

			constexpr size_t begin_index() const;	//!< Get the index of the first element of the slice.
			constexpr size_t end_index() const;		//!< Get the index immediately after the last element of the slice.

			/*!
				\ brief Get the number of elements in between the beginnings of each segment.

				Contiguous slices have a stride equal to size().
			*/
			constexpr size_t stride() const;

			/*!
				\brief Get the number of elements in between each segment.

				Contiguous slices have a gap of 0.
			*/
			constexpr size_t gap() const;

			/*!
				\brief Get the number of elements within each segment.

				Contiguous slices have a segment size equal to size().
			*/
			constexpr size_t segment() const;

			/*!
				\brief Get whether there are no gaps.

				This slice is contiguous if stride is 0.
			*/
			constexpr bool is_contiguous() const;

			constexpr size_t size() const;			//!< Get the total number of elements in the slice.
			constexpr bool is_divisible_by(size_t rhs) const;

			//! \brief Returns ptr.
			template <typename T>
			constexpr T *begin(T *ptr) const;

			//! \brief Returns an after-the-last-element pointer of the array pointed to by ptr.
			template <typename T>
			constexpr T *end(T *ptr) const;
		private:
			static constexpr size_t UnboundEnd = std::numeric_limits<size_t>::max();
			static constexpr size_t UnboundSize = std::numeric_limits<size_t>::max();

			size_t mBegin;
			size_t mEnd;
			size_t mGap;
			size_t mSegment;
	};

	template <typename T>
	inline constexpr T& operator<<(T& stream, slice obj)
	{
		if (obj.is_contiguous())
		{
			if (obj.end_index() == std::numeric_limits<size_t>::max())
				stream << obj.begin_index() << ":";
			else
				stream << obj.begin_index() << ":" << obj.end_index();
		}
		else if (obj.segment() == 1)
		{
			if (obj.end_index() == std::numeric_limits<size_t>::max())
				stream << obj.begin_index() << "::" << obj.stride();
			else
				stream << obj.begin_index() << ":" << obj.end_index() << ":" << obj.stride();
		}
		else
		{
			if (obj.end_index() == std::numeric_limits<size_t>::max())
				stream << obj.begin_index() << "::" << obj.stride() << ":" << obj.segment();
			else
				stream << obj.begin_index() << ":" << obj.end_index() << ":" << obj.stride() << ":" << obj.segment();
		}

		return stream;
	}

	inline constexpr slice::slice(size_t begin) :
		slice(begin, UnboundEnd)
	{
	}

	inline constexpr slice::slice(size_t begin, size_t end) :
		slice(begin, end, 1)
	{
	}

	inline constexpr slice::slice(size_t begin, size_t end, size_t stride) :
		slice(begin, end, stride, 1)
	{
	}

	inline constexpr slice::slice(size_t begin, size_t end, size_t stride, size_t segment) :
		mBegin(begin),
		mEnd(end),
		mSegment(segment)
	{
		if (mSegment == 0)
			throw std::invalid_argument("Segment must not be 0.");

		if (stride < mSegment)
			throw std::invalid_argument("Stride cannot be less than segment.");
		mGap = stride - mSegment;

		// If the end is not unbounded, then we can make some simplifications to the parameters.
		if (mEnd != UnboundEnd)
		{
			// If the end of a slice lands in a stride gap, we can move the end to the end of the last segment.
			if ((0 < mEnd) and (mEnd % (mSegment + mGap) == 0))
				mEnd -= mGap;
			else if (mSegment < mEnd % (mSegment + mGap))
				mEnd = (mEnd / (mSegment + mGap))*(mSegment + mGap) + mSegment;

			// If the end occurs before the first stride gap, then stride might as well be 0.
			if (mEnd - mBegin <= mSegment)
				mGap = 0;
		}

		// If there is no gap (i.e. it's contiguous), then make segment equal to the size.
		if (mGap == 0)
			mSegment = (mEnd == UnboundEnd) ? UnboundSize : mEnd - mBegin;
	}

	inline size_t slice::hash_code() const
	{
		size_t seed;

		boost::hash_combine(seed, mBegin);
		boost::hash_combine(seed, mEnd);
		boost::hash_combine(seed, mGap);
		boost::hash_combine(seed, mSegment);

		return seed;
	}

	inline constexpr bool slice::operator ==(const slice &rhs) const
	{
		return
			(mBegin == rhs.mBegin) and
			(mEnd == rhs.mEnd) and
			(mGap == rhs.mGap) and
			(mSegment == rhs.mSegment);
	}

	inline constexpr bool slice::operator !=(const slice &rhs) const
	{
		return
			(mBegin != rhs.mBegin) or
			(mEnd != rhs.mEnd) or
			(mGap != rhs.mGap) or
			(mSegment != rhs.mSegment);
	}

	inline constexpr slice slice::operator *(size_t rhs) const
	{
		if (mEnd == UnboundEnd)
			return slice(mBegin * rhs, UnboundEnd, stride() * rhs, mSegment * rhs);
		else
			return slice(mBegin * rhs, mEnd * rhs, stride() * rhs, mSegment * rhs);
	}

	inline constexpr bool slice::is_divisible_by(size_t rhs) const
	{
		return
			mBegin % rhs == 0 and
			(mEnd == UnboundEnd or mEnd % rhs == 0) and
			mGap % rhs == 0 and
			(is_contiguous() or mSegment % rhs == 0);
	}

	inline constexpr slice slice::operator /(size_t rhs) const
	{
		if (mEnd == UnboundEnd)
			return slice(mBegin / rhs, UnboundEnd, stride() / rhs, mSegment / rhs);
		else
			return slice(mBegin / rhs, mEnd / rhs, stride() / rhs, mSegment / rhs);
	}

	inline constexpr size_t slice::begin_index() const
	{
		return mBegin;
	}

	template <typename T>
	inline constexpr T *slice::begin(T *ptr) const
	{
		return ptr + mBegin;
	}

	inline constexpr size_t slice::end_index() const
	{
		return mEnd;
	}

	template <typename T>
	inline constexpr T *slice::end(T *ptr) const
	{
		if (mEnd == UnboundEnd)
			return (T *)(UnboundEnd);
		else
			return ptr + mEnd;
	}

	inline constexpr size_t slice::stride() const
	{
		return mSegment + mGap;
	}

	inline constexpr size_t slice::gap() const
	{
		return mGap;
	}

	inline constexpr size_t slice::segment() const
	{
		return mSegment;
	}

	inline constexpr bool slice::is_contiguous() const
	{
		return gap() == 0;
	}

	inline constexpr size_t slice::size() const
	{
		if (mEnd == UnboundEnd)
			return UnboundSize;
		else
		{
			const auto unslicedLength = mEnd - mBegin;

			const auto stridedSegments = unslicedLength / stride();
			const auto excess = std::min(unslicedLength % stride(), segment());

			return (stridedSegments * mSegment) + excess;
		}
	}
}

#include "sliced_span.hpp"

namespace cupl
{
	template <typename T>
	inline constexpr sliced_span<T> slice::operator()(std::span<T> span) const
	{
		return sliced_span<T>(span, *this);
	}

	template <typename T, size_t N>
	constexpr sliced_span<T> slice::operator()(std::array<T, N>& span) const
	{
		return sliced_span<T>(span, *this);
	}

	template <typename T, size_t N>
	constexpr sliced_span<const T> slice::operator()(const std::array<T, N>& span) const
	{
		return sliced_span<T>(span, *this);
	}

	template <typename T>
	constexpr sliced_span<T> slice::operator()(std::vector<T>& span) const
	{
		return sliced_span<T>(span, *this);
	}

	template <typename T>
	constexpr sliced_span<const T> slice::operator()(const std::vector<T>& span) const
	{
		return sliced_span<T>(span, *this);
	}
}

namespace std
{
	template <>
	struct hash<cupl::slice>
	{
		constexpr size_t operator()(const cupl::slice &obj)
		{
			return obj.hash_code();
		}
	};
}

#endif
