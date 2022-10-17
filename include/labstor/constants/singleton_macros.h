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

#ifndef LABSTOR_SINGLETON_MACROS_H
#define LABSTOR_SINGLETON_MACROS_H

#include <labstor/util/singleton.h>

#include <labstor/ipc_manager/ipc_manager.h>
#define LABSTOR_IPC_MANAGER scs::Singleton<labstor::IpcManager>::GetInstance()
#define LABSTOR_IPC_MANAGER_T labstor::IpcManager*

#include <labstor/runtime/configuration_manager.h>
#define LABSTOR_CONFIGURATION_MANAGER scs::Singleton<labstor::ConfigurationManager>::GetInstance()
#define LABSTOR_CONFIGURATION_MANAGER_T labstor::ConfigurationManager*

#include <labstor/introspect/system_info.h>
#define LABSTOR_SYSTEM_INFO scs::Singleton<labstor::SystemInfo>::GetInstance()
#define LABSTOR_SYSTEM_INFO_T labstor::SystemInfo*

#endif  // LABSTOR_SINGLETON_MACROS_H
