#ifndef VOXEL_ENGINE_THREADING_H
#define VOXEL_ENGINE_THREADING_H

#include "voxel/common/threading/task_executor.h"
#include "voxel/common/threading/thread_pool.h"
#include "voxel/common/threading/worker_thread.h"

namespace voxel {
    using ThreadLock = std::unique_lock<std::mutex>;
}

#endif //VOXEL_ENGINE_THREADING_H
