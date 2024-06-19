// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <memory>
#include <vector>

namespace ocpp {

/// \brief Helper struct that allows the of an iterator in an interface, can be implemented using any forward iterator
template <typename T> struct ForwardIteratorWrapper {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;

    /// \brief Abstract struct to implement to enable the use of this iterator wrapper
    struct Base {
        virtual ~Base() = default;

        /// \brief Get a reference to the value pointed to by the iterator
        virtual reference deref() const = 0;
        /// \brief Increment the iterator once to the next position
        virtual void advance() = 0;
        /// \brief Check for equality between this iterator and \p other
        virtual bool not_equal(const Base& other) = 0;
    };

    /// \brief Construct a new wrapper using any type that implements the abstract struct Base
    explicit ForwardIteratorWrapper(std::unique_ptr<Base> it) : it{std::move(it)} {
    }

    /// \brief Get a reference to the value pointed to by the iterator
    reference operator*() const {
        return it->deref();
    };

    /// \brief Increment the iterator once to the next position
    ForwardIteratorWrapper& operator++() {
        it->advance();
        return *this;
    }

    /// \brief Check for inequality between this \p a and \p b
    friend bool operator!=(const ForwardIteratorWrapper& a, const ForwardIteratorWrapper& b) {
        return a.it->not_equal(*b.it);
    };

private:
    std::unique_ptr<Base> it;
};

/// \brief Implementation for the ForwardIteratorWrapper based on a vector of unique_ptr with type T
template <typename T> struct UniquePtrVectorIterator : ForwardIteratorWrapper<T>::Base {
    using iterator_type = typename std::vector<std::unique_ptr<T>>::iterator;
    using reference = typename ForwardIteratorWrapper<T>::reference;
    using base = typename ForwardIteratorWrapper<T>::Base;

    explicit UniquePtrVectorIterator(iterator_type it) : it{it} {
    }

    reference deref() const override {
        return *it->get();
    }

    void advance() override {
        ++it;
    }

    bool not_equal(const base& other) override {
        return it != dynamic_cast<const UniquePtrVectorIterator&>(other).it;
    }

private:
    iterator_type it;
};

} // namespace ocpp