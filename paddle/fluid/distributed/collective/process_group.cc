// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "paddle/fluid/distributed/collective/process_group.h"

namespace paddle {
namespace distributed {

bool ProcessGroup::Task::IsCompleted() {
  std::lock_guard<std::mutex> lock(mutex_);
  return is_completed_;
}

ProcessGroup::ProcessGroup(int rank, int size, int gid)
    : rank_(rank), size_(size), gid_(gid) {
  if (gid != kIgnoreId) {
    auto map = ProcessGroupMapFromGid::getInstance();
    map->insert(gid_, this);
  }
}

// TODO(sunyilun): methods below will be removed later
ProcessGroupIdMap& ProcessGroupIdMap::GetInstance() {
  static ProcessGroupIdMap instance;
  return instance;
}

void ProcessGroupIdMap::DestroyProcessGroup(int gid) {
  int use_count = ProcessGroupIdMap::GetInstance()[gid].use_count();
  for (int i = 0; i < use_count; ++i) {
    ProcessGroupIdMap::GetInstance()[gid].reset();
  }
}

}  //  namespace distributed
}  //  namespace paddle
