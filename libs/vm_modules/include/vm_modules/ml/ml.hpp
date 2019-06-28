#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "vm/module.hpp"
#include <cstdlib>

#include "vm_modules/ml/dataloaders/commodity_dataloader.hpp"
#include "vm_modules/ml/dataloaders/mnist_dataloader.hpp"

#include "vm_modules/ml/ops/loss_functions/cross_entropy.hpp"
#include "vm_modules/ml/ops/loss_functions/mean_square_error.hpp"

#include "vm_modules/ml/optimisation/adam_optimiser.hpp"

#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/state_dict.hpp"
#include "vm_modules/ml/training_pair.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {

inline void BindML(fetch::vm::Module &module)
{
  // ml fundamentals
  VMStateDict::Bind(module);
  VMGraph::Bind(module);
  VMTrainingPair::Bind(module);

  // dataloaders
  VMMnistDataLoader::Bind(module);
  VMCommodityDataLoader::Bind(module);

  // loss functions
  VMCrossEntropyLoss::Bind(module);
  VMMeanSquareError::Bind(module);

  // optimisers
  VMAdamOptimiser::Bind(module);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch