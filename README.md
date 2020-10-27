# Arraymap

Arraymap is c++20 associative container with API similar to std::map and guaranteed O(1) lookup complexity UNDER ALL CIRCUMSTANCES.

In addition, adding elements is guaranteed not to invalidate iterators, removing element only invalidates iterator if it points to removed element.

Benchmarks have shown roughly ~50% faster lookup times and ~30% faster element adding times compared to std::unordered_map when key type is 4 bytes long (for example int).

And unlike std::unordered_map, arraymap is ordered!!!

All this sounds too good to be true? Well, yes, there are some major drawbacks to this map:

First, the worst-case memory performance is AWFUL (best-case memory performance is similar to unordered_map). Worst case happens when all the data if fully random, best case if all data is within a range (like int between 0 and 1e6).

Second, key types are limited to being trivially copyable, which means std::strings etc. cannot be used as key.

Third, lookup times increase linearly as sizeof(key_type) increaes, and so does worst-case memory performance (best-case doesn't change too much),


In summary, this is a "heavy-weight" container and becomes practical when data sets become large (few million) and key values are packed within a range (or a set of ranges),

## Usage

Simplest use-case is almost equivalent to std::map, just declare it:
```c++
arraymap::arraymap<int, int> theMap;
```
and start using it.

only difference for a simple use case is range-based for loops need to be value based rather than reference-based:
```c++
for(auto [key, val]: theMap)
```
rather than
```c++
for(auto & [key, val]: theMap)
```



To change ordering, an ordering element is used instead of compare element

The map is internally ordered line an unsigned integer. To change how you custom type is ordered, you must make a class with two static methods:
```c++
T apply(const T &value)
T restore(const T &value)
```
where apply() should perform some inexpensive operations (like flipping bits etc) to change your ordering to your desired,
and restore() should restore the original state of your key type.


allocator allocates value_type only, since the key is not actually stored.


## How it works

Inside, this container uses an unbalanced tree of arrays (hence the name arraymap), 16-member each.
Lookup is done simply using array offsets taken from within the key_type

## Licence
[MIT]
