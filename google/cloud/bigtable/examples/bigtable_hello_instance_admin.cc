// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! [all code]

//! [bigtable includes]
#include "google/cloud/bigtable/instance_admin.h"
//! [bigtable includes]
#include <iostream>

int main(int argc, char* argv[]) try {
  if (argc != 5) {
    std::string const cmd = argv[0];
    auto last_slash = std::string(cmd).find_last_of('/');
    auto program = cmd.substr(last_slash + 1);
    std::cerr << "\nUsage: " << program
              << " <project-id> <instance-id> <cluster-id> <zone>\n\n"
              << "Example: " << program
              << " my-project my-instance my-instance-c1 us-central1-f\n";
    return 1;
  }

  std::string const project_id = argv[1];
  std::string const instance_id = argv[2];
  std::string const cluster_id = argv[3];
  std::string const zone = argv[4];

  //! [aliases]
  namespace cbt = google::cloud::bigtable;
  using google::cloud::future;
  using google::cloud::StatusOr;
  //! [aliases]

  // Connect to the Cloud Bigtable admin endpoint.
  //! [connect instance admin]
  cbt::InstanceAdmin instance_admin(
      cbt::CreateDefaultInstanceAdminClient(project_id, cbt::ClientOptions()));
  //! [connect instance admin]

  //! [check instance exists]
  std::cout << "\nCheck Instance exists:\n";
  StatusOr<cbt::InstanceList> instances = instance_admin.ListInstances();
  if (!instances) {
    throw std::runtime_error(instances.status().message());
  }
  if (!instances->failed_locations.empty()) {
    std::cerr
        << "The service tells us it has no information about these locations:";
    for (auto const& failed_location : instances->failed_locations) {
      std::cerr << " " << failed_location;
    }
    std::cerr << ". Continuing anyway\n";
  }
  auto instance_name =
      instance_admin.project_name() + "/instances/" + instance_id;
  auto instance_name_it = std::find_if(
      instances->instances.begin(), instances->instances.end(),
      [&instance_name](google::bigtable::admin::v2::Instance const& i) {
        return i.name() == instance_name;
      });
  bool instance_exists = instance_name_it != instances->instances.end();
  std::cout << "The instance " << instance_id
            << (instance_exists ? "does" : "does not") << " exist already\n";
  //! [check instance exists]

  // Create instance if does not exists
  if (!instance_exists) {
    //! [create production instance]
    std::cout << "\nCreating a PRODUCTION Instance: ";

    // production instance needs at least 3 nodes
    auto cluster_config = cbt::ClusterConfig(zone, 3, cbt::ClusterConfig::HDD);
    cbt::InstanceConfig config(instance_id, "Sample Instance",
                               {{cluster_id, cluster_config}});
    config.set_type(cbt::InstanceConfig::PRODUCTION);

    google::cloud::future<void> creation_done =
        instance_admin.CreateInstance(config).then(
            [instance_id](
                future<StatusOr<google::bigtable::admin::v2::Instance>> f) {
              auto instance = f.get();
              if (!instance) {
                std::cerr << "Could not create instance " << instance_id
                          << "\n";
                throw std::runtime_error(instance.status().message());
              }
              std::cout << "Successfully created instance: "
                        << instance->DebugString() << "\n";
            });
    // Note how this blocks until the instance is created, in production code
    // you may want to perform this task asynchronously.
    creation_done.get();
    std::cout << "DONE\n";
    //! [create production instance]
  }

  //! [list instances]
  std::cout << "\nListing Instances:\n";
  instances = instance_admin.ListInstances();
  if (!instances) {
    throw std::runtime_error(instances.status().message());
  }
  if (!instances->failed_locations.empty()) {
    std::cerr
        << "The service tells us it has no information about these locations:";
    for (auto const& failed_location : instances->failed_locations) {
      std::cerr << " " << failed_location;
    }
    std::cerr << ". Continuing anyway\n";
  }
  for (auto const& instance : instances->instances) {
    std::cout << "  " << instance.name() << "\n";
  }
  std::cout << "DONE\n";
  //! [list instances]

  //! [get instance]
  std::cout << "\nGet Instance:\n";
  auto instance = instance_admin.GetInstance(instance_id);
  if (!instance) {
    throw std::runtime_error(instance.status().message());
  }
  std::cout << "Instance details :\n" << instance->DebugString() << "\n";
  //! [get instance]

  //! [list clusters]
  std::cout << "\nListing Clusters:\n";
  StatusOr<cbt::ClusterList> cluster_list =
      instance_admin.ListClusters(instance_id);
  if (!cluster_list) {
    throw std::runtime_error(cluster_list.status().message());
  }
  if (!cluster_list->failed_locations.empty()) {
    std::cout << "The Cloud Bigtable service reports that the following "
                 "locations are temporarily unavailable and no information "
                 "about clusters in these locations can be obtained:\n";
    for (auto const& failed_location : cluster_list->failed_locations) {
      std::cout << failed_location << "\n";
    }
  }
  std::cout << "Cluster Name List:\n";
  for (auto const& cluster : cluster_list->clusters) {
    std::cout << "Cluster Name: " << cluster.name() << "\n";
  }
  std::cout << "DONE\n";
  //! [list clusters]

  //! [delete instance]
  std::cout << "Deleting instance " << instance_id << "\n";
  google::cloud::Status delete_status =
      instance_admin.DeleteInstance(instance_id);
  if (!delete_status.ok()) {
    throw std::runtime_error(delete_status.message());
  }
  std::cout << "DONE\n";
  //! [delete instance]

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard C++ exception raised: " << ex.what() << "\n";
  return 1;
}
//! [all code]
