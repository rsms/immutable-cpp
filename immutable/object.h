#pragma once
#include "base.h"

#include <stdio.h>
#define DBG(fmt, ...) printf("%s: " fmt " (:%d)\n", __FUNCTION__, ##__VA_ARGS__ , __LINE__)
//#define DBG(fmt, ...)


namespace immutable {
  
  using TypeTag = char;
  
  #ifndef NDEBUG
    #define IMMUTABLE_WITH_TYPE_TAGS
  #endif

  #ifdef IMMUTABLE_WITH_TYPE_TAGS
    #define ImmutableTypeTagHead             TypeTag typeTag;
    #define ImmutableAssertTypeTag(ptr, tag) assert((ptr)->typeTag == tag)
  #else
    #define ImmutableTypeTagHead
    #define ImmutableAssertTypeTag(ptr, tag) ((void)0)
  #endif

  struct Object : RefCounted {
    #ifdef IMMUTABLE_WITH_TYPE_TAGS
    ImmutableTypeTagHead
    Object(TypeTag t) : typeTag(t) {}
    #else
    Object(TypeTag) {}
    #endif
  };


  // Reference-counted container for a value of any type
  template <typename T>
  struct Value : Object {
    T value; // the actual value is stored here

    // Construct a value in-place
    template <class... Args>
    Value(Args&&... args) : Object(TYPE_TAG), value(fwd<Args>(args)...) {}

    // allow implicit cast to T
    operator T&() { return value; }
    operator const T&() const { return value; }

    // copyable and movable
    Value(const Value&) = default;
    Value(Value&&) = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) = default;

    void dealloc() { delete this; }
    static constexpr TypeTag TYPE_TAG = 'V';
    IMMUTABLE_REFCOUNTED_IMPL(Value)
  };
  
  
  struct ArrayImp;


  // Immutable persistent array
  template <typename T>
  struct Array : RefCounted {
    // the empty array
    static ref<Array> empty();

    // Number of values in this array
    uint32 size() const { return _size; }
    
    // Append value to the end.
    ref<Array> push(Value<T>* v) const;

    // Construct a value T in-place and append to the end
    template <class... Args>
    ref<Array> push(Args&&... args) const {
      return push(new Value<T>(fwd<Args>(args)...));
    }

    // Find value at index. Returns nullptr if index is out-of bounds.
    const ref<Value<T>> find(uint32 i) const;
    

    // copyable and movable
    Array(const Array&) = default;
    Array(Array&&) = default;
    Array& operator=(const Array&) = default;
    Array& operator=(Array&&) = default;

  protected:
    friend struct ArrayImp;

    uint32      _size;  //
    uint32      _shift; // BITS times (the depth of this trie minus one)
    ref<Object> _root;  //
    ref<Object> _tail;  //

    Array() = delete; // use Array::empty() instead
    Array(uint32 size, uint32 shift, ref<Object> root, ref<Object> tail);
    Array(ref<Object> root, ref<Object> tail);
    void dealloc() { delete this; }

    IMMUTABLE_REFCOUNTED_IMPL(Array)
  };
  
  
  struct ArrayImp {
    static constexpr uint32  BITS     = 2;            // 5, 4, 3, 2 ...
    static constexpr uint32  BRANCHES = 1 << BITS;    // 2^5=32, 2^4=16, 2^3=8, 2^2=4 ...
    static constexpr uint32  MASK     = BRANCHES - 1; // 31 (or 0x1f), 15, 7, 3 ...

    struct N;
    using  A = Array<void*>;

    static A EMPTY;

    static Object* objAt(A* a, uint32 i);
    static A* push(A* a, Object* obj);

    struct detail;
  };
  
  
  template <typename T>
  ref<Array<T>> Array<T>::empty() {
    return (Array<T>*)&ArrayImp::EMPTY;
  }
  
  
  // Constructor only used for initialization of ArrayImp::EMPTY
  template <typename T>
  inline Array<T>::Array(ref<Object> root, ref<Object> tail)
    : Array(0, ArrayImp::BITS, root, tail)
  {
    retain();
  }

  template <typename T>
  inline Array<T>::Array(uint32 size, uint32 shift, ref<Object> root, ref<Object> tail)
    : _size(size), _shift(shift), _root(root), _tail(tail)
  {}
  
  
  template <typename T>
  ref<Array<T>> Array<T>::push(Value<T>* v) const {
    assert(v != nullptr);
    return (Array<T>*)ArrayImp::push(
      (ArrayImp::A*)this,
      static_cast<Object*>(v)
    );
  }
  

  template <typename T>
  inline const ref<Value<T>> Array<T>::find(uint32 i) const {
    Object* obj = ArrayImp::objAt((ArrayImp::A*)this, i);
    if (obj) {
      ImmutableAssertTypeTag(obj, Value<T>::TYPE_TAG);
      return static_cast<Value<T>*>(obj);
    }
    return nullptr;
  }


} // namespace
