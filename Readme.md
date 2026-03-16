# Cupl-Cpp - a set of Complementary Utility Programming Libraries for C++

Cupl-Cpp is a set of libraries designed to complement the C++ Standard Libraries.

## `cupl/registries`

A header-only library that provideds unordered collections `registry_set` and `registry_map`, where membership is only preserved by holding onto a handle called a `registration`.

### Dependancies

* [Boost C++ Libraries](https://www.boost.org/)
	* [Unordered](https://www.boost.org/library/latest/unordered/)
* [TBB](https://uxlfoundation.github.io/oneTBB/)

## `cupl/slicing`

A header-only library that provides types for sliced access to spans similar to Python's slicing.

### Dependancies

* [Boost C++ Libraries](https://www.boost.org/)
	* [Container Hash](https://www.boost.org/library/latest/container_hash/)
