#ifndef NME_CONTAINERS_LINKED_LIST_H
#define NME_CONTAINERS_LINKED_LIST_H
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

template<typename T, ListLink T::* Link>
inline T* list_from_link(ListLink* link) {
    // offset of the Link member in T
    const uptr off = reinterpret_cast<usize>(&(reinterpret_cast<T*>(0)->*Link));
    return reinterpret_cast<T*>(reinterpret_cast<u8*>(link) - off);
}

}  // namespace nme

#endif  // NME_CONTAINERS_LINKED_LIST_H
