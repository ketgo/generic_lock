// Copyright 2021 Ketan Goyal
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

#ifndef GENERIC_LOCK__DETAILS__DEPENDENCY_GRAPH_HPP
#define GENERIC_LOCK__DETAILS__DEPENDENCY_GRAPH_HPP

#include <set>
#include <unordered_map>

namespace gl {
namespace details {

/**
 * @brief Directed graph used to track depedency between different concurrently
 * running theads. A thread `A` is dependent on thread `B` if `A` is waiting
 * to access a shared data which is currently locked by `B`.
 *
 * @tparam T Thread identifier type.
 */
template <class T>
class DependencyGraph {
 public:
  /**
   * @brief Construct a new Dependency Graph object.
   *
   */
  DependencyGraph() = default;

  /**
   * @brief Add dependency from thread with identifier `id_a` to that with
   * identifier `id_b`.
   *
   * @param id_a Constant reference to the identifier of the dependent thread.
   * @param id_b Constant reference to the identifier of the depended thread.
   */
  void Add(const T& id_a, const T& id_b) { _dependency_map[id_a][id_b] = true; }

  /**
   * @brief Remove dependency from thread with identifier `id_a` to that with
   * identifier `id_b`.
   *
   * @param id_a Constant reference to the identifier of the dependent thread.
   * @param id_b Constant reference to the identifier of the depended thread.
   */
  void Remove(const T& id_a, const T& id_b) {
    // Removes dependency edge only if it exists
    auto it = _dependency_map.find(id_a);
    if (it != _dependency_map.end()) {
      it->second.erase(id_b);
      // Removes element from graph if no more dependency edges exist
      if (it->second.empty()) {
        _dependency_map.erase(it);
      }
    }
  }

  /**
   * @brief Remove all dependencies for thread with the given identifier.
   *
   * @param id Constant reference to the thread identifier.
   */
  void Remove(const T& id) {
    // Erase all dependency edges for the given transaction identifier
    for (auto& element : _dependency_map) {
      element.second.erase(id);
    }
    _dependency_map.erase(id);
  }

  /**
   * @brief Check if a thread with identifier `id_a` is depenedent on a thread
   * with identifier `id_b`.
   *
   * @param id_a Constant reference to the identifier of the dependent thread.
   * @param id_b Constant reference to the identifier of the depended thread.
   * @returns `true` if the thread is dependent else `false`.
   */
  bool IsDependent(const T& id_a, const T& id_b) {
    auto it = _dependency_map.find(id_a);
    if (it != _dependency_map.end()) {
      return it->second.find(id_b) != it->second.end();
    }
    return false;
  }

  /**
   * @brief Detects a cycle in the dependency graph. If a cycle is observed then
   * a callback is called with the set of thread identifiers that are part of
   * the detected cycle as argument. Thus the callback should have a signature
   * `void callback(std::set<T>&)`. Furthermore, the method returns `true` if a
   * cycle was observed, otherwise `false`.
   *
   * @param id Constant reference to the thread identifier from where to start
   * lookup. This identifier should exist in the dependency graph.
   * @param callback The cycle handler callback.
   * @returns `true` if a cycle was detected else `false`.
   */
  template <class CycleHandler>
  bool DetectCycle(const T& id, CycleHandler callback) const {
    std::unordered_map<T, T> parents;
    std::unordered_map<T, bool> visited;

    auto result = DetectCycle(id, parents, visited);
    if (result.second) {
      // Create the set of identifiers constructing the observed cycle
      std::set<T> ids;
      ids.insert(result.first);
      auto node = parents.at(result.first);
      while (node != result.first) {
        ids.insert(node);
        node = parents.at(node);
      }
      // Call the cycle handler callback
      callback(ids);
    }

    return result.second;
  }

 private:
  /**
   * @brief The method starts traversing the dependency graph using breath first
   * search algorithm through recursion. It marks each node as either "visited",
   * "visiting", or "not-visited". A cycle is observed when we reach a
   * "visiting" node again during our transversal. The path taken during the
   * transversal can be obtained from the `partents` map, storing the parent
   * node of each node traveled.
   *
   * @param node Constent reference to the node being transversed.
   * @param parents Reference to the map storing immediate parents of each node
   * transversed.
   * @param visited Reference to the map storing visit status of each node.
   * @returns A pair containing a flag indicating if a cycle was observed and
   * the identifier on which the cycle was observed or null.
   */
  std::pair<T, bool> DetectCycle(const T& node,
                                 std::unordered_map<T, T>& parents,
                                 std::unordered_map<T, bool>& visited) const {
    // Current node is being observed for the first time so mark it as
    // in the process of being visited.
    visited[node] = false;

    // Find cycles in the connected child nodes
    auto it = _dependency_map.find(node);
    if (it != _dependency_map.end()) {
      for (auto& element : it->second) {
        auto result = DetectCycle(element.first, node, parents, visited);
        if (result.second) {
          // Found cycle so stop transversal.
          return result;
        }
      }
    }

    // Mark the current node as completely visited.
    visited[node] = true;

    // No cycle detected.
    return {{}, false};
  }

  /**
   * @brief The method traverses the dependency graph using breath first
   * search algorithm through recursion. It marks each node as either "visited",
   * "visiting", or "not-visited". A cycle is observed when we reach a
   * "visiting" node again during our transversal. The path taken during the
   * transversal can be obtained from the `partents` map, storing the parent
   * node of each node traveled.
   *
   * @param node Constent reference to the node being transversed.
   * @param parent Constant referene to the immediate parent node previously
   * transversed.
   * @param parents Reference to the map storing immediate parents of each node
   * transversed.
   * @param visited Reference to the map storing visit status of each node.
   * @returns A pair containing a flag indicating if a cycle was observed and
   * the identifier on which the cycle was observed or null.
   */
  std::pair<T, bool> DetectCycle(const T& node, const T& parent,
                                 std::unordered_map<T, T>& parents,
                                 std::unordered_map<T, bool>& visited) const {
    // Set the parent node of the current node.
    parents[node] = parent;

    // Check if the current node has already been visited.
    auto visited_it = visited.find(node);
    if (visited_it != visited.end()) {
      // Node visited before so return if its already completely visited.
      if (visited_it->second) {
        return {{}, false};
      }
      // Node not completely visited so we have a cycle.
      return {node, true};
    }

    // Current node is being observed for the first time so mark it as
    // in the process of being visited.
    visited[node] = false;

    // Find cycles in the connected child nodes
    auto it = _dependency_map.find(node);
    if (it != _dependency_map.end()) {
      for (auto& element : it->second) {
        auto result = DetectCycle(element.first, node, parents, visited);
        if (result.second) {
          // Found cycle so stop transversal.
          return result;
        }
      }
    }

    // Mark the current node as completely visited.
    visited[node] = true;

    // No cycle detected yet.
    return {{}, false};
  }

  // Dependency map
  std::unordered_map<T, std::unordered_map<T, bool>> _dependency_map;
};

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__DEPENDENCY_GRAPH_HPP */
