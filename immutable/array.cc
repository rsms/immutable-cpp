#include "array.h"
#include <thread>

// Uncomment to enable pedantic runtime checks for debug builds
//#define DCHECK(expr) assert(expr)

#ifndef DCHECK
  #define DCHECK(expr) ((void)0)
#endif

// Development debug logging
#define DBG(fmt, ...) printf("%s: " fmt " (:%d)\n", __FUNCTION__, ##__VA_ARGS__ , __LINE__)
#ifdef DBG
  #include <stdio.h>
#else
  #define DBG(fmt, ...)
#endif

namespace immutable {
  using A = ArrayImp::A;
  using N = ArrayImp::N;

//  static auto constexpr BITS     = ArrayImp::BITS;
  static auto constexpr BRANCHES = ArrayImp::BRANCHES;
//  static auto constexpr MASK     = ArrayImp::MASK;
  
  using EditID = std::thread::id;
  static EditID NO_EDIT;
  
  struct empty_initializer {};
  

  struct ArrayImp::N : Object {
    static constexpr TypeTag TYPE_TAG = 'N';
    IMMUTABLE_REFCOUNTED_IMPL(N)
    public:
    EditID      edit;
    uint32      length;
    ref<Object> _v[0];
    
    N(EditID ed, uint32 len) : Object(TYPE_TAG), edit(ed), length(len) {}
    
    // Constructor used by EMPTY_ROOT and EMPTY_TAIL
    explicit N(empty_initializer, uint32 len)
      : Object(TYPE_TAG), _refcount(1), edit(NO_EDIT), length(len)
    {
      uint32 i = 0;
      while (i < length) {
        construct(&_v[i++]);
      }
    }

    // Note: returns a node with zero refcount, so it should be put in a ref immediately.
    static N* create(uint32 len, EditID edit) {
      DCHECK(len <= BRANCHES);
      N* n = (N*)calloc(sizeof(N) + (sizeof(ref<Object>) * BRANCHES), 1);
      construct(n, edit, len);
      // the following is needed if we use malloc instead of calloc:
      //while (len--) {
      //  construct(&n->_v[len]);
      //}
      return n;
    }
    
    void dealloc() {
      uint32 i = 0;
      while (i < length) {
        auto& v = _v[i++];
        if (v) {
          v->release();
        }
      }
      ::free(this);
    }
    
    // Returns a shallow copy of this node
    N* copy(uint32 len, EditID edit_) const {
      auto n = create(len, edit_);

      uint32 i = min(n->length, length);
      while (i--) {
        n->_v[i] = _v[i];
      }
      return n;
    }
    inline N* copy(uint32 len) const {
      return copy(len, edit);
    }
    
    // Optimized version of copy that avoids increment and decrement of reference for
    // object at slot `index`.
    // n2 = n1->copyAssign(1, obj) is functionally equivalent to:
    // 1.  auto n2 = n1->copy(n1->length);
    // 2.  n2->slot(1) = obj;
    // but it avoids retaining and releasing of object copied at line 1.
    N* copyAssign(uint32 index, Object* obj, EditID edit_) const {
      auto n = create(length, edit_);

      uint32 i = length;
      while (i--) {
        n->_v[i] = (i == index) ? obj : _v[i].ptr();
      }

      return n;
    }
    inline N* copyAssign(uint32 index, Object* obj) const {
      return copyAssign(index, obj, edit);
    }

    ref<Object>& slot(uint32 i) { return _v[i]; }
    const ref<Object>& slot(uint32 i) const { return _v[i]; }
  };
  
  
  struct ArrayImp::detail {
    
    template <typename A>
    static IMMUTABLE_ALWAYS_INLINE uint32 tailoff(A* a) {
      if (a->_end < BRANCHES) {
        // tail at root
        return 0;
      }
      //return uint32(a->_end - a->_tail->length);
      return uint32(((a->_end - 1) >> BITS) << BITS);
    }
    
    template <typename A>
    static IMMUTABLE_ALWAYS_INLINE N& root(A* a) {
      ImmutableAssertTypeTag(a->_root, N::TYPE_TAG);
      return *static_cast<N*>(a->_root.ptr());
    }
    
    template <typename A>
    static IMMUTABLE_ALWAYS_INLINE N& tail(A* a) {
      ImmutableAssertTypeTag(a->_tail, N::TYPE_TAG);
      return *static_cast<N*>(a->_tail.ptr());
    }
    
    
    template <typename A>
    static IMMUTABLE_ALWAYS_INLINE Object* firstValue(A* a) {
      // assumes a->_end > 0
      N* n = uncheckedSlotsFor(a, 0);
      Object* val = n->slot(0);
      ImmutableAssertTypeTag(val, A::ValueT::TYPE_TAG);
      return val;
    }
    
    
    static IMMUTABLE_ALWAYS_INLINE A* push(A* a, Object* obj) {
      
      // room in tail?
      if (a->_end - tailoff(a) < BRANCHES) {
        auto newTail = tail(a).copy(tail(a).length + 1);
        newTail->slot(tail(a).length) = obj;
        return new A(0, a->_end + 1, a->_shift, a->_root, newTail);
      }
      
      // full tail, push into tree
      N* newRoot;
      
      //auto tailNode = &tail(a);
      auto tailNode = tail(a).copy(tail(a).length, root(a).edit);
      
      auto newShift = a->_shift;
      
      // overflow root?
      if ((a->_end >> BITS) > (1 << a->_shift)) {
        newRoot = N::create(BRANCHES, root(a).edit);
        newRoot->slot(0) = a->_root;
        newRoot->slot(1) = newPath(root(a).edit, a->_shift, tailNode);
        newShift += BITS;
      } else {
        newRoot = pushTail(a, a->_shift, root(a), tailNode);
      }

      auto newTail = N::create(1, tail(a).edit);
      newTail->slot(0) = obj;
      return new A(0, a->_end + 1, newShift, newRoot, newTail);
    }
    

    static N* pushTail(A* a, uint32 level, const N& parentNode, N* tailNode) {
      //if parent is leaf, insert node,
      // else does it map to an existing child? -> nodeToInsert = pushNode one more level
      // else alloc new path
      //return  nodeToInsert placed in copy of parentNode
      uint32 subidx = ((a->_end - 1) >> level) & MASK;
      
      N* nodeToInsert;
      if (level == BITS) {
        nodeToInsert = tailNode;
      } else {
        auto& child = parentNode.slot(subidx);
        if (child) {
          ImmutableAssertTypeTag(child, N::TYPE_TAG);
          nodeToInsert = pushTail(a, level - BITS, *((N*)child.ptr()), tailNode);
        } else {
          nodeToInsert = newPath(root(a).edit, level - BITS, tailNode);
        }
      }

      return parentNode.copyAssign(subidx, nodeToInsert);
    }


    static IMMUTABLE_ALWAYS_INLINE TA* tpush(TA* a, Object* val) {
      if (!isEditable(a)) {
        return nullptr;
      }
      auto i = a->_end;
      
      // room in tail?
      if (i - tailoff(a) < BRANCHES) {
        tail(a).slot(i & MASK) = val;
        ++a->_end;
        return a;
      }

      // full tail, push into tree
      N* newRoot;
      ref<N> tailNode = &tail(a);

      N* newTail = N::create(BRANCHES, root(a).edit);
      newTail->slot(0) = val;
      a->_tail = newTail;

      auto newShift = a->_shift;

      // overflow root?
      if ((a->_end >> BITS) > (1 << a->_shift)) {
        newRoot = N::create(BRANCHES, root(a).edit);
        newRoot->slot(0) = a->_root;
        newRoot->slot(1) = newPath(root(a).edit, a->_shift, tailNode);
        newShift += BITS;
      } else {
        newRoot = tpushTail(a, a->_shift, &root(a), tailNode);
      }
      a->_root = newRoot;
      a->_shift = newShift;
      ++a->_end;
      return a;
    }
    
    
    static N* tpushTail(TA* a, uint32 level, N* parentNode, N* tailNode) {
      //if parent is leaf, insert node,
      // else does it map to an existing child? -> nodeToInsert = pushNode one more level
      // else alloc new path
      //return  nodeToInsert placed in parent
      parentNode = ensureEditable(a, parentNode);
      uint32 subidx = ((a->_end - 1) >> level) & MASK;

      N* nodeToInsert;
      if (level == BITS) {
        nodeToInsert = tailNode;
      } else {
        N* child = static_cast<N*>(parentNode->slot(subidx).ptr());
        if (child) {
          ImmutableAssertTypeTag(child, N::TYPE_TAG);
          nodeToInsert = tpushTail(a, level - BITS, child, tailNode);
        } else {
          nodeToInsert = newPath(root(a).edit, level - BITS, tailNode);
        }
      }

      parentNode->slot(subidx) = nodeToInsert;
      return parentNode;
    }
    
    
    static N* newPath(EditID edit, uint32 level, N* node) {
      if (level == 0) {
        return node;
      }
      N* ret = N::create(BRANCHES, edit);
      ret->slot(0) = newPath(edit, level - BITS, node);
      return ret;
    }
    

    static IMMUTABLE_ALWAYS_INLINE A* set(A* a, uint32 i, Object* obj) {
      // Note: i is assumed to be less than a->_end
      if (i >= tailoff(a)) {
        // Common case: i is inside tail — copy tail and replace tail slot
        return new A(0, a->_end, a->_shift, &root(a), tail(a).copyAssign(i & MASK, obj));
      }
      // build tree
      return new A(0, a->_end, a->_shift, doAssoc(a, a->_shift, root(a), i, obj), &tail(a));
    }
    
    
    static N* doAssoc(A* a, uint32 level, const N& node, uint32 i, Object* obj) {
      if (level == 0) {
        return node.copyAssign(i & MASK, obj);
      }
      
      uint32 subidx = (i >> level) & MASK;
      N* subNode = static_cast<N*>(node.slot(subidx).ptr());
      ImmutableAssertTypeTag(subNode, N::TYPE_TAG);

      return node.copyAssign(subidx, doAssoc(a, level - BITS, *subNode, i, obj));
    }
    
    
    static IMMUTABLE_ALWAYS_INLINE TA* tset(TA* a, uint32 i, Object* val) {
      // Note: i is assumed to be less than a->_end
      if (!isEditable(a)) {
        return nullptr;
      }
      
      if (i >= tailoff(a)) {
        tail(a).slot(i & MASK) = val;
        return a;
      }
      
      a->_root = tdoAssoc(a, a->_shift, &root(a), i, val);
      return a;
    }
    
    
    static N* tdoAssoc(TA* a, uint32 level, N* node, uint32 i, Object* val) {
      node = ensureEditable(a, node);
      if (level == 0) {
        node->slot(i & MASK) = val;
      } else {
        uint32 subidx = (i >> level) & MASK;
        N* subNode = static_cast<N*>(node->slot(subidx).ptr());
        ImmutableAssertTypeTag(subNode, N::TYPE_TAG);
        node->slot(subidx) = tdoAssoc(a, level - BITS, subNode, i, val);
      }
      return node;
    }
    
    
    template <typename A>
    static inline N* checkedSlotsFor(A* a, uint32 i) {
      DCHECK(i < a->_end);

      if (i >= tailoff(a)) {
        return &tail(a);
      }
      
      N* node = &root(a);
      
      for (uint32 level = a->_shift; level > 0; level -= BITS) {
        DCHECK(node);
        auto k = (i >> level) & MASK;
        if (k < node->length) {
          Object* obj = node->slot((i >> level) & MASK);
          if (!obj) {
            return nullptr;
          }
          ImmutableAssertTypeTag(obj, N::TYPE_TAG);
          node = static_cast<N*>(obj);
        } else {
          return nullptr;
        }
      }
      
      return node;
    }
    
    
    template <typename A>
    static inline N* uncheckedSlotsFor(A* a, uint32 i) {
      DCHECK(i < a->_end);
      if (i >= tailoff(a)) {
        return &tail(a);
      }
      
      N* node = &root(a);
      
      for (uint32 level = a->_shift; level > 0; level -= BITS) {
        DCHECK(((i >> level) & MASK) < node->length);
        node = static_cast<N*>(node->slot((i >> level) & MASK).ptr());
        DCHECK(node);
        ImmutableAssertTypeTag(node, N::TYPE_TAG);
      }
      
      return node;
    }
    
    
    // unchecked
    static inline N* editableSlotsFor(TA* a, uint32 i){
      DCHECK(i < a->_end);
      if (i >= tailoff(a)) {
        return &tail(a);
      }
      
      N* node = &root(a);
      
      for (uint32 level = a->_shift; level > 0; level -= BITS) {
        DCHECK(((i >> level) & MASK) < node->length);
        node = ensureEditable(a, static_cast<N*>(node->slot((i >> level) & MASK).ptr()));
        DCHECK(node);
        ImmutableAssertTypeTag(node, N::TYPE_TAG);
      }

      return node;
    }
    
    
    static IMMUTABLE_ALWAYS_INLINE A* pop(A* a) {
      // assumes a is not empty
      if (a->_end == 1) {
        return &ArrayImp::EMPTY;
      }

      if (a->_end - tailoff(a) > 1) {
        // inside tail and there's at least one more item in tail
        N* newTail = tail(a).copy(tail(a).length - 1);
        return new A(0, a->_end - 1, a->_shift, &root(a), newTail);
      }
      
      DCHECK(a->_end >= 2);
      
      N* newTail = uncheckedSlotsFor(a, a->_end - 2);
      N* newRoot = popTail(a, a->_shift, root(a));
      
      int newShift = a->_shift;
      if (!newRoot) {
        newRoot = &EMPTY_NODE;
      }

      if (a->_shift > BITS && !newRoot->slot(1)) {
        newRoot = static_cast<N*>(newRoot->slot(0).ptr());
        ImmutableAssertTypeTag(newRoot, N::TYPE_TAG);
        newShift -= BITS;
      }

      return new A(0, a->_end - 1, newShift, newRoot, newTail);
    }
    
    
    static N* popTail(A* a, uint32 level, const N& node) {
      uint32 subidx = ((a->_end - 2) >> level) & MASK;
      if (level > BITS) {
        DCHECK(node.slot(subidx));
        ImmutableAssertTypeTag(node.slot(subidx), N::TYPE_TAG);
        N* newChild = popTail(a, level - BITS, *static_cast<N*>(node.slot(subidx).ptr()));
        if (newChild != nullptr || subidx != 0) {
          return node.copyAssign(subidx, newChild, root(a).edit);
        }
      } else if (subidx != 0) {
        return node.copyAssign(subidx, nullptr, root(a).edit);
      }
      return nullptr;
    }
    
    
    static IMMUTABLE_ALWAYS_INLINE TA* tpop(TA* a) {
      // assumes a is not empty
      if (a->_end == 1) {
        a->_end = 0;
        return a;
      }
      
      auto i = a->_end - 1;

      // pop in tail?
      if ((i & MASK) > 0) {
        --a->_end;
        return a;
      }
      
      DCHECK(a->_end >= 2);

      ref<N> newTail = editableSlotsFor(a, a->_end - 2);
      N* newRoot = tpopTail(a, a->_shift, &root(a));

      auto newShift = a->_shift;
      if (!newRoot) {
        newRoot = N::create(BRANCHES, root(a).edit);
      }

      if (a->_shift > BITS && !newRoot->slot(1)) {
        newRoot = static_cast<N*>(newRoot->slot(0).ptr());
        ImmutableAssertTypeTag(newRoot, N::TYPE_TAG);
        newRoot = ensureEditable(a, newRoot);
        newShift -= BITS;
      }

      a->_root = newRoot;
      a->_shift = newShift;
      --a->_end;
      a->_tail = newTail;

      return a;
    }
    
    
    static N* tpopTail(TA* a, uint32 level, N* node) {
      node = ensureEditable(a, node);
      uint32 subidx = ((a->_end - 2) >> level) & MASK;
      if (level > BITS) {
        DCHECK(node->slot(subidx));
        ImmutableAssertTypeTag(node->slot(subidx), N::TYPE_TAG);
        N* newChild = tpopTail(a, level - BITS, static_cast<N*>(node->slot(subidx).ptr()));
        if (newChild != nullptr || subidx != 0) {
          node->slot(subidx) = newChild;
          return node;
        }
      } else if (subidx != 0) {
        node->slot(subidx) = nullptr;
        return node;
      }
      return nullptr;
    }
    
    
    static inline bool isEditable(TA* a) {
      return root(a).edit != NO_EDIT;
    }

    static inline N* ensureEditable(TA* a, N* node) {
      if (node->edit != root(a).edit) {
        node = node->copy(node->length, root(a).edit);
      }
      return node;
    }
    
    
    // Assumes start and end are absolute
    static inline bool isOutOfBounds(A* a, uint32 start, uint32 end) {
      DCHECK(start >= a->_start);
      return end <= a->_end && start <= end;
    }
    

    static A* pushAllFn(A* a, const ItFunc& next) {
      A::ValueT* vptr = next();
      if (vptr) {
        auto t = createTransient(a);
        do {
          tpush(t, vptr);
        } while ((vptr = next()));
        a = createPersistent(t);
        t->release();
      }
      return a;
    }
    
    static A* pushAllIt(A* a, A::Iterator& it) {
      if (it != END_ITERATOR) {
        auto t = createTransient(a);
        do {
          tpush(t, it.value());
        } while (++it != END_ITERATOR);
        a = createPersistent(t);
        t->release();
      }
      return a;
    }
    
    
    // copies items in the range [start,end) of src to dst
    // Assumes start and end are absolute.
    static void tpushAll(TA* dst, A* src, uint32 start, uint32 end) {
      A::Iterator I(src, start, end);
      for (; I != END_ITERATOR; ++I) {
        DCHECK(I.value());
        tpush(dst, I.value());
      }
    }

  
  }; // detail
  
  
  ref<Object>* ArrayImp::slotsFor(A* a, uint32 i, uint32& length) {
    N* n = detail::uncheckedSlotsFor(a, i);
    length = n->length;
    return n->_v;
  }
  
  ref<Object>* ArrayImp::slotsFor(TA* a, uint32 i, uint32& length) {
    N* n = detail::uncheckedSlotsFor(a, i);
    length = n->length;
    return n->_v;
  }

  
  Object* ArrayImp::findValue(A* a, uint32 i) {
    N* n = detail::checkedSlotsFor(a, i);
    const auto k = i & MASK;
    if (n && k < n->length) {
      return n->slot(k);
    }
    return nullptr; // empty branch or index out-of bounds
  }
  
  Object* ArrayImp::findValue(TA* a, uint32 i) {
    N* n = detail::checkedSlotsFor(a, i);
    const auto k = i & MASK;
    if (n && k < n->length) {
      return n->slot(k);
    }
    return nullptr; // empty branch or index out-of bounds
  }
  
  
  Object* ArrayImp::getValue(A* a, uint32 i) {
    Object* val = detail::uncheckedSlotsFor(a, i)->slot(i & MASK);
    ImmutableAssertTypeTag(val, A::ValueT::TYPE_TAG);
    return val;
  }
  
  Object* ArrayImp::getValue(TA* a, uint32 i) {
    Object* val = detail::uncheckedSlotsFor(a, i)->slot(i & MASK);
    ImmutableAssertTypeTag(val, A::ValueT::TYPE_TAG);
    return val;
  }
  
  
  Object* ArrayImp::firstValue(A* a) {
    return detail::firstValue(a);
  }
  
  Object* ArrayImp::firstValue(TA* a) {
    return detail::firstValue(a);
  }
  
  
  ArrayImp::A* ArrayImp::set(A* a, uint32 i, Object* obj) {
    return detail::set(a, i, obj);
  }
  
  ArrayImp::A* ArrayImp::push(A* a, Object* obj) {
    return detail::push(a, obj);
  }
  
  ArrayImp::A* ArrayImp::pop(A* a) {
    return detail::pop(a);
  }
  
  ArrayImp::TA* ArrayImp::pop(TA* a) {
    return detail::tpop(a);
  }
  
  
  ArrayImp::A* ArrayImp::slice(A* a, uint32 start, uint32 end) {
    // Note: assumed start and end are absolute
    if (!detail::isOutOfBounds(a, start, end)) {
      return nullptr;
    }
    if (start == end) {
      // [1 2 3 4 5] slice(2,2) => []
      return &EMPTY;
    }
    if (start == a->_start && end == a->_end) {
      // [1 2 3 4 5] slice(0,5) => [1 2 3 4 5]
      return a;
    }
    if (end == a->_end && end - start >= a->size()/2) {
      // [1 2 3 4 5] slice(2,END) => [3 4 5]
      // optimization where we return a with just a start offset, referencing
      // the same underlying data; root and tail are simply retained.
      // TODO: find a way to release inaccessible values. We can't simply release them
      //       here, because they are owned by root and tail, which are shared.
      return new A(start, end, a->_shift, a->_root, a->_tail);
    }
    // [1 2 3 4 5] slice(1,4) => [2 3 4]
    // because we rely on _end to be the true end of the underlying data (for push'ing),
    // we need to build a new array when the slice ends before _end.
    // We effectively create a new array with items in range [start,end) of a.
    auto t = createTransient(&EMPTY);
    detail::tpushAll(t, a, start, end); // add [start,end) from a onto b
    a = createPersistent(t);
    t->release();
    return a;
  }
  
  
  ArrayImp::A* ArrayImp::without(A* a, uint32 start, uint32 end) {
    // Note: assumed start and end are absolute
    if (!detail::isOutOfBounds(a, start, end)) {
      return nullptr;
    }
    if (start == end) {
      // [1 2 3 4 5] without(3,3) => [1 2 3 4 5]
      return a;
    }
    if (start == a->_start) {
      if (end == a->_end) {
        // [1 2 3 4 5] without(0,5) => []
        return &EMPTY;
      }
      // [1 2 3 4 5] without(0,3) => [4 5]
      return slice(a, end, a->_end);
    }
    if (end == a->_end) { // start != 0 && start != end
      // [1 2 3 4 5] without(2,5) => [1 2]
      return slice(a, a->_start, start);
    }
    // [1 2 3 4 5] without(2,4) => [1 2 5]
    // TODO: something more efficient
    ref<A> left = slice(a, a->_start, start); // [1 2]
    auto t = createTransient(left);
    detail::tpushAll(t, a, end, a->_end);
    a = createPersistent(t);
    t->release();
    return a;
  }


  ArrayImp::A* ArrayImp::cons(A* a, Object* val) {
    // [1 2 3] cons(0) => [0 1 2 3]
    // TODO: something more efficient
    auto t = createTransient(&EMPTY);
    detail::tpush(t, val);
    detail::tpushAll(t, a, a->_start, a->_end);
    a = createPersistent(t);
    t->release();
    return a;
  }
  
  
  ArrayImp::A* ArrayImp::splice(A* a, uint32 start, uint32 end, A::Iterator& it) {
    // Note: assumed start and end are absolute
    if (!detail::isOutOfBounds(a, start, end)) {
      return nullptr;
    }
    if (start == a->_start) {
      if (end == a->_end) {
        // [1 2 3 4 5] splice(0,5, [6 7]) => [6 7]
        return detail::pushAllIt(&ArrayImp::EMPTY, it);
      }
      // [1 2 3 4 5] splice(0,3, [6 7]) => [4 5 6 7]
      ref<A> b = slice(a, end, a->_end);
      return detail::pushAllIt(b, it);
    }
    if (end == a->_end) { // start != 0 && start != end
      if (start == end) {
        // [1 2 3 4 5] splice(5,5, [6 7]) => [1 2 3 4 5 6 7]
        return detail::pushAllIt(a, it);
      }
      // [1 2 3 4 5] splice(2,5, [6 7]) => [1 2 6 7]
      ref<A> b = slice(a, a->_start, start);
      return detail::pushAllIt(b, it);
    }
    // case: start > 0 && end < size
    // [1 2 3 4 5] splice(2,4, [6 7]) => [1 2 6 7 5]
    
    auto t = createTransient(&EMPTY);
    // add head, e.g. [1 2]
    detail::tpushAll(t, a, a->_start, start);
    // add new items, e.g. [1 2 6 7]
    while (it != END_ITERATOR) {
      detail::tpush(t, it.value());
      ++it;
    }
    // add tail, e.g. [1 2 6 7 5]
    detail::tpushAll(t, a, end, a->_end);
    a = createPersistent(t);
    t->release();
    return a;
  }
  
  
  ArrayImp::A* ArrayImp::splicefn(A* a, uint32 start, uint32 end, const ItFunc& next) {
    // Note: assumed start and end are absolute
    if (!detail::isOutOfBounds(a, start, end)) {
      return nullptr;
    }
    if (start == a->_start) {
      if (end == a->_end) {
        // [1 2 3 4 5] splice(0,5, [6 7]) => [6 7]
        return detail::pushAllFn(&ArrayImp::EMPTY, next);
      }
      // [1 2 3 4 5] splice(0,3, [6 7]) => [4 5 6 7]
      ref<A> b = slice(a, end, a->_end);
      return detail::pushAllFn(b, next);
    }
    if (end == a->_end) { // start != 0 && start != end
      if (start == end) {
        // [1 2 3 4 5] splice(5,5, [6 7]) => [1 2 3 4 5 6 7]
        return detail::pushAllFn(a, next);
      }
      // [1 2 3 4 5] splice(2,5, [6 7]) => [1 2 6 7]
      ref<A> b = slice(a, a->_start, start);
      return detail::pushAllFn(b, next);
    }
    // case: start > 0 && end < size
    // [1 2 3 4 5] splice(2,4, [6 7]) => [1 2 6 7 5]

    auto t = createTransient(&EMPTY);
    // add head, e.g. [1 2]
    detail::tpushAll(t, a, a->_start, start);
    // add new items, e.g. [1 2 6 7]
    A::ValueT* vptr;
    while ((vptr = next())) {
      detail::tpush(t, vptr);
    }
    // add tail, e.g. [1 2 6 7 5]
    detail::tpushAll(t, a, end, a->_end);
    a = createPersistent(t);
    t->release();
    return a;
  }
  
  
  ArrayImp::TA* ArrayImp::createTransient(A* a) {
    N* root = (N*)a->_root.ptr();
    N* editableRoot = root->copy(root->length, std::this_thread::get_id());
    N* editableTail = ((N*)a->_tail.ptr())->copy(BRANCHES, editableRoot->edit);
    return new TA(a->_start, a->_end, a->_shift, editableRoot, editableTail);
  }

  
  ArrayImp::TA* ArrayImp::set(TA* a, uint32 i, Object* obj) {
    return detail::tset(a, i, obj);
  }
  
  ArrayImp::TA* ArrayImp::push(TA* a, Object* obj) {
    return detail::tpush(a, obj);
  }
  
  
  ArrayImp::A* ArrayImp::createPersistent(TA* t) {
    if (!detail::isEditable(t)) {
      return nullptr;
    }
    N* root = &detail::root(t);
    root->edit = NO_EDIT;
    N* trimmedTail = detail::tail(t).copy(t->_end - detail::tailoff(t), NO_EDIT);
    return new A(t->_start, t->_end, t->_shift, root, trimmedTail);
  }
  
  
  //Object* ArrayImp::consPersistent(TA* t, A* a) {
  //  DCHECK(a != &EMPTY);
  //  if (!detail::isEditable(t)) {
  //    a->_end = 0;
  //    a->_shift = EMPTY._shift;
  //    a->_tail = EMPTY._tail;
  //    return EMPTY._root;
  //  }
  //  N* root = &detail::root(t);
  //  root->edit = NO_EDIT;
  //  N* trimmedTail = detail::tail(t).copy(t->_end - detail::tailoff(t), NO_EDIT);
  //  a->_end = t->_end;
  //  a->_shift = t->_shift;
  //  a->_tail = trimmedTail;
  //  return root;
  //}
  
  
  // The empty root node has BRANCHES number of slots, while the empty tail
  // does not have any slots at all.
  //
  // Because we allocate branch slots as part of a node, and thus the N struct
  // has no default space for branches. So we need to allocate storage in some
  // other way:
  static uint8 EMPTY_ROOT_storage[
    sizeof(ArrayImp::N) + (sizeof(ref<Object>) * BRANCHES)
  ];
  ArrayImp::N* EMPTY_ROOT = (ArrayImp::N*)&EMPTY_ROOT_storage;
  static struct _init {_init() {
    asm __volatile__ ("" : : : ); // stops _init from being stripped
    construct(EMPTY_ROOT, empty_initializer(), BRANCHES);
  }} __init;

  ArrayImp::N ArrayImp::EMPTY_NODE(empty_initializer(), 0);

  ArrayImp::A ArrayImp::EMPTY(EMPTY_ROOT, &EMPTY_NODE);
  
  A::Iterator ArrayImp::END_ITERATOR(nullptr);
  
} // namespace

