/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#include "ps/ps_context.h"

#include "kernel/kernel.h"
#include "utils/log_adapter.h"
#include "utils/ms_utils.h"
#if ((defined ENABLE_CPU) && (!defined _WIN32) && !defined(__APPLE__))
#include "distributed/cluster/cluster_context.h"
#include "ps/ps_cache/ps_data/ps_data_prefetch.h"
#include "distributed/embedding_cache/embedding_cache_utils.h"
#else
#include "distributed/cluster/dummy_cluster_context.h"
#endif

namespace mindspore {
namespace ps {
std::shared_ptr<PSContext> PSContext::instance() {
  static std::once_flag init_flag;
  static std::shared_ptr<PSContext> ps_instance = nullptr;
  std::call_once(init_flag, [&]() {
    if (ps_instance == nullptr) {
      ps_instance.reset(new (std::nothrow) PSContext());
      MS_EXCEPTION_IF_NULL(ps_instance);
    }
  });

  return ps_instance;
}

void PSContext::SetPSEnable(bool enabled) {
  ps_enabled_ = enabled;
  if (ps_enabled_) {
    std::string ms_role = common::GetEnv(kEnvRole);
    if (ms_role == "") {
      ms_role = this->ms_role();
    }
    MS_LOG(INFO) << "PS mode is enabled. MS_ROLE is " << ms_role;

    if (ms_role == kEnvRoleOfWorker) {
      is_worker_ = true;
    } else if (ms_role == kEnvRoleOfPServer || ms_role == kEnvRoleOfServer) {
      is_pserver_ = true;
    } else if (ms_role == kEnvRoleOfScheduler) {
      is_sched_ = true;
    }

    worker_num_ = std::strtol(common::GetEnv(kEnvWorkerNum).c_str(), nullptr, kBase);
    server_num_ = std::strtol(common::GetEnv(kEnvPServerNum).c_str(), nullptr, kBase);
    scheduler_host_ = common::GetEnv(kEnvSchedulerHost);
    if (scheduler_host_.length() > kLength) {
      MS_LOG(EXCEPTION) << "The scheduler host's length can not exceed " << kLength;
    }
    scheduler_port_ = std::strtol(common::GetEnv(kEnvSchedulerPort).c_str(), nullptr, kBase);
    if (scheduler_port_ > kMaxPort) {
      MS_LOG(EXCEPTION) << "The port: " << scheduler_port_ << " is illegal.";
    }
    scheduler_manage_port_ =
      static_cast<uint16_t>((std::strtol(common::GetEnv(kEnvSchedulerManagePort).c_str(), nullptr, kBase)));
    if (scheduler_manage_port_ > kMaxPort) {
      MS_LOG(EXCEPTION) << "The port << " << scheduler_manage_port_ << " is illegal.";
    }
    cluster_config_ = std::make_unique<core::ClusterConfig>(worker_num_, server_num_, scheduler_host_, scheduler_port_);
    node_id_ = common::GetEnv(kEnvNodeId);
    if (node_id_.length() > kLength) {
      MS_LOG(EXCEPTION) << "The node id length can not exceed " << kLength;
    }
    server_mode_ = kServerModePS;
  } else {
    MS_LOG(INFO) << "PS mode is disabled.";
    is_worker_ = false;
    is_pserver_ = false;
    is_sched_ = false;
  }
}

bool PSContext::is_ps_mode() const { return ps_enabled_; }

void PSContext::Reset() {
  ps_enabled_ = false;
  is_worker_ = false;
  is_pserver_ = false;
  is_sched_ = false;
#if ((defined ENABLE_CPU) && (!defined _WIN32) && !defined(__APPLE__))
  if (ps::PsDataPrefetch::GetInstance().cache_enable()) {
    set_cache_enable(false);
  }
#endif
}

std::string PSContext::ms_role() const {
  if (is_worker_) {
    return kEnvRoleOfWorker;
  } else if (is_pserver_) {
    return kEnvRoleOfPServer;
  } else if (is_sched_) {
    return kEnvRoleOfScheduler;
  } else {
    return kEnvRoleOfNotPS;
  }
}

bool PSContext::is_worker() const {
  if (distributed::cluster::ClusterContext::instance()->initialized()) {
    return role_ == kEnvRoleOfWorker;
  }
  return is_worker_;
}

bool PSContext::is_server() const {
  if (distributed::cluster::ClusterContext::instance()->initialized()) {
    return role_ == kEnvRoleOfServer || role_ == kEnvRoleOfPServer;
  }
  return is_pserver_;
}

bool PSContext::is_scheduler() const {
  if (distributed::cluster::ClusterContext::instance()->initialized()) {
    return role_ == kEnvRoleOfScheduler;
  }
  return is_sched_;
}

uint32_t PSContext::initial_worker_num() const { return worker_num_; }

uint32_t PSContext::initial_server_num() const { return server_num_; }

std::string PSContext::scheduler_host() const { return scheduler_host_; }

void PSContext::SetPSRankId(uint32_t rank_id) { rank_id_ = rank_id; }

uint32_t PSContext::ps_rank_id() const { return rank_id_; }

void PSContext::InsertHashTableSize(const std::string &param_name, size_t cache_vocab_size, size_t embedding_size,
                                    size_t vocab_size, int32_t param_key) const {
#if ((defined ENABLE_CPU) && (!defined _WIN32) && !defined(__APPLE__))
  if (enable_distributed_mindrt()) {
    embedding_cache_table_manager.InsertHashTableSize(param_name, cache_vocab_size, embedding_size, vocab_size,
                                                      param_key);
  }
#endif
}

void PSContext::ReInsertHashTableSize(const std::string &new_param_name, const std::string &cur_param_name,
                                      size_t cache_vocab_size, size_t embedding_size) const {
#if ((defined ENABLE_CPU) && (!defined _WIN32) && !defined(__APPLE__))
  if (enable_distributed_mindrt()) {
    embedding_cache_table_manager.ReInsertHashTableSize(new_param_name, cur_param_name, cache_vocab_size,
                                                        embedding_size);
  }
#endif
}

void PSContext::InsertWeightInitInfo(const std::string &, size_t, size_t) const { return; }

void PSContext::InsertAccumuInitInfo(const std::string &param_name, float init_val) const {
#if ((defined ENABLE_CPU) && (!defined _WIN32) && !defined(__APPLE__))
  embedding_cache_table_manager.InsertAccumuInitInfo(param_name, init_val);
#endif
}

void PSContext::CloneHashTable(const std::string &dest_param_name, int32_t dest_param_key,
                               const std::string &src_param_name, int32_t src_param_key) const {
#if ((defined ENABLE_CPU) && (!defined _WIN32) && !defined(__APPLE__))
  if (enable_distributed_mindrt()) {
    embedding_cache_table_manager.CloneHashTable(dest_param_name, dest_param_key, src_param_name, src_param_key);
  }
#endif
}

void PSContext::set_cache_enable(bool cache_enable) const {
#if ((defined ENABLE_CPU) && (!defined _WIN32) && !defined(__APPLE__))
  PsDataPrefetch::GetInstance().set_cache_enable(cache_enable);
#endif
}

bool PSContext::cache_enable() const {
#if ((defined ENABLE_CPU) && (!defined _WIN32)) && !defined(__APPLE__)
  return PsDataPrefetch::GetInstance().cache_enable();
#endif
  return false;
}

void PSContext::set_rank_id(uint32_t) const { return; }

void PSContext::set_server_mode(const std::string &server_mode) {
  if (server_mode != kServerModePS) {
    MS_LOG(EXCEPTION) << server_mode << " is invalid. Server mode must be " << kServerModePS;
    return;
  }
  MS_LOG(INFO) << "Server mode: " << server_mode << " is used for Server and Worker. Scheduler will ignore it.";
  server_mode_ = server_mode;
}

const std::string &PSContext::server_mode() const { return server_mode_; }

void PSContext::set_ms_role(const std::string &role) {
  if (role != kEnvRoleOfWorker && role != kEnvRoleOfPServer && role != kEnvRoleOfServer &&
      role != kEnvRoleOfScheduler) {
    MS_LOG(EXCEPTION) << "ms_role " << role << " is invalid.";
    return;
  }
  MS_LOG(INFO) << "MS_ROLE of this node is " << role;
  role_ = role;
}

void PSContext::set_worker_num(uint32_t worker_num) { worker_num_ = worker_num; }
uint32_t PSContext::worker_num() const { return worker_num_; }

void PSContext::set_server_num(uint32_t server_num) { server_num_ = server_num; }
uint32_t PSContext::server_num() const { return server_num_; }

void PSContext::set_scheduler_ip(const std::string &sched_ip) { scheduler_host_ = sched_ip; }

std::string PSContext::scheduler_ip() const { return scheduler_host_; }

void PSContext::set_scheduler_port(uint16_t sched_port) { scheduler_port_ = sched_port; }

uint16_t PSContext::scheduler_port() const { return scheduler_port_; }

core::ClusterConfig &PSContext::cluster_config() {
  if (cluster_config_ == nullptr) {
    cluster_config_ = std::make_unique<core::ClusterConfig>(worker_num_, server_num_, scheduler_host_, scheduler_port_);
    MS_EXCEPTION_IF_NULL(cluster_config_);
  }
  return *cluster_config_;
}

void PSContext::set_scheduler_manage_port(uint16_t sched_port) { scheduler_manage_port_ = sched_port; }
uint16_t PSContext::scheduler_manage_port() const { return scheduler_manage_port_; }

void PSContext::set_config_file_path(const std::string &path) { config_file_path_ = path; }

std::string PSContext::config_file_path() const { return config_file_path_; }

void PSContext::set_node_id(const std::string &node_id) { node_id_ = node_id; }

const std::string &PSContext::node_id() const { return node_id_; }

bool PSContext::enable_ssl() const { return enable_ssl_; }

void PSContext::set_enable_ssl(bool enabled) { enable_ssl_ = enabled; }

std::string PSContext::client_password() const { return client_password_; }
void PSContext::set_client_password(const std::string &password) { client_password_ = password; }

std::string PSContext::server_password() const { return server_password_; }
void PSContext::set_server_password(const std::string &password) { server_password_ = password; }

std::string PSContext::http_url_prefix() const { return http_url_prefix_; }

void PSContext::set_instance_name(const std::string &instance_name) { instance_name_ = instance_name; }

const std::string &PSContext::instance_name() const { return instance_name_; }

bool PSContext::enable_distributed_mindrt() const {
  bool ms_cluster_enabled = distributed::cluster::ClusterContext::instance()->initialized();
  return ms_cluster_enabled;
}
}  // namespace ps
}  // namespace mindspore
