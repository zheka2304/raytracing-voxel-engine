#ifndef VOXEL_ENGINE_HEAP_H
#define VOXEL_ENGINE_HEAP_H

namespace voxel {

template<typename T, typename Index, typename Value>
class Heap {
public:
    using ItemRef = T*;

private:
    std::vector<ItemRef> m_items;
    Index m_item_index;
    Value m_item_value;

public:
    auto size() {
        return m_items.size();
    }

    void addItem(ItemRef item) {
        m_item_index(*item) = i32(m_items.size());
        m_items.push_back(item);
        sortUp(item);
        sortDown(item);
    }

    ItemRef removeItem(ItemRef removed_item) {
        i32 removed_index = m_item_index(*removed_item);
        i32 last_index = m_items.size() - 1;

        // if this is last item - just remove
        if (last_index == removed_index) {
            m_items.resize(last_index);
            return removed_item;
        }

        // swap last item and removed item
        std::swap(m_items[removed_index], m_items[last_index]);
        ItemRef last_item = m_items[removed_index];
        m_item_index(*last_item) = removed_index;
        m_item_index(*removed_item) = -1;

        // remove last item
        m_items.resize(last_index);

        // restore order
        sortUp(last_item);
        sortDown(last_item);

        return removed_item;
    }

    ItemRef getFirst() {
        return m_items.empty() ? nullptr : m_items[0];
    }

    ItemRef popFirst() {
        return m_items.empty() ? nullptr : removeItem(m_items[0]);
    }

    void updateItem(ItemRef item) {
        sortUp(item);
        sortDown(item);
    }

    void sortUp(ItemRef item) {
        i32 index = m_item_index(*item);
        i64 value = m_item_value(*item);
        while (index > 0) {
            i32 parent_index = (index - 1) / 2;
            ItemRef parent_item = m_items[parent_index];
            if (m_item_value(*parent_item) < value) {
                std::swap(m_items[index], m_items[parent_index]);
                m_item_index(*item) = parent_index;
                m_item_index(*parent_item) = index;
                index = parent_index;
            } else {
                break;
            }
        }
    }

    void sortDown(ItemRef item) {
        i32 index = m_item_index(*item);
        i64 value = m_item_value(*item);
        i32 heap_size = m_items.size();
        while (true) {
            i32 left_index = index * 2 + 2;
            i32 right_index = index * 2 + 1;
            ItemRef left_item = left_index < heap_size ? m_items[left_index] : nullptr;
            ItemRef right_item = right_index < heap_size ? m_items[right_index] : nullptr;

            i32 max_index;
            ItemRef max_item;
            if (!left_item || right_item && m_item_value(*left_item) < m_item_value(*right_item)) {
                max_index = right_index;
                max_item = right_item;
            } else {
                max_index = left_index;
                max_item = left_item;
            }

            if (max_item && m_item_value(*max_item) > value) {
                std::swap(m_items[index], m_items[max_index]);
                m_item_index(*item) = max_index;
                m_item_index(*max_item) = index;
                index = max_index;
            } else {
                break;
            }
        }
    }
};

}

#endif //VOXEL_ENGINE_HEAP_H
