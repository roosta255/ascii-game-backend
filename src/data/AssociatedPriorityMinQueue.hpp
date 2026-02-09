#include <queue>
#include <vector>
#include <functional>

template<typename T>
class AssociatedPriorityMinQueue {
private:
    // Internal structure to associate priority with the data
    struct Entry {
        int priority;
        T element;

        // Custom comparator for "Min-Queue" behavior
        // std::priority_queue is a Max-Heap by default, 
        // so we use '>' to make the smallest value the "top".
        bool operator>(const Entry& other) const {
            return priority > other.priority;
        }
    };

    // Use std::greater to ensure the smallest priority is at the top
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> _queue;

public:
    // 1) Push an element with a specific priority
    void push(int priority, const T& element) {
        _queue.push({priority, element});
    }

    // 2) Pop the element with the lowest priority value
    // Returns false if the queue is empty
    bool pop(std::function<void(const T&)> consumer) {
        if (_queue.empty()) {
            return false;
        }

        // Get the top (lowest priority number)
        Entry top = _queue.top();
        _queue.pop();

        // Pass the element to the consumer
        consumer(top.element);
        
        return true;
    }

    // Useful helper
    bool isEmpty() const {
        return _queue.empty();
    }
    
    size_t size() const {
        return _queue.size();
    }
};