#include "test.h"
#include <immutable/array.h>
#include <vector>
#include <string>
#include <stdio.h>

using namespace immutable;

TEST(ArrayBasics) {
  // Start with a ref to the empty array
  auto a = Array<int>::empty();

  // Make sure the empty iterator is not null (null is distinct from empty)
  assert(a != nullptr);
  
  // Nothing should be found
  assert(a->findValue(123) == nullptr);
  assert(a->findValue(1) == nullptr);
  
  // Number of sample values to fill the array with
  uint32 count = ArrayImp::BRANCHES * ArrayImp::BRANCHES;
  
  // push(), findValue() and get()
  for (uint32 i = 0; i < count; ++i) {
    int value = i+1;
    a = a->push(value);
    assert(a != nullptr);
    assert(a->size() == i+1);
    assert(a->findValue(i) != nullptr);
    assert(a->findValue(i)->value == value);
    assert(a->get(i) == value);
  }
  assert(a->findValue(count) == nullptr);

  // set() and get()
  for (uint32 i = 0; i < count; ++i) {
    int value = i+1;
    a = a->set(i, value);
    assert(a->get(i) == value);
  }
  
  // attempting to set a value that is out of bounds yields null
  assert(a->set(count, 123) == nullptr);
  
  // first and last value accessors
  assert(a->first() == 1);
  assert(a->last() == int(count));
}


// Create an array with count values
static ref<Array<int>> mkvals(uint32 size) {
  auto a = Array<int>::empty();
  for (int i = 0; i < int(size); ++i) {
    int value = i+1;
    a = a->push(value);
  }
  return a;
}


TEST(ArrayCreate) {
  // create with initializer_list
  auto a = Array<int>::create({1, 2, 3});
  assert(a->size() == 3);
  assert(a->get(0) == 1);
  assert(a->get(1) == 2);
  assert(a->get(2) == 3);

  // create with input_iterator as reference
  auto c = std::vector<int>({1, 2, 3});
  auto I = c.begin();
  a = Array<int>::create(I, c.end());
  assert(a->size() == 3);
  assert(a->get(0) == 1);
  assert(a->get(1) == 2);
  assert(a->get(2) == 3);

  // create with input_iterator as rvalue
  c = std::vector<int>({1, 2, 3});
  a = Array<int>::create(c.begin(), c.end());
  assert(a->size() == 3);
  assert(a->get(0) == 1);
  assert(a->get(1) == 2);
  assert(a->get(2) == 3);
  
  // create with iterable as rvalue
  // In this case, create() will move each value rather than copying them
  a = Array<int>::create(std::vector<int>({1, 2, 3}));
  assert(a->size() == 3);
  assert(a->get(0) == 1);
  assert(a->get(1) == 2);
  assert(a->get(2) == 3);
  
  // create with iterable as constant reference
  const std::vector<int> constvec({1, 2, 3});
  a = Array<int>::create(constvec);
  assert(a->size() == 3);
  assert(a->get(0) == 1);
  assert(a->get(1) == 2);
  assert(a->get(2) == 3);
  
  // create with other array via Array::Iterator
  auto b = Array<int>::create({1, 2, 3});
  a = Array<int>::create(b->begin(), b->end());
  assert(a->size() == 3);
  assert(a->get(0) == 1);
  assert(a->get(1) == 2);
  assert(a->get(2) == 3);

  // Create with strings (just to be sure since we otherwise deal with int)
  auto d = Array<std::string>::create({
    std::string("foo"),
    std::string("bar"),
    std::string("baz"),
  });
  assert(d->size() == 3);
  assert(d->get(0) == "foo");
  assert(d->get(1) == "bar");
  assert(d->get(2) == "baz");
}


TEST(ArrayIterator) {
  auto a = mkvals(ArrayImp::BRANCHES * ArrayImp::BRANCHES);

  // iterator
  {
    auto I = a->begin();
    auto E = a->end();
    int i = 0;
    for (; I != E; ++I) {
      assert(*I == ++i);
    }
  }

  // find-value iterator
  {
    int i = ArrayImp::BRANCHES;
    auto I = a->find(i);
    auto E = a->end();
    for (; I != E; ++I) {
      assert(*I == ++i);
    }
  }
  
  // ranged iterator
  {
    uint32 len = 3;
    for (uint32 phase = 1; phase < (a->size() - len); phase += len) {
      uint32 i = phase;
      auto I = a->begin(i, i+len);
      auto E = a->end();
      assert(*I == int(i+1)); ++i; ++I;
      assert(*I == int(i+1)); ++i; ++I;
      assert(*I == int(i+1)); ++i; ++I;
      assert(I == E);
    }
  }
  
  // iterator distance
  {
    auto I = a->begin();
    auto E = a->end();
    assert(E.distanceTo(I) == a->size());
    assert(I.distanceTo(E) == a->size());
    
    auto I2 = a->begin();
    uint32 n = 3, i = 0;
    for (; i < n; ++i) {
      ++I2;
    }
    assert(I2.distanceTo(I) == n);
    assert(I.distanceTo(I2) == n);
    assert(I2.distanceTo(E) == a->size() - n);
  }
  
  // range-based for loop
  { int i = 0;
    for (auto& v : *a) {
      assert(v == ++i);
    }
  }

  // non-standard iteration using valid()
  {
    int i = 0;
    for (auto I = a->begin(); I.valid(); ++I) {
      assert(*I == ++i);
    }
  }

}

TEST(ArrayPop) {
  auto a = mkvals(ArrayImp::BRANCHES * ArrayImp::BRANCHES);
  
  // pop
  {
    auto a2 = a;
    auto popIndex = a->size();
    while (popIndex--) {
      auto oldSize = a2->size();
      a2 = a2->pop();
      assert(a2 != nullptr);
      assert(a2->size() == oldSize - 1);
      
      // verify values
      for (uint32 i = 0; i < oldSize; ++i) {
        if (i == oldSize-1) {
          // value that was removed should be gone
          assert(a2->findValue(i) == nullptr);
        } else {
          assert(a2->get(i) == int(i+1));
        }
      }
    }
  }
}


TEST(ArrayTransient) {
  uint32 count = ArrayImp::BRANCHES * ArrayImp::BRANCHES;
  auto a = mkvals(count);

  // transient
  auto t = a->asTransient();
  assert(t->size() == a->size());
  
  // findValue()
  assert(t->findValue(a->size()) == nullptr);
  for (uint32 i = 0; i < count; ++i) {
    auto& v = t->findValue(i);
    assert(v != nullptr);
    assert(v->value == int(i+1));
  }
  
  // get()
  for (uint32 i = 0; i < count; ++i) {
    assert(t->get(i) == int(i+1));
  }
  
  // seal with persistent(), returning an Array referencing the data created by t
  a = t->makePersistent();
  assert(a != nullptr);
  assert(a->size() == count);
  assert(t->makePersistent() == nullptr); // already sealed
  assert(t->push(123) == nullptr);    // sealed
  
  // create transient array using push(), then verify with findValue() and get()
  t = Array<int>::empty()->asTransient();
  for (uint32 i = 0; i < count; ++i) {
    int value = int(i+1);
    assert(t->push(value) == t); // should return itself
    assert(t->size() == i+1);
    assert(t->findValue(i+1) == nullptr);
    assert(t->findValue(i) != nullptr);
    assert(t->findValue(i)->value == value);
    assert(t->get(i) == value);
  }
  
  // set() and get()
  for (uint32 i = 0; i < count; ++i) {
    int value = int(i+1)*10;
    assert(t->set(i, value) == t);
    assert(t->get(i) == value);
  }
  
  // first() and last()
  assert(t->first() == 10);
  assert(t->last() == int(count*10));
  
  // pop
  {
    auto popIndex = count;
    while (popIndex--) {
      auto oldSize = t->size();
      assert(t->pop() == t);
      assert(t->size() == oldSize - 1);
      
      // verify values
      for (uint32 i = 0; i < oldSize; ++i) {
        if (i == oldSize-1) {
          // value that was removed should be gone
          assert(t->findValue(i) == nullptr);
        } else {
          assert(t->get(i) == int(i+1)*10);
        }
      }
    }
  }

  // modify
  a = Array<int>::create({1, 2, 3});
  a = a->modify([](auto a) {
    a->set(0, 10);
    a->set(1, 20)->set(2, 30);
  });
  assert(a->get(0) == 10);
  assert(a->get(1) == 20);
  assert(a->get(2) == 30);
}


TEST(ArrayCons) {
  auto a = Array<int>::create({1, 2, 3});
  a = a->cons(0);
  assert(a->size() == 4);
  assert(a->get(0) == 0);
  assert(a->get(1) == 1);
  assert(a->get(2) == 2);
  assert(a->get(3) == 3);
}


TEST(ArrayConcat) {
  auto a = Array<int>::create({1,2,3});

  // concat (uses iterator "under the hood")
  // [1 2 3] concat [4 5 6] => [1 2 3 4 5 6]
  a = a->concat(Array<int>::create({4,5,6}));
  assert(a->size() == 6);
  for (uint32 i = 0; i < 6; ++i) {
    assert(a->get(i) == int(i+1));
  }
  
  // effectively concat using push with ranged iterator
  // [1 2 3] push([4 5 6 7] begin(1,3)) => [1 2 3 5 6]
  a = Array<int>::create({1,2,3});
  auto b = Array<int>::create({4,5,6,7});
  a = a->push(b->begin(1,3), b->end());
  assert(a->size() == 5);
  assert(a->get(0) == 1);
  assert(a->get(1) == 2);
  assert(a->get(2) == 3);
  assert(a->get(3) == 5);
  assert(a->get(4) == 6);
}


TEST(ArraySlice) {
  auto a = Array<int>::create({1,2,3,4,5});
  
  // out-of bounds
  auto b = a->slice(0,9); // end is beyond size
  assert(b == nullptr);
  b = a->slice(2,1); // end is less than start
  assert(b == nullptr);
  b = a->slice(9,9); // start is beyond size
  assert(b == nullptr);
  
  // [1 2 3 4 5] slice(2,2) => []
  b = a->slice(2,2);
  assert(b);
  assert(b->size() == 0);
  assert((ArrayImp::A*)b.ptr() == &ArrayImp::EMPTY);
  
  // [1 2 3 4 5] slice(0,5) => [1 2 3 4 5]
  b = a->slice(0,5);
  assert(b.ptr() == a.ptr()); // same object
  assert(b->size() == 5);
  
  // [1 2 3 4 5] slice(2,END) => [3 4 5]
  b = a->slice(2);
  assert(b);
  assert(b->size() == 3);
  assert(b->get(0) == 3);
  assert(b->get(1) == 4);
  assert(b->get(2) == 5);
  
  // [1 2 3 4 5] slice(0,3) => [1 2 3]
  b = a->slice(0, 3);
  assert(b->size() == 3);
  assert(b->get(0) == 1);
  assert(b->get(1) == 2);
  assert(b->get(2) == 3);
  
  // [1 2 3 4 5] slice(1,4) => [2 3 4]
  b = a->slice(1, 4);
  assert(b->size() == 3);
  assert(b->get(0) == 2);
  assert(b->get(1) == 3);
  assert(b->get(2) == 4);
  
  // [1 2 3 4 5] slice(1,2) => [2]
  b = a->slice(1, 2);
  assert(b->size() == 1);
  assert(b->get(0) == 2);
  
  // iterator over slice
  { b = a->slice(0,3); // [1 2 3] 4 5
    assert(b->size() == 3);
    auto I = b->begin();
    auto E = b->end();
    assert(I != E);
    assert(*I == 1); ++I;
    assert(*I == 2); ++I;
    assert(*I == 3); ++I;
    assert(I == E);
  }
  { b = a->slice(2,4); // 1 2 [3 4] 5
    assert(b->size() == 2);
    auto I = b->begin();
    auto E = b->end();
    assert(I != E);
    assert(*I == 3); ++I;
    assert(*I == 4); ++I;
    assert(I == E);
  }
  { b = a->slice(3); // 1 2 3 [4 5]
    assert(b->size() == 2);
    auto I = b->begin();
    auto E = b->end();
    assert(I != E);
    assert(*I == 4); ++I;
    assert(*I == 5); ++I;
    assert(I == E);
  }
  
  // ———————————————————————————————————
  // slice of a slice
  
  // [1 2 3 4 5] slice(2,END) => [3 4 5] slice(1,END) => [4 5]
  b = a->slice(2);
  assert(b);
  assert(b->size() == 3);
  b = b->slice(1);
  assert(b);
  assert(b->size() == 2);
  assert(b->get(0) == 4);
  assert(b->get(1) == 5);
  
  // [1 2 3 4 5] slice(2,END) => [3 4 5] slice(1,2) => [4]
  b = a->slice(2)->slice(1,2);
  assert(b->size() == 1);
  assert(b->get(0) == 4);
  
  // [1 2 3 4 5] slice(1,4) => [2 3 4] slice(1,END) => [3 4]
  b = a->slice(1,4)->slice(1);
  assert(b->size() == 2);
  assert(b->get(0) == 3);
  assert(b->get(1) == 4);
  
  // [1 2 3 4 5] slice(1,4) => [2 3 4] slice(1,2) => [3]
  b = a->slice(1,4)->slice(1,2);
  assert(b->size() == 1);
  assert(b->get(0) == 3);
  
  // [1 2 3 4 5] slice(2,END) => [3 4 5] slice(0,2) => [3 4]
  b = a->slice(2)->slice(0,2);
  assert(b->size() == 2);
  assert(b->get(0) == 3);
  assert(b->get(1) == 4);
  
  // [1 2 3 4 5] slice(1,4) => [2 3 4] slice(0,2) => [2 3]
  b = a->slice(1,4)->slice(0,2);
  assert(b->size() == 2);
  assert(b->get(0) == 2);
  assert(b->get(1) == 3);
}


TEST(ArrayWithout) {
  auto a = Array<int>::create({1,2,3,4,5});
  
  // out-of bounds
  auto b = a->without(0,9); // end is beyond size
  assert(b == nullptr);
  b = a->without(2,1); // end is less than start
  assert(b == nullptr);
  b = a->without(9,9); // start is beyond size
  assert(b == nullptr);
  
  // [1 2 3 4 5] without(3,3) => [1 2 3 4 5] (zero range)
  b = a->without(3,3);
  assert(b);
  assert(b.ptr() == a.ptr()); // same array
  
  // [1 2 3 4 5] without(0,5) => []
  b = a->without(0,5);
  assert(b);
  assert(b->size() == 0);
  assert((ArrayImp::A*)b.ptr() == &ArrayImp::EMPTY);
  
  // [1 2 3 4 5] without(0,3) => [4 5]
  b = a->without(0,3);
  assert(b);
  assert(b->size() == 2);
  assert(b->get(0) == 4);
  assert(b->get(1) == 5);
  
  // [1 2 3 4 5] without(2,5) => [1 2]
  b = a->without(2,5);
  assert(b);
  assert(b->size() == 2);
  assert(b->get(0) == 1);
  assert(b->get(1) == 2);
  
  // [1 2 3 4 5] without(2,4) => [1 2 5]
  b = a->without(2,4);
  assert(b);
  assert(b->size() == 3);
  assert(b->get(0) == 1);
  assert(b->get(1) == 2);
  assert(b->get(2) == 5);
}


TEST(ArraySplice) {
  // splice in another Array
  auto a = Array<int>::create({1,2,3,4,5});
  auto c = Array<int>::create({6,7});
  
  // out-of bounds
  auto b = a->splice(0,9, c); // end is beyond size
  assert(b == nullptr);
  b = a->splice(2,1, c); // end is less than start
  assert(b == nullptr);
  b = a->splice(9,9, c); // start is beyond size
  assert(b == nullptr);
  
  // [1 2 3 4 5] splice(0,5, [6 7]) => [6 7]
  b = a->splice(0,5, c);
  assert(b->size() == 2);
  assert(b->get(0) == 6);
  assert(b->get(1) == 7);
  
  // [1 2 3 4 5] splice(0,3, [6 7]) => [4 5 6 7]
  b = a->splice(0,3, c);
  assert(b->size() == 4);
  assert(b->get(0) == 4);
  assert(b->get(1) == 5);
  assert(b->get(2) == 6);
  assert(b->get(3) == 7);
  
  // [1 2 3 4 5] splice(5,5, [6 7]) => [1 2 3 4 5 6 7]
  b = a->splice(5,5, c);
  assert(b->size() == 7);
  assert(b->get(0) == 1);
  assert(b->get(1) == 2);
  // ...
  assert(b->get(5) == 6);
  assert(b->get(6) == 7);
  
  // [1 2 3 4 5] splice(2,5, [6 7]) => [1 2 6 7]
  b = a->splice(2,5, c);
  assert(b->size() == 4);
  assert(b->get(0) == 1);
  assert(b->get(1) == 2);
  assert(b->get(2) == 6);
  assert(b->get(3) == 7);
  
  // [1 2 3 4 5] splice(2,4, [6 7]) => [1 2 6 7 5]
  b = a->splice(2,4, c);
  assert(b->size() == 5);
  assert(b->get(0) == 1);
  assert(b->get(1) == 2);
  assert(b->get(2) == 6);
  assert(b->get(3) == 7);
  assert(b->get(4) == 5);
}


TEST(ArraySpliceIterable) {
  // splice in "some iterable" that will use splicefn (lambda iterator)
  auto a = Array<int>::create({1,2,3,4,5});
  std::vector<int> c({6,7});
  
  // out-of bounds
  auto b = a->splice(0,9, c.begin(), c.end()); // end is beyond size
  assert(b == nullptr);
  b = a->splice(2,1, c.begin(), c.end()); // end is less than start
  assert(b == nullptr);
  b = a->splice(9,9, c.begin(), c.end()); // start is beyond size
  assert(b == nullptr);
  
  // BUG a has been deallocated here
  assert(a->size()); // accesses deallocated memory
  
  // [1 2 3 4 5] splice(0,5, [6 7]) => [6 7]
  b = a->splice(0,5, c.begin(), c.end());
  assert(b->size() == 2);
  assert(b->get(0) == 6);
  assert(b->get(1) == 7);
  
  // [1 2 3 4 5] splice(0,3, [6 7]) => [4 5 6 7]
  b = a->splice(0,3, c.begin(), c.end());
  assert(b->size() == 4);
  assert(b->get(0) == 4);
  assert(b->get(1) == 5);
  assert(b->get(2) == 6);
  assert(b->get(3) == 7);
  
  // [1 2 3 4 5] splice(5,5, [6 7]) => [1 2 3 4 5 6 7]
  b = a->splice(5,5, c.begin(), c.end());
  assert(b->size() == 7);
  assert(b->get(0) == 1);
  assert(b->get(1) == 2);
  // ...
  assert(b->get(5) == 6);
  assert(b->get(6) == 7);
  
  // [1 2 3 4 5] splice(2,5, [6 7]) => [1 2 6 7]
  b = a->splice(2,5, c.begin(), c.end());
  assert(b->size() == 4);
  assert(b->get(0) == 1);
  assert(b->get(1) == 2);
  assert(b->get(2) == 6);
  assert(b->get(3) == 7);
  
  // [1 2 3 4 5] splice(2,4, [6 7]) => [1 2 6 7 5]
  b = a->splice(2,4, c.begin(), c.end());
  assert(b->size() == 5);
  assert(b->get(0) == 1);
  assert(b->get(1) == 2);
  assert(b->get(2) == 6);
  assert(b->get(3) == 7);
  assert(b->get(4) == 5);
}


TEST(ArrayCompare) {
  auto a = Array<int>::create({1,2,3,4,5});
  
  // same array, or same data
  assert(a->compare(a) == 0);
  assert(a == a);
  assert(a->compare(a->slice(0)) == 0);
  assert(a == a->slice(0));
  
  // references same root but different range
  assert(a->compare(a->slice(1)) == 1);
  assert(a->slice(1)->compare(a) == -1);
  
  // same values by pointer equality (b refers to same Value objects)
  auto b = a->asTransient()->makePersistent();
  assert(a->compare(b) == 0);
  
  // compare values by value (b refers to different Value objects)
  b = Array<int>::create({1,2,3,4,5});
  assert(a->compare(b) == 0);
  
  b = Array<int>::create({1,2,3,4});
  assert(a->compare(b) == 1); // b is less because of its size
  
  b = Array<int>::create({1,2,3,4,5,6});
  assert(a->compare(b) == -1); // b is greater because of its size
  
  b = Array<int>::create({1,2,3,4,4});
  assert(a->compare(b) == 1); // b is less because of its values
  
  b = Array<int>::create({1,2,3,4,6});
  assert(a->compare(b) == -1); // b is greater because of its values
}
