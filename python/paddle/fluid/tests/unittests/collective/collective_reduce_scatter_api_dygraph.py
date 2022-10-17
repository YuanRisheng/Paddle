# Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import paddle
import paddle.distributed as dist
import paddle.fluid as fluid
import test_collective_api_base as test_base


class TestCollectiveReduceScatterAPI(test_base.TestCollectiveAPIRunnerBase):

    def __init__(self):
        self.global_ring_id = 0

    def get_model(self, main_prog, startup_program, rank, indata=None):
        with fluid.program_guard(main_prog, startup_program):
            # NOTE: this is a hack relying on an undocumented behavior that `to_tensor` uses uint16 to replace bfloat16
            if indata.dtype == "bfloat16":
                tindata = paddle.to_tensor(indata, "float32").cast("uint16")
                subdata1, subdata2 = paddle.split(tindata, 2, axis=0)
                dist.reduce_scatter(subdata1, [subdata1, subdata2])
                return [subdata1.cast("float32").numpy()]
            else:
                tindata = paddle.to_tensor(indata)
                subdata1, subdata2 = paddle.split(tindata, 2, axis=0)
                dist.reduce_scatter(subdata1, [subdata1, subdata2])
                return [subdata1.numpy()]


if __name__ == "__main__":
    test_base.runtime_main(TestCollectiveReduceScatterAPI, "reduce_scatter")
