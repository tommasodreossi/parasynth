/**
 * @file SetsUnion.h
 * @author Alberto Casagrande (acasagrande@units.it)
 * @brief Representing and handling unions of sets
 * @version 0.1
 * @date 2022-05-27
 *
 * @copyright Copyright (c) 2022
 */

#ifndef SETSUNION_H_
#define SETSUNION_H_

#include <list>
#include <memory>

#ifdef WITH_THREADS
#include <mutex>
#include <shared_mutex>

#include "SapoThreads.h"
#endif // WITH_THREADS

/**
 * @brief Unions of sets
 *
 * This class represents unions of sets.
 *
 * @tparam SET_TYPE is the set type
 */
template<class SET_TYPE>
class SetsUnion;

/**
 * @brief Unions of sets
 *
 * This template represents unions of sets as a
 * list of non-comparable sets: any two sets \f$A\f$ and
 * \f$B\f$ in the list are such that
 * \f$A \not\subseteq B\f$ and \f$A \not\supseteq B\f$.
 * Whenever a new sets \f$S\f$ is added to the union,
 * if \f$S\f$ is a subset for any set in the list,
 * the list does not change. Otherwise, all the sets in
 * the list included by \f$S\f$ are removed from the
 * list and \f$S\f$ itself is appended at its end.
 *
 * @tparam SET_TYPE is the set type
 */
template<class SET_TYPE>
class SetsUnion : private std::list<SET_TYPE>
{
  /**
   * @brief Add a set to a sets union
   *
   * This method adds a set `set_obj` to a union of sets.
   * First of all the method, tests whether the new set
   * is empty. If this is the case, no set is added to the
   * union. No set is added to the union also in the case
   * in which `set_obj` is subset of some of the first
   * `sets_to_cmp` elements in the list of the united sets.
   * Otherwise, all the first `sets_to_cmp` elements in the
   * list of the united sets that are subsets of `set_obj`
   * are removed from the list and `set_obj` is added
   * to the set.
   * The method returns a Boolean value that testifies
   * whether the computation has added `set_obj` at the
   * end of the sets list.
   *
   * @param[in] set_obj is the set to be added
   * @param[in] sets_to_cmp is the number of sets in the list
   *            to be compared to `set_obj`
   * @return `true` if and only if the computation has added
   *         `set_obj` at the end of the sets list
   */
  bool add(const SET_TYPE &set_obj, size_t sets_to_cmp)
  {
    if (size() != 0 && (this->front().dim() != set_obj.dim())) {
      throw std::domain_error("Adding a set to a union of sets "
                              "that has different dimension");
    }

    if (set_obj.is_empty()) {
      return false;
    }

    // for any of the first `sets_to_cmp` sets in the list
    auto it = std::begin(*this);
    while (it != std::end(*this) && sets_to_cmp-- > 0) {
      // if the set includes `set_obj`, then
      // `set_obj` is already included in the union
      // and the current object can be returned
      if (it->includes(set_obj)) {
        return false;
      }

      // if the set is a subset of `set_obj`, then
      // this set can be removed as `set_obj` is
      // going to be added to the list
      if (it->is_subset_of(set_obj)) {
        it = std::list<SET_TYPE>::erase(it);
      } else {
        ++it;
      }
    }

    // if `set_obj` is not a subset of any
    // of the sets in the head, then
    // append it to the union itself
    this->push_back(set_obj);

    return true;
  }

  /**
   * @brief Add a set to a sets union
   *
   * This method adds a set `set_obj` to a union of sets.
   * First of all the method, tests whether the new set
   * is empty. If this is the case, no set is added to the
   * union. No set is added to the union also in the case
   * in which `set_obj` is subset of some of the first
   * `sets_to_cmp` elements in the list of the united sets.
   * Otherwise, all the first `sets_to_cmp` elements in the
   * list of the united sets that are subsets of `set_obj`
   * are removed from the list and `set_obj` is added
   * to the set.
   * The method returns a Boolean value that testifies
   * whether the computation has added `set_obj` at the
   * end of the sets list.
   *
   * @param[in] set_obj is the set to be added
   * @param[in] sets_to_cmp is the number of sets in the list
   *            to be compared to `set_obj`
   * @return `true` if and only if the computation has added
   *         `set_obj` at the end of the sets list
   */
  bool add(SET_TYPE &&set_obj, size_t sets_to_cmp)
  {
    if (size() != 0 && (this->front().dim() != set_obj.dim())) {
      throw std::domain_error("Adding a set to a union of sets "
                              "that has different dimension");
    }

    if (set_obj.is_empty()) {
      return false;
    }

    // for any of the first `sets_to_cmp` sets in the list
    auto it = std::begin(*this);
    while (it != std::end(*this) && sets_to_cmp-- > 0) {
      // if the set includes `set_obj`, then
      // `set_obj` is already included in the union
      // and the current object can be returned
      if (it->includes(set_obj)) {
        return false;
      }

      // if the set is a subset of `set_obj`, then
      // this set can be removed as `set_obj` is
      // going to be added to the list
      if (it->is_subset_of(set_obj)) {
        it = std::list<SET_TYPE>::erase(it);
      } else {
        ++it;
      }
    }

    // if `set_obj` is not a subset of any
    // of the sets in the head, then
    // append it to the union itself
    this->push_back(std::move(set_obj));

    return true;
  }

public:
  /**
   * @brief A constant iterator alias
   */
  using const_iterator = typename std::list<SET_TYPE>::const_iterator;

  /**
   * @brief A iterator alias
   */
  using iterator = typename std::list<SET_TYPE>::iterator;

  /**
   * @brief Constructor
   *
   */
  SetsUnion() {}

  /**
   * @brief Constructor
   *
   * @param[in] set_obj is the set to be included in
   *            the union
   */
  SetsUnion(const SET_TYPE &set_obj)
  {
    this->add(set_obj);
  }

  /**
   * @brief Constructor
   *
   * @param[in] set_obj is the set to be included in
   *            the union
   */
  SetsUnion(SET_TYPE &&set_obj)
  {
    this->add(std::move(set_obj));
  }

  /**
   * @brief A copy constructor for a union of sets
   *
   * @param[in] orig is a union of sets
   */
  SetsUnion(const SetsUnion<SET_TYPE> &orig): std::list<SET_TYPE>(orig) {}

  /**
   * @brief Constructor
   *
   * @param[in] sets is a container of sets
   */
  SetsUnion(const std::list<SET_TYPE> &sets)
  {
    for (auto it = std::begin(sets); it != std::end(sets); ++it) {
      this->add(*it);
    }
  }

  /**
   * @brief Constructor
   *
   * @param[in] sets is a container of sets
   */
  SetsUnion(std::list<SET_TYPE> &&sets)
  {
    for (auto it = std::begin(sets); it != std::end(sets); ++it) {
      this->add(std::move(*it));
    }
  }

  /**
   * @brief A swap constructor for a union of sets
   *
   * @param[in] orig is a union of sets
   */
  SetsUnion(SetsUnion<SET_TYPE> &&orig)
  {
    std::swap(*(static_cast<std::list<SET_TYPE> *>(this)),
              *(static_cast<std::list<SET_TYPE> *>(&orig)));
  }

  /**
   * @brief An assignment operator for unions of sets
   *
   * @param[in] orig is a union of sets
   */
  SetsUnion<SET_TYPE> &operator=(const SetsUnion<SET_TYPE> &orig)
  {
    this->clear();

    for (auto it = std::cbegin(orig); it != std::cend(orig); ++it) {
      this->push_back(*it);
    }

    return *this;
  }

  /**
   * @brief A swap assignment for unions of sets
   *
   * @param[in] orig is a union of sets
   */
  SetsUnion<SET_TYPE> &operator=(SetsUnion<SET_TYPE> &&orig)
  {
    std::swap(*(static_cast<std::list<SET_TYPE> *>(this)),
              *(static_cast<std::list<SET_TYPE> *>(&orig)));

    return *this;
  }

  /**
   * @brief Add a set to the union
   *
   * This method adds a set `set_obj` to a union of sets.
   * First of all the method, tests whether the new set
   * is empty. If this is the case, no set is added to the
   * union. No set is added to the union also in the case
   * in which `set_obj` is subset of some of the elements
   * in the list of the united sets.
   * Otherwise, the elements in the list of the united
   * sets that are subsets of `set_obj` are removed from
   * the list and `set_obj` is added to the set.
   *
   * @param[in] set_obj is the set to be added
   * @return a reference to the updated union
   */
  inline SetsUnion<SET_TYPE> &add(const SET_TYPE &set_obj)
  {
    add(set_obj, size());

    return *this;
  }

  /**
   * @brief Add a set to the union
   *
   * This method adds a set `set_obj` to a union of sets.
   * First of all the method, tests whether the new set
   * is empty. If this is the case, no set is added to the
   * union. No set is added to the union also in the case
   * in which `set_obj` is subset of some of the elements
   * in the list of the united sets.
   * Otherwise, the elements in the list of the united
   * sets that are subsets of `set_obj` are removed from
   * the list and `set_obj` is added to the set.
   *
   * @param[in] set_obj is the set to be added
   * @return a reference to the updated union
   */
  inline SetsUnion<SET_TYPE> &add(SET_TYPE &&set_obj)
  {
    add(std::move(set_obj), size());

    return *this;
  }

  /**
   * @brief Update a sets union by joining another sets union
   *
   * This method works in-place and changes the calling object.
   *
   * @param[in] sets_union is a sets union
   * @return a reference to the updated object
   */
  SetsUnion<SET_TYPE> &update(const SetsUnion<SET_TYPE> &sets_union)
  {
    unsigned int from_sets_union = 0;

    for (auto it = std::cbegin(sets_union); it != std::cend(sets_union);
         ++it) {
      if (this->add(*it, size() - from_sets_union)) {
        ++from_sets_union;
      }
    }

    return *this;
  }

  /**
   * @brief Update a sets union by joining another sets union
   *
   * This method works in-place and changes the calling object.
   *
   * @param[in] sets_union is a sets union
   * @return a reference to the updated object
   */
  SetsUnion<SET_TYPE> &update(SetsUnion<SET_TYPE> &&sets_union)
  {
    unsigned int from_sets_union = 0;

    for (auto it = std::begin(sets_union); it != std::end(sets_union); ++it) {
      if (this->add(std::move(*it), size() - from_sets_union)) {
        ++from_sets_union;
      }
    }

    return *this;
  }

  /**
   * @brief Check whether one of the sets in a union contains a set
   *
   * @param[in] set_obj is the set whose inclusion must be tested
   * @return `true` if and only if `set` is a subset of some of
   *         the sets in the union
   */
  bool any_includes(const SET_TYPE &set_obj) const
  {

    if (set_obj.is_empty()) {
      return true;
    }

#ifdef WITH_THREADS
    class ThreadResult
    {
      mutable std::shared_timed_mutex mutex;
      bool value;

    public:
      ThreadResult(): value(false) {}

      bool get() const
      {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex);

        return value;
      }

      void set(const bool &value)
      {
        std::unique_lock<std::shared_timed_mutex> wlock(mutex);

        this->value = value;
      }
    };

    ThreadResult result;

    auto check_and_update = [&result, &set_obj](const SET_TYPE &s) {
      if (!result.get() && set_obj.is_subset_of(s)) {
        result.set(true);
      }
    };

    ThreadPool::BatchId batch_id = thread_pool.create_batch();

    for (auto it = std::cbegin(*this); it != std::cend(*this); ++it) {
      // submit the task to the thread pool
      thread_pool.submit_to_batch(batch_id, check_and_update, std::ref(*it));
    }

    // join to the pool threads
    thread_pool.join_threads(batch_id);

    // close the batch
    thread_pool.close_batch(batch_id);

    return result.get();
#else  // WITH_THREADS
    for (auto it = std::cbegin(*this); it != std::cend(*this); ++it) {
      if (set_obj.is_subset_of(*this)) {
        return true;
      }
    }

    return false;
#endif // WITH_THREADS
  }

  /**
   * @brief Get the number of sets in the union
   *
   * @returns the number of sets in the union
   */
  size_t size() const
  {
    return std::list<SET_TYPE>::size();
  }

  /**
   * @brief Get the space dimension of the sets
   *
   * @returns the space dimension of the sets or
   *          0 if the union is the empty
   */
  size_t dim() const
  {
    if (this->empty()) {
      return 0;
    }

    return this->front().dim();
  }

  /**
   * @brief The begin iterator
   *
   * @return the begin iterator
   */
  iterator begin()
  {
    return std::list<SET_TYPE>::begin();
  }

  /**
   * @brief The end iterator
   *
   * @return the end iterator
   */
  iterator end()
  {
    return std::list<SET_TYPE>::end();
  }

  /**
   * @brief The constant begin iterator
   *
   * @return the constant begin iterator
   */
  const_iterator begin() const
  {
    return std::list<SET_TYPE>::begin();
  }

  /**
   * @brief The constant end iterator
   *
   * @return the constant end iterator
   */
  const_iterator end() const
  {
    return std::list<SET_TYPE>::end();
  }

  /**
   * @brief Test whether the union of sets is empty
   *
   * @return `true` if and only if the union of sets is empty
   */
  inline bool is_empty() const
  {
    // since sets are added to the union exclusively by using
    // add methods and those methods exclusively push in the
    // list non-empty set, the union is empty if and only if
    // the list is empty

    return this->empty();
  }
};

/**
 * Compute the intersection of two sets unions
 *
 * @param[in] A is a sets union
 * @param[in] B is a sets union
 * @return the sets union representing \f$A \cap B\f$
 */
template<typename SET_TYPE>
SetsUnion<SET_TYPE> intersect(const SetsUnion<SET_TYPE> &A,
                              const SetsUnion<SET_TYPE> &B)
{
  SetsUnion<SET_TYPE> result;

  for (auto t_it = std::begin(A); t_it != std::end(A); ++t_it) {
    for (auto s_it = std::begin(B); s_it != std::end(B); ++s_it) {
      result.add(intersect(*t_it, *s_it));
    }
  }

  return result;
}

/**
 * Compute the intersection between a sets unions and a set
 *
 * @tparam SET_TYPE is the set type
 * @param[in] A is a sets union
 * @param[in] B is a set
 * @return the sets union representing \f$A \cap B\f$
 */
template<typename SET_TYPE>
inline SetsUnion<SET_TYPE> intersect(const SetsUnion<SET_TYPE> &A,
                                     const SET_TYPE &B)
{
  return intersect(A, SetsUnion<SET_TYPE>(B));
}

/**
 * Compute the intersection between a sets unions and a set
 *
 * @tparam SET_TYPE is the set type
 * @param[in] A is a set
 * @param[in] B is a sets union
 * @return the bundles union representing \f$A \cap B\f$
 */
template<typename SET_TYPE>
inline SetsUnion<SET_TYPE> intersect(const SET_TYPE &A,
                                     const SetsUnion<SET_TYPE> &B)
{
  return intersect(B, A);
}

/**
 * Compute the union of two sets unions
 *
 * @tparam SET_TYPE is the set type
 * @param[in] A is a sets union
 * @param[in] B is a sets union
 * @return the sets union representing \f$A \cup B\f$
 */
template<typename SET_TYPE>
inline SetsUnion<SET_TYPE> make_union(const SetsUnion<SET_TYPE> &A,
                                      const SetsUnion<SET_TYPE> &B)
{
  return SetsUnion<SET_TYPE>(A).update(B);
}

/**
 * Compute the union of two sets
 *
 * @tparam SET_TYPE is the set type
 * @param[in] A is a set
 * @param[in] B is a set
 * @return the sets union representing \f$A \cup B\f$
 */
template<typename SET_TYPE>
inline SetsUnion<SET_TYPE> make_union(const SET_TYPE &A, const SET_TYPE &B)
{
  return SetsUnion<SET_TYPE>(A).add(B);
}

#endif /* SETSUNION_H_ */
