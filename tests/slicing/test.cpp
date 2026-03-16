#define BOOST_TEST_MODULE CuplSlicing
#include <boost/test/included/unit_test.hpp>

#include <vector>

#include "cupl/slicing/slice.hpp"
#include "cupl/slicing/sliced_span.hpp"

using namespace std;
using namespace cupl;

BOOST_AUTO_TEST_CASE(check_slice_caluclations)
{
	slice fullSlice(0);
	slice forthsSlice(3, -1, 4, 1);
	slice exceptForthsSlice(0, -1, 4, 3);
	slice boundedSlice(0, 100, 20, 10);
	slice contiguousSlice(0, 100);

	// is_contiguous
	BOOST_TEST(fullSlice.is_contiguous());
	BOOST_TEST(!forthsSlice.is_contiguous());
	BOOST_TEST(!exceptForthsSlice.is_contiguous());
	BOOST_TEST(!boundedSlice.is_contiguous());
	BOOST_TEST(contiguousSlice.is_contiguous());

	// size
	BOOST_TEST(fullSlice.size() == std::numeric_limits<size_t>::max());
	BOOST_TEST(forthsSlice.size() == std::numeric_limits<size_t>::max());
	BOOST_TEST(exceptForthsSlice.size() == std::numeric_limits<size_t>::max());
	BOOST_TEST(boundedSlice.size() == 50);
	BOOST_TEST(contiguousSlice.size() == 100);

	// is_divisible_by
	BOOST_TEST(fullSlice.is_divisible_by(1));
	BOOST_TEST(fullSlice.is_divisible_by(10));
	BOOST_TEST(forthsSlice.is_divisible_by(1));
	BOOST_TEST(!forthsSlice.is_divisible_by(4));
	BOOST_TEST(exceptForthsSlice.is_divisible_by(1));
	BOOST_TEST(!exceptForthsSlice.is_divisible_by(4));
	BOOST_TEST(boundedSlice.is_divisible_by(1));
	BOOST_TEST(boundedSlice.is_divisible_by(5));
	BOOST_TEST(boundedSlice.is_divisible_by(10));
	BOOST_TEST(!boundedSlice.is_divisible_by(7));
	BOOST_TEST(contiguousSlice.is_divisible_by(1));
	BOOST_TEST(contiguousSlice.is_divisible_by(10));
	BOOST_TEST(contiguousSlice.is_divisible_by(100));
	BOOST_TEST(!contiguousSlice.is_divisible_by(101));

	// divide
	BOOST_TEST(fullSlice / 1 == fullSlice);
	BOOST_TEST(fullSlice / 10 == slice(0));
	BOOST_TEST(forthsSlice / 1 == forthsSlice);
	BOOST_TEST(exceptForthsSlice / 1 == exceptForthsSlice);
	BOOST_TEST(boundedSlice / 1 == boundedSlice);
	BOOST_TEST(boundedSlice / 10 == slice(0, 10, 2, 1));
	BOOST_TEST(contiguousSlice / 1 == contiguousSlice);
	BOOST_TEST(contiguousSlice / 100 == slice(0, 1));
}

BOOST_AUTO_TEST_CASE(check_sliced_span)
{
	vector<int> sequence(105);

	for (size_t i = 0; i < sequence.size(); ++i)
		sequence[i] = i;

	slice forthsSlice(3, -1, 4, 1);
	slice exceptForthsSlice(0, -1, 4, 3);
	slice boundedSlice(0, 100, 20, 10);
	slice contiguousSlice(0, 100);

	{
		slice fullSlice(0);
		auto sliced = fullSlice(sequence);
		BOOST_REQUIRE(sliced.size() == 105);
		BOOST_TEST(sliced.is_contiguous());
		BOOST_REQUIRE(&*sliced.end() == (&*sliced.begin() + 105));
		BOOST_REQUIRE(sliced.end() == (sliced.begin() + 105));

		size_t i = 0;
		for (auto iter = sliced.begin(); iter != sliced.end(); ++iter)
		{
			BOOST_REQUIRE_MESSAGE(iter == (sliced.begin() + i), "iter != (sliced.begin() + " << i << ")");
			BOOST_TEST(*iter == i);
			++i;
		}

		/*
		size_t i = 0;
		for (const auto& e : sliced)
		{
			BOOST_REQUIRE(e == i);
			++i;
		}
		*/
	}

	{
		slice forthsSlice(3, -1, 4, 1);
		auto sliced = forthsSlice(sequence);
		BOOST_TEST(sliced.size() == (105/4));
		BOOST_TEST(!sliced.is_contiguous());

		size_t i = 0;
		for (const auto& e : sliced)
		{
			BOOST_TEST(e == ((i+1)*4 - 1));
			++i;
		}
	}

	{
		slice exceptForthsSlice(0, -1, 4, 3);
		auto sliced = exceptForthsSlice(sequence);
		BOOST_TEST(sliced.size() == 105-(105/4));
		BOOST_TEST(!sliced.is_contiguous());

		size_t i = 0;
		for (const auto& e : sliced)
		{
			BOOST_TEST(e == (4*(i/3) + (i % 3)));
			++i;
		}
	}

	{
		slice boundedSlice(0, 100, 20, 10);
		auto sliced = boundedSlice(sequence);
		BOOST_TEST(sliced.size() == 50);
		BOOST_TEST(!sliced.is_contiguous());

		size_t i = 0;
		for (const auto& e : sliced)
		{
			BOOST_TEST(e == (20*(i/10) + (i % 10)));
			++i;
		}
	}

	{
		slice contiguousSlice(0, 100);
		auto sliced = contiguousSlice(sequence);
		BOOST_TEST(sliced.size() == 100);
		BOOST_TEST(sliced.is_contiguous());

		size_t i = 0;
		for (const auto& e : sliced)
		{
			BOOST_TEST(e == i);
			++i;
		}
	}
}
