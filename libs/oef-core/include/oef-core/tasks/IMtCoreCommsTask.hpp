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

#include "oef-base/comms/EndpointBase.hpp"
#include "oef-core/tasks-base/IMtCoreTask.hpp"
#include "oef-messages/fetch_protobuf.hpp"

#include <utility>

class IMtCoreCommsTask : public IMtCoreTask
{
public:
  explicit IMtCoreCommsTask(std::shared_ptr<EndpointBase<google::protobuf::Message>> endpoint)
    : endpoint{std::move(endpoint)}
  {}
  ~IMtCoreCommsTask() override = default;

protected:
  std::shared_ptr<EndpointBase<google::protobuf::Message>> endpoint;

private:
  IMtCoreCommsTask(const IMtCoreCommsTask &other) = delete;
  IMtCoreCommsTask &operator=(const IMtCoreCommsTask &other)  = delete;
  bool              operator==(const IMtCoreCommsTask &other) = delete;
  bool              operator<(const IMtCoreCommsTask &other)  = delete;
};
