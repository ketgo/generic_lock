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

#ifndef GENERIC_LOCK__DEPENDENCY_GRAPH_HPP
#define GENERIC_LOCK__DEPENDENCY_GRAPH_HPP

#include <unordered_map>

namespace gl {

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
  static constexpr T null_id = 0;

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
  void Add(const T& id_a, const T& id_b) {
    // Assert thread identifier is not null
    assert(id_a != null_id);
    assert(id_b != null_id);

    _dependency_map[id_a][id_b] = true;
  }

  /**
   * @brief Remove dependency from thread with identifier `id_a` to that with
   * identifier `id_b`.
   *
   * @param id_a Constant reference to the identifier of the dependent thread.
   * @param id_b Constant reference to the identifier of the depended thread.
   */
  void Remove(const T& id_a, const T& id_b) {
    // Assert thread identifier is not null
    assert(id_a != null_id);
    assert(id_b != null_id);

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
    // Assert thread identifier is not null
    assert(id != null_id);

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
    // Assert thread identifier is not null
    assert(id_a != null_id);
    assert(id_b != null_id);

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
   * `void callback(std::set<T>&)`.
   *
   * @param id Constant reference to the thread identifier from where to start
   * lookup. This identifier should exist in the dependency graph.
   * @param callback The cycle handler callback.
   *
   */
  template <class CycleHandler>
  void DetectCycle(const T& id, CycleHandler callback) const {
    // Assert thread identifier is not null
    assert(id != null_id);

    std::unordered_map<T, T> parents;
    std::unordered_map<T, bool> visited;

    // NOTE: Thread identifier with value of `0` is considered null.
    T node = DetectCycle(id, null_id, parents, visited);
    if (node) {
      // Create the set of identifiers constructing the observed cycle
      std::set<T> ids;
      ids.insert(node);
      T parent = parents.at(node);
      while (parent != node) {
        ids.insert(parent);
        parent = parents.at(parent);
      }
      // Call the cycle handler callback
      callback(ids);
    }
  }

 private:
  /**
   * @brief The method transverses the dependency graph using breath first
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
   * @returns The identifier on which the cycle was observed or null.
   */
  T DetectCycle(const T& node, const T& parent,
                std::unordered_map<T, T>& parents,
                std::unordered_map<T, bool>& visited) const {
    // Default return value
    T rvalue = null_id;

    // Set the parent node of the current node.
    parents[node] = parent;

    // Check if the current node has already been visited.
    auto visited_it = visited.find(node);
    if (visited_it != visited.end()) {
      // Node visited before so return if its already completely visited.
      if (visited_it->second) {
        return rvalue;
      }
      // Node not completely visited so we have a cycle.
      return node;
    }

    // Current node is being observed for the first time so mark it as
    // in the process of being visited.
    visited[node] = false;

    // Find cycles in the connected child nodes
    auto it = _dependency_map.find(node);
    if (it != _dependency_map.end()) {
      for (auto& element : it->second) {
        rvalue = DetectCycle(element.first, node, parents, visited);
        if (rvalue) {
          // Found cycle so stop transversal.
          return rvalue;
        }
      }
    }

    // Mark the current node as completely visited.
    visited[node] = true;

    // No cycle detected yet.
    return rvalue;
  }

  // Dependency map
  std::unordered_map<T, std::unordered_map<T, bool>> _dependency_map;
};

}  // namespace gl

#endif /* GENERIC_LOCK__DEPENDENCY_GRAPH_HPP */
