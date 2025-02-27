/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "plugin/device/ascend/hal/profiler/parallel_strategy_profiling.h"

namespace mindspore {
namespace profiler {
namespace ascend {
void ParallelStrategy::DumpProfileParallelStrategy(const FuncGraphPtr &func_graph) {}

std::shared_ptr<ParallelStrategy> &ParallelStrategy::GetInstance(){
  static std::shared_ptr<ParallelStrategy> parallel_strategy_inst_;
  return parallel_strategy_inst_;
}

void ParallelStrategy::SaveParallelStrategyToFile(){}
}  // namespace ascend
}  // namespace profiler
}  // namespace mindspore
