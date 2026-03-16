#define BOOST_TEST_MODULE CuplRegistries
#include <boost/test/included/unit_test.hpp>

#include "cupl/registries/registry_map.hpp"
#include "cupl/registries/registry_set.hpp"

using namespace cupl;

BOOST_AUTO_TEST_CASE(check_registry_set)
{
	registry_set<int> regSet;

	BOOST_TEST(regSet.empty());
	BOOST_TEST(regSet.size() == 0);

	registration<int> r0 = regSet.insert(42);
	BOOST_TEST(!regSet.empty());
	BOOST_TEST(regSet.size() == 1);
	BOOST_TEST(regSet.contains(42));
	BOOST_TEST(!regSet.contains(47));
	BOOST_TEST(!regSet.contains(0));

	registration<int> r1 = regSet.insert(47);
	BOOST_TEST(!regSet.empty());
	BOOST_TEST(regSet.size() == 2);
	BOOST_TEST(regSet.contains(42));
	BOOST_TEST(regSet.contains(47));
	BOOST_TEST(!regSet.contains(0));

	{
		registration<int> rScoped = regSet.insert(0);
		BOOST_TEST(!regSet.empty());
		BOOST_TEST(regSet.size() == 3);
		BOOST_TEST(regSet.contains(42));
		BOOST_TEST(regSet.contains(47));
		BOOST_TEST(regSet.contains(0));
	}

	BOOST_TEST(!regSet.empty());
	BOOST_TEST(regSet.size() == 2);
	BOOST_TEST(regSet.contains(42));
	BOOST_TEST(regSet.contains(47));
	BOOST_TEST(!regSet.contains(0));

	r0.reset();
	BOOST_TEST(!regSet.empty());
	BOOST_TEST(regSet.size() == 1);
	BOOST_TEST(!regSet.contains(42));
	BOOST_TEST(regSet.contains(47));
	BOOST_TEST(!regSet.contains(0));

	r1.detach_copy();
	BOOST_TEST(!regSet.empty());
	BOOST_TEST(regSet.size() == 1);
	BOOST_TEST(!regSet.contains(42));
	BOOST_TEST(regSet.contains(47));
	BOOST_TEST(!regSet.contains(0));
}

BOOST_AUTO_TEST_CASE(check_registry_map)
{
	registry_map<int, int> regMap;

	BOOST_TEST(regMap.empty());
	BOOST_TEST(regMap.size() == 0);

	registration<int> r0 = regMap.emplace(42, 4422);
	BOOST_TEST(!regMap.empty());
	BOOST_TEST(regMap.size() == 1);
	BOOST_TEST(regMap.contains(42));
	BOOST_TEST(!regMap.contains(47));
	BOOST_TEST(!regMap.contains(0));

	registration<int> r1 = regMap.emplace(47, 447);
	BOOST_TEST(!regMap.empty());
	BOOST_TEST(regMap.size() == 2);
	BOOST_TEST(regMap.contains(42));
	BOOST_TEST(regMap.contains(47));
	BOOST_TEST(!regMap.contains(0));

	{
		registration<int> rScoped = regMap.emplace(0, 0);
		BOOST_TEST(!regMap.empty());
		BOOST_TEST(regMap.size() == 3);
		BOOST_TEST(regMap.contains(42));
		BOOST_TEST(regMap.contains(47));
		BOOST_TEST(regMap.contains(0));
	}

	BOOST_TEST(!regMap.empty());
	BOOST_TEST(regMap.size() == 2);
	BOOST_TEST(regMap.contains(42));
	BOOST_TEST(regMap.contains(47));
	BOOST_TEST(!regMap.contains(0));

	r0.reset();
	BOOST_TEST(!regMap.empty());
	BOOST_TEST(regMap.size() == 1);
	BOOST_TEST(!regMap.contains(42));
	BOOST_TEST(regMap.contains(47));
	BOOST_TEST(!regMap.contains(0));

	r1.detach_copy();
	BOOST_TEST(!regMap.empty());
	BOOST_TEST(regMap.size() == 1);
	BOOST_TEST(!regMap.contains(42));
	BOOST_TEST(regMap.contains(47));
	BOOST_TEST(!regMap.contains(0));
}
