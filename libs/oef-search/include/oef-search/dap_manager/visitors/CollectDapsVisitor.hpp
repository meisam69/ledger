#pragma once
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

#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/dap_manager/Visitor.hpp"

#include <memory>

class CollectDapsVisitor : public Visitor<Stack>
{
public:
  static constexpr char const *LOGGING_NAME = "CollectDapsVisitor";
  using VisitNodeExitStates                 = typename Visitor<Stack>::VisitNodeExitStates;

  CollectDapsVisitor() = default;

  virtual ~CollectDapsVisitor() = default;

  virtual VisitNodeExitStates VisitNode(Branch &node, uint32_t /*depth*/)
  {
    node.MergeDaps();
    return VisitNodeExitStates::COMPLETE;
  }
  virtual VisitNodeExitStates VisitLeaf(Leaf & /*leaf*/, uint32_t /*depth*/)
  {
    return VisitNodeExitStates::COMPLETE;
  }

protected:
private:
  CollectDapsVisitor(const CollectDapsVisitor &other) = delete;  // { copy(other); }
  CollectDapsVisitor &operator                        =(const CollectDapsVisitor &other) =
      delete;                                                 // { copy(other); return *this; }
  bool operator==(const CollectDapsVisitor &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const CollectDapsVisitor &other) = delete;  // const { return compare(other)==-1; }
};
