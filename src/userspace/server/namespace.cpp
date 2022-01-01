//
// Created by lukemartinlogan on 11/26/21.
//

#include <labstor/userspace/server/server.h>
#include <labstor/constants/debug.h>
#include <labstor/userspace/server/namespace.h>
#include <modules/kernel/secure_shmem/netlink_client/secure_shmem_client_netlink.h>
#include <modules/live_upgrade/live_upgrade.h>
#include <modules/registrar/server/registrar_server.h>

labstor::Server::Namespace::Namespace() {
    AUTO_TRACE("labstor::Server::Namespace::Namespace")
    LABSTOR_CONFIGURATION_MANAGER_T labstor_config_ = LABSTOR_CONFIGURATION_MANAGER;
    uint32_t max_entries = labstor_config_->config_["namespace"]["max_entries"].as<uint32_t>();
    uint32_t max_collisions = labstor_config_->config_["namespace"]["max_collisions"].as<uint32_t>();
    uint32_t request_unit = labstor_config_->config_["namespace"]["shmem_request_unit"].as<uint32_t>() * SizeType::BYTES;
    uint32_t shmem_size = labstor_config_->config_["namespace"]["shmem_kb"].as<uint32_t>() * SizeType::KB;
    region_size_ = shmem_size;

    //Create a shared memory region
    TRACEPOINT(max_entries, max_collisions, request_unit, shmem_size)
    LABSTOR_KERNEL_SHMEM_ALLOC_T shmem = LABSTOR_KERNEL_SHMEM_ALLOC;
    region_id_ = shmem->CreateShmem(shmem_size, true);
    if(region_id_ < 0) {
        throw SHMEM_CREATE_FAILED.format();
    }
    shmem->GrantPidShmem(getpid(), region_id_);
    region_ = shmem->MapShmem(region_id_, shmem_size);
    if(!region_) {
        throw MMAP_FAILED.format(strerror(errno));
    }
    TRACEPOINT(region_id_)

    //Initialize the namespace tables
    TRACEPOINT("NamespaceTables")
    uint32_t remainder = shmem_size;
    void *section = region_;
    ns_ids_.Init(section, labstor::ipc::mpmc::ring_buffer<uint32_t>::GetSize(max_entries));
    remainder -= ns_ids_.GetSize();
    section = ns_ids_.GetNextSection();
    key_to_ns_id_.Init(region_, section, labstor::ipc::mpmc::string_map::GetSize(max_entries), 0, 16);
    remainder -= key_to_ns_id_.GetSize();
    section = key_to_ns_id_.GetNextSection();
    //shared_state_.Init(section, labstor::ipc::array_uint32_t::GetSize(max_entries));
    //remainder -= shared_state_.GetSize();
    //section = shared_state_.GetNextSection();
    TRACEPOINT("NamespaceTables")

    //Create memory allocator on remaining memory
    TRACEPOINT("Allocator")
    labstor::ipc::shmem_allocator *alloc;
    alloc = new labstor::ipc::shmem_allocator();
    alloc->Init(region_, section, remainder, request_unit);
    shmem_alloc_ = alloc;
    TRACEPOINT("Allocator")
}

void labstor::Server::Namespace::Init() {
    //Add the registration module
    TRACEPOINT("AddKey")
    AddKey(labstor::ipc::string("Registrar", shmem_alloc_), new labstor::Registrar::Server());
    TRACEPOINT("AddKey")
}