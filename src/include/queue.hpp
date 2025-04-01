#include "util/lin_allocator.hpp"
#include <stdexcept>
#ifndef QUEUE_HPP
#define QUEUE_HPP

// Node structure for the linked list
template <typename T>
struct Node {
    T data;
    Node* next;

    Node(const T& value) : data(value), next(nullptr) {}
};

// Queue class
template <typename T>
class Queue {
private:
    Node<T>* frontNode;
    Node<T>* rearNode;
    int queueSize;

public:
    // Constructor
    Queue() : frontNode(nullptr), rearNode(nullptr), queueSize(0) {}

    // Destructor
    ~Queue() {
        while (!isEmpty()) {
            dequeue();
        }
    }

    // Enqueue operation
    void enqueue(const T& value) {
        Node<T>* newNode = make<Node<T>>(value);
        if (isEmpty()) {
            frontNode = rearNode = newNode;
        } else {
            rearNode->next = newNode;
            rearNode = newNode;
        }
        queueSize++;
    }

    // Dequeue operation
    void dequeue() {
        if (isEmpty()) {
            throw std::underflow_error("Queue underflow: Cannot dequeue from an empty queue.");
        }
        Node<T>* temp = frontNode;
        frontNode = frontNode->next;
        dealloc(temp);
        queueSize--;
        if (isEmpty()) {
            rearNode = nullptr;
        }
    }

    // Get the front element
    T front() const {
        if (isEmpty()) {
            throw std::underflow_error("Queue is empty: No front element.");
        }
        return frontNode->data;
    }

    // Check if the queue is empty
    bool isEmpty() const {
        return queueSize == 0;
    }

    // Get the size of the queue
    int size() const {
        return queueSize;
    }
};

#endif
