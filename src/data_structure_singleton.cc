/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <labstor/constants/macros.h>
#include <labstor/util/singleton.h>
#include <labstor/constants/data_structure_singleton_macros.h>

#include <labstor/introspect/system_info.h>
template<> std::unique_ptr<labstor::SystemInfo> scs::Singleton<labstor::SystemInfo>::obj_ = nullptr;
#include <labstor/memory/memory_manager.h>
template<> std::unique_ptr<labstor::ipc::MemoryManager> scs::Singleton<labstor::ipc::MemoryManager>::obj_ = nullptr;
#include <labstor/thread/thread_manager.h>
template<> std::unique_ptr<labstor::ThreadManager> scs::Singleton<labstor::ThreadManager>::obj_ = nullptr;
