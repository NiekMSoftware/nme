#ifndef NME_CONTAINERS_LINKED_LIST_H
#define NME_CONTAINERS_LINKED_LIST_H

#include "nme/platform/debug/assert.h"
#include "nme/platform/types.h"

namespace nme {

// Intrusive doubly linked list node.
// Embed one ListLink per list a T can be in at once.
struct ListLink {
    ListLink* next;
    ListLink* prev;
};

template<typename T, ListLink T::* Link>
struct LinkedList {
    ListLink m_sentinel;        // ring anchor
    usize    m_count;
};

// --- link <-> element ---

// Recover the enclosing T from an embedded link by subtracting the member's offset.
template<typename T, ListLink T::* Link>
inline T* list_from_link(ListLink* link) {
    const uptr off = reinterpret_cast<usize>(&(reinterpret_cast<T*>(0)->*Link));
    return reinterpret_cast<T*>(reinterpret_cast<u8*>(link) - off);
}

// --- internal splice ---

namespace detail {
inline void list_link_insert(ListLink* node, ListLink* prev, ListLink* next) {
    node->next = next;
    node->prev = prev;
    prev->next = node;
    next->prev = node;
}
inline void list_link_remove(const ListLink* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}
}  // namespace detail

// --- lifetime ---

template<typename T, ListLink T::* Link>
inline void list_link_init(LinkedList<T, Link>* l) {
    l->m_sentinel.next = &l->m_sentinel;
    l->m_sentinel.prev = &l->m_sentinel;
    l->m_count = 0;
}

// --- queries ---

template<typename T, ListLink T::* Link>
inline usize list_count(const LinkedList<T, Link>* l) { return l->m_count; }
template<typename T, ListLink T::* Link>
inline bool list_empty(const LinkedList<T, Link>* l) { return l->m_count == 0; }

// --- insert / remove ---

template<typename T, ListLink T::* Link>
inline void list_push_front(LinkedList<T, Link>* l, T* e) {
    detail::list_link_insert(&e->*Link, &l->m_sentinel, l->m_sentinel.next);
    ++l->m_count;
}
template<typename T, ListLink T::* Link>
inline void list_push_back(LinkedList<T, Link>* l, T* e) {
    detail::list_link_insert(&e->*Link, l->m_sentinel.prev, &l->m_sentinel);
    ++l->m_count;
}

template<typename T, ListLink T::* Link>
inline void list_remove(LinkedList<T, Link>* l, T* e) {
    NME_ASSERT(l->m_count > 0);
    ListLink* n = &(e->*Link);
    detail::list_link_remove(n);
    n->next = n->prev = nullptr;
    --l->m_count;
}

// --- access ---

template<typename T, ListLink T::* Link>
inline T* list_front(LinkedList<T, Link>* l) {
    NME_ASSERT(l->m_count > 0);
    return list_from_link<T, Link>(l->m_sentinel.next);
}
template<typename T, ListLink T::* Link>
inline T* list_back(LinkedList<T, Link>* l) {
    NME_ASSERT(l->m_count > 0);
    return list_from_link<T, Link>(l->m_sentinel.prev);
}

// --- iter ---

template<typename T, ListLink T::* Link>
inline T* list_head(LinkedList<T, Link>* l) {
    return l->m_count > 0 ? list_from_link<T, Link>(l->m_sentinel.next) : nullptr;
}
template<typename T, ListLink T::* Link>
inline T* list_tail(LinkedList<T, Link>* l) {
    return l->m_count > 0 ? list_from_link<T, Link>(l->m_sentinel.prev) : nullptr;
}
template<typename T, ListLink T::* Link>
inline T* list_next(LinkedList<T, Link>* l, T* e) {
    ListLink* n = (e->*Link).next;
    return n == &l->m_sentinel ? nullptr : list_from_link<T, Link>(n);
}
template<typename T, ListLink T::* Link>
inline T* list_prev(LinkedList<T, Link>* l, T* e) {
    ListLink* p = (e->*Link).prev;
    return p == &l->m_sentinel ? nullptr : list_from_link<T, Link>(p);
}

}  // namespace nme

#endif  // NME_CONTAINERS_LINKED_LIST_H
