//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "ledger/miner/transaction_layout_queue.hpp"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace fetch {
namespace ledger {

/**
 * Adds a transaction layout to the queue
 *
 * @param item The transaction layout to be added to the queue
 * @return true if successful, otherwise false
 */
bool TransactionLayoutQueue::Add(TransactionLayout const &item)
{
  auto const &digest = item.digest();

  // try to add this digest and check if it's new
  bool success = digests_.insert(digest).second;

  if (success)
  {
    // this is a non-duplicate transaction
    list_.push_back(item);
  }

  return success;
}

/**
 * Remove a transaction specified by a digest
 *
 * @param digest The digest of the transaction layout to be removed
 * @return true if successful, otherwise false
 */
bool TransactionLayoutQueue::Remove(Digest const &digest)
{
  // try to remove this digest to see if it's known
  if (digests_.erase(digest) != 0)
  {
    // attempt to find the target digest
    auto const it = std::find_if(
        list_.begin(), list_.end(),
        [&digest](TransactionLayout const &layout) { return layout.digest() == digest; });
    assert(it != list_.end());
    list_.erase(it);
    return true;
  }

#ifndef NDEBUG
  {
    // there's no such digest among those enqueued, assert that there's no such layout
    auto const it = std::find_if(
        list_.begin(), list_.end(),
        [&digest](TransactionLayout const &layout) { return layout.digest() == digest; });
    assert(it == list_.end());
  }
#endif

  return false;
}

/**
 * Remove a set of transactions from the queue specified by the input set
 *
 * @param digests The set of the transaction digests to be removed
 * @return The number of transaction layouts removed from the queue
 */
std::size_t TransactionLayoutQueue::Remove(DigestSet const &digests_to_remove)
{
  const std::size_t current_population = digests_.size();

  if (!digests_to_remove.empty())
  {
    list_.remove_if([this, &digests_to_remove](TransactionLayout const &layout) {
      auto const &digest = layout.digest();

      // determine if we should remove this element
      bool const remove = digests_to_remove.count(digest) != 0;

      // also update the digests_to_remove set too
      if (remove)
      {
        digests_.erase(digest);
      }

      return remove;
    });
  }

  return current_population - digests_.size();
}

/**
 * Splice the contents of the specified queue onto the end of the current queue
 *
 * After the operation the contents of the input queue will be zero
 *
 * @param other The queue to be spliced on to the end of the existing one
 */
void TransactionLayoutQueue::Splice(TransactionLayoutQueue &other)
{
  // extract the queue from other queue
  UnderlyingList input = std::move(other.list_);
  other.list_.clear();
  other.digests_.clear();

  // loop through the queue and filter out the duplicate entries from the main queue
  for (auto it = input.begin(); it != input.end();)
  {
    bool const new_entry = digests_.find(it->digest()) == digests_.end();

    if (new_entry)
    {
      // update this queues digest set (in anticipation for new transaction objects)
      digests_.insert(it->digest());

      // advance on to the next entry
      ++it;
    }
    else
    {
      // remove the entry from the list
      it = other.list_.erase(it);
    }
  }

  // splice the contents
  list_.splice(list_.end(), std::move(input));
}

void TransactionLayoutQueue::Splice(TransactionLayoutQueue &other, Iterator start, Iterator end)
{
  // Step 1. Determine the first in the subset which is a non-duplicate
  Iterator current{start};
  while (current != end)
  {
    auto const &digest = current->digest();

    if (digests_.find(digest) == digests_.end())
    {
      break;
    }

    // duplicate - remove the element from the "other" list
    other.digests_.erase(digest);
    current = other.list_.erase(current);
  }

  // if all the elements are duplicate exit early
  if (current == end)
  {
    return;
  }

  // Step 2. We now have at least one element in the range which must be copied
  Iterator const begin{current};

  // loop through the queue and filter out the duplicate entries from the main queue
  while (current != end)
  {
    auto const &digest = current->digest();

    // remove the digest item from the "other" queue
    other.digests_.erase(digest);

    // determine if this is a new entry or not
    bool const new_entry = digests_.insert(digest).second;

    if (new_entry)
    {
      // advance on to the next entry
      ++current;
    }
    else
    {
      // remove the entry from the list
      current = other.list_.erase(current);
    }
  }

  // splice the contents of the new range into the
  list_.splice(list_.end(), other.list_, begin, end);
}

TransactionLayoutQueue::Iterator TransactionLayoutQueue::Erase(ConstIterator iterator)
{
  assert(iterator != cend());

  // remove the associated digest from the set
  digests_.erase(iterator->digest());

  // remove the list element
  return list_.erase(iterator);
}

TransactionLayoutQueue::Iterator TransactionLayoutQueue::Erase(ConstIterator first,
                                                               ConstIterator last)
{
  // remove the associated digests from the set
  for (auto itr = first; itr != last; ++itr)
  {
    digests_.erase(itr->digest());
  }

  // remove the list elements
  return list_.erase(first, last);
}

TransactionLayoutQueue::ConstIterator TransactionLayoutQueue::cbegin() const
{
  return list_.cbegin();
}

TransactionLayoutQueue::ConstIterator TransactionLayoutQueue::begin() const
{
  return list_.begin();
}

TransactionLayoutQueue::Iterator TransactionLayoutQueue::begin()
{
  return list_.begin();
}

TransactionLayoutQueue::ConstIterator TransactionLayoutQueue::cend() const
{
  return list_.cend();
}

TransactionLayoutQueue::ConstIterator TransactionLayoutQueue::end() const
{
  return list_.end();
}

TransactionLayoutQueue::Iterator TransactionLayoutQueue::end()
{
  return list_.end();
}

std::size_t TransactionLayoutQueue::size() const
{
  return digests_.size();
}

bool TransactionLayoutQueue::empty() const
{
  return digests_.empty();
}

DigestSet const &TransactionLayoutQueue::digests() const
{
  return digests_;
}

TransactionLayoutQueue::TxLayoutSet TransactionLayoutQueue::TxLayouts() const
{
  return {list_.cbegin(), list_.cend()};
}

}  // namespace ledger
}  // namespace fetch
