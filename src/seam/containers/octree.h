#pragma once

#if RUN_DOCTEST
#include "doctest.h"
#endif

#include <functional>
#include "glm/glm.hpp"

namespace seam {

	/// <summary>
	/// Sparse Octree implementation.
	/// TODOs:
	/// - Handle worst case by vectorizing Leafs past a certain depth, letting them contain > N items.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template <typename T, uint16_t N>
	class Octree {
		using PositionGetter = std::function<glm::vec3(T*)>;

	public:
		Octree(glm::vec3 _center, glm::vec3 _bounds, PositionGetter get_position) {
			tree_center = _center;
			tree_bounds = _bounds;
			root = new OctreeNode(true, nullptr);
			GetPosition = get_position;
		}

		~Octree() {
			delete root;
		}

		size_t Count() {
			return root->count;
		}

		void Add(T* item, glm::vec3 position) {
			Add(item, position, root, tree_center, tree_bounds);
		}

		void Remove(T* item, glm::vec3 position) {
			Remove(item, position, root, tree_center, tree_bounds);
		}

		void Update(T* item, glm::vec3 old_pos, glm::vec3 new_pos) {
			auto old_res = FindLeaf(old_pos, root, tree_center, tree_bounds);
			auto new_res = FindLeaf(new_pos, root, tree_center, tree_bounds);
			if (old_res.leaf != new_res.leaf) {
				// printf("update remove/add\n");

				Remove(item, old_pos);
				ValidateTree(root);

				Add(item, new_pos);

				//Remove(item, old_pos, old_res.branch, old_res.branch_center, old_res.branch_bounds);
				// Add(item, new_pos, new_res.branch, new_res.branch_center, new_res.branch_bounds);

				ValidateTree(root);
			}
		}

		/// <summary>
		/// Search for items in a sphere.
		/// </summary>
		/// <param name="center">The center of the search area.</param>
		/// <param name="radius">Radius of the sphere for the search area.</param>
		/// <param name="found">Will be cleared and then modified to add each item found within the sphere.</param>
		/// <returns></returns>
		size_t FindItems(glm::vec3 center, float radius, std::vector<T*>& found) {
			found.clear();
			return FindItems(center, radius, tree_center, tree_bounds, root, found);
		}

		void PrintTree() {
			PrintTree(root, tree_center, tree_bounds, 0);
		}

	private:
		struct OctreeNode {
			OctreeNode* parent;
			bool is_branch = false;
			size_t count = 0;

			OctreeNode(bool as_branch, OctreeNode* _parent) {
				Reconfigure(as_branch, _parent);
			}

			~OctreeNode() {
				if (is_branch) {
					for (int i = 0; i < branch_children.size(); i++) {
						if (branch_children[i] != nullptr) {
							delete branch_children[i];
						}
					}
				}
			}

			void Reconfigure(bool as_branch, OctreeNode* new_parent) {
				count = 0;
				is_branch = as_branch;
				parent = new_parent;
				branch_children = { 0 };
				items = { 0 };
			}

			union {
				std::array<OctreeNode*, 8> branch_children;
				std::array<T*, N> items;
			};
		};

		struct FindResult {
			OctreeNode* branch;
			OctreeNode* leaf;
			uint8_t leaf_index;
			glm::vec3 branch_center;
			glm::vec3 branch_bounds;
			glm::vec3 leaf_center;
			glm::vec3 leaf_bounds;
		};

		enum IntersectResult {
			INTERSECT_NONE,
			INTERSECT_PARTIAL,
			INTERSECT_FULL
		};

		void Add(T* item, glm::vec3 position, OctreeNode* branch, glm::vec3 center, glm::vec3 bounds) {
			const auto res = FindLeaf(position, branch, center, bounds);
			assert(res.branch->is_branch);
			OctreeNode* leaf = res.leaf;
			if (res.leaf == nullptr) {
				// TODO replace me with grabbing a leaf from a pool
				res.branch->branch_children[res.leaf_index] = leaf = new OctreeNode(false, res.branch);
			}

			if (leaf->count >= N) {
				// Leaf is full and needs to be split into a branch.
				// TODO pool me
				auto new_branch = new OctreeNode(true, res.branch);
				SplitLeaf(new_branch, leaf, res.leaf_center, res.leaf_bounds);
				// TODO return leaf to pool
				memset(leaf, 0, sizeof(OctreeNode));
				delete leaf;
				res.branch->branch_children[res.leaf_index] = new_branch;

				Add(item, position, new_branch, res.leaf_center, res.leaf_bounds);

				ValidateTree(root);
			} else {
				leaf->items[leaf->count] = item;

				// Increase the item count for the Leaf and each Branch above it.
				OctreeNode* updated = leaf;
				while (updated != nullptr) {
					updated->count += 1;
					updated = updated->parent;
				}
			}
		}

		void Remove(T* item, glm::vec3 position, OctreeNode* branch, glm::vec3 center, glm::vec3 bounds) {
			const auto res = FindLeaf(position, branch, center, bounds);
			assert(res.branch->is_branch);
			OctreeNode* leaf = res.leaf;
			assert(res.leaf != nullptr && res.leaf->count > 0);
			
			uint8_t i = 0;
			bool found = false;
			for (; i < res.leaf->count; i++) {
				T* leaf_item = res.leaf->items[i];
				if (leaf_item == item) {
					found = true;
					break;
				}
			}

			assert(found);
			if (found) {
				// Move the last item in the list to take the removed position.
				res.leaf->items[i] = res.leaf->items[res.leaf->count - 1];
				res.leaf->items[res.leaf->count - 1] = nullptr;

				OctreeNode* updated = res.leaf;
				while (updated != nullptr) {
					updated->count -= 1;
					updated = updated->parent;
				}

				if (res.branch->count < N/2 && res.branch->parent != nullptr) {
					CollapseBranch(res.branch, FindChildIndex(res.branch));
					ValidateTree(root);
				}
			}
		}

		size_t FindItems(
			glm::vec3 search_center, 
			float search_radius,
			glm::vec3 branch_center, 
			glm::vec3 branch_bounds,
			OctreeNode* node, 
			std::vector<T*>& found) 
		{
			IntersectResult intersect = CalculateBoxBoundsIntersection(branch_center, branch_bounds, search_center, glm::vec3(search_radius));

			if (intersect == INTERSECT_FULL) {
				// The search area completely contains this branch.
				// Find all items down this branch and do the final radius check and add.
				return AddItems(node, search_center, search_radius, found);
			} else if (intersect == INTERSECT_PARTIAL) {
				if (node->is_branch) {
					size_t added = 0;
					for (uint8_t i = 0; i < 8; i++) {
						OctreeNode* child = node->branch_children[i];
						if (child != nullptr) {
							glm::vec3 child_center = CalculateCenter(branch_center, branch_bounds, i);
							added += FindItems(search_center, search_radius, child_center, branch_bounds / 2.f, child, found);
						}
					}
					return added;
				} else {
					return AddLeafItems(node, search_center, search_radius, found);
				}
			} else {
				return 0;
			}
		}

		size_t AddItems(
			OctreeNode* node,
			glm::vec3 search_center,
			float search_radius,
			std::vector<T*>& found)
		{
			size_t added = 0;
			if (node == nullptr) {
				return 0;
			} else if (node->is_branch) {
				for (uint8_t i = 0; i < 8; i++) {
					added += AddItems(node->branch_children[i], search_center, search_radius, found);
				}
			} else {
				return AddLeafItems(node, search_center, search_radius, found);
			}
			return added;
		}

		size_t AddLeafItems(
			OctreeNode* leaf,
			glm::vec3 search_center,
			float search_radius,
			std::vector<T*>& found)
		{
			assert(!leaf->is_branch);
			size_t added = 0;
			for (uint8_t j = 0; j < leaf->count; j++) {
				// Actually grab the item's position and make sure it's within the search sphere.
				float distance = glm::distance(search_center, GetPosition(leaf->items[j]));
				if (distance <= search_radius) {
					found.push_back(leaf->items[j]);
					added += 1;
				}
			}
			return added;
		}

		IntersectResult CalculateBoxBoundsIntersection(
			glm::vec3 node_center, 
			glm::vec3 node_bounds, 
			glm::vec3 search_center, 
			glm::vec3 search_bounds) 
		{
			const glm::vec3 node_min = node_center - node_bounds;
			const glm::vec3 node_max = node_center + node_bounds;
			const glm::vec3 search_min = search_center - search_bounds;
			const glm::vec3 search_max = search_center + search_bounds;

			const bool overlaps_x = (node_min.x <= search_min.x && search_min.x <= node_max.x)
				|| (node_min.x <= search_max.x && search_max.x <= node_max.x)
				|| (search_min.x <= node_min.x && node_min.x <= search_max.x)
				|| (search_min.x <= node_min.x && node_max.x <= search_max.x);

			const bool overlaps_y = (node_min.y <= search_min.y && search_min.y <= node_max.y)
				|| (node_min.y <= search_max.y && search_max.y <= node_max.y)
				|| (search_min.y <= node_min.y && node_min.y <= search_max.y)
				|| (search_min.y <= node_min.y && node_max.y <= search_max.y);

			const bool overlaps_z = (node_min.z <= search_min.z && search_min.z <= node_max.z)
				|| (node_min.z <= search_max.z && search_max.z <= node_max.z)
				|| (search_min.z <= node_min.z && node_min.z <= search_max.z)
				|| (search_min.z <= node_min.z && node_max.z <= search_max.z);

			if (overlaps_x && overlaps_y && overlaps_z) {
				// The intersection is a full intersection if the search bounds completely encapsulate node bounds.
				return search_min.x <= node_min.x && search_max.x >= node_max.x
					&& search_min.y <= node_min.y && search_max.y >= node_max.y
					&& search_min.z <= node_min.z && search_max.z >= node_max.z
					? INTERSECT_FULL : INTERSECT_PARTIAL;
			}

			return INTERSECT_NONE;
		}

		FindResult FindLeaf(glm::vec3 position, OctreeNode* branch, glm::vec3 center, glm::vec3 bounds) {
			const auto node_index = CalculateNodeIndex(position, center, bounds);
			const auto child_node = branch->branch_children[node_index];

			const auto new_bounds = bounds / 2.0f;
			const glm::vec3 new_center = CalculateCenter(center, bounds, node_index);

			// Bail out when we reach an empty node or a leaf.
			if (child_node == nullptr || !child_node->is_branch) {
				FindResult res;
				res.branch = branch;
				res.leaf = child_node;
				res.leaf_index = node_index;
				res.branch_center = center;
				res.branch_bounds = bounds;
				res.leaf_center = new_center;
				res.leaf_bounds = new_bounds;
				return res;
			}

			// Child node is a branch, keep recursing
			return FindLeaf(position, child_node, new_center, new_bounds);
		}

		void SplitLeaf(OctreeNode* branch, OctreeNode* leaf, glm::vec3 center, glm::vec3 bounds) {
			for (size_t i = 0; i < leaf->count; i++) {
				glm::vec3 position = GetPosition(leaf->items[i]);
				std::uint8_t leaf_index = CalculateNodeIndex(position, center, bounds);
				OctreeNode* new_leaf = branch->branch_children[leaf_index];
				if (new_leaf == nullptr) {
					// TODO POOL POOOL
					branch->branch_children[leaf_index] = new_leaf = new OctreeNode(false, branch);
				}
				new_leaf->items[new_leaf->count] = leaf->items[i];
				new_leaf->count += 1;
				branch->count += 1;
			}
		}

		void CollapseBranch(OctreeNode* branch, uint8_t branch_index) {
			assert(branch->count <= N);
			OctreeNode* collapsed_leaf = new OctreeNode(false, branch->parent);
			for (uint8_t i = 0; i < 8; i++) {
				OctreeNode* node = branch->branch_children[i];
				if (node == nullptr) {
					continue;
				} else if (node->is_branch) {
					printf("collapsing inner branch\n");
					CollapseBranch(node, i);
				} 

				// IMPORTANT: this should always run after collapsing a branch!
				OctreeNode* leaf = branch->branch_children[i];
				assert(!leaf->is_branch);
				// TODO a lil memcpy would be faster...
				for (size_t j = 0; j < leaf->count; j++) {
					assert(collapsed_leaf->count < N);
					collapsed_leaf->items[collapsed_leaf->count] = leaf->items[j];
					collapsed_leaf->count += 1;
				}
			}
			assert(collapsed_leaf->count == branch->count);
			branch->parent->branch_children[branch_index] = collapsed_leaf;

			memset(branch, 0, sizeof(OctreeNode));
			delete branch;
		}

		std::uint8_t FindChildIndex(OctreeNode* branch) {
			assert(branch->parent != nullptr);
			for (uint8_t i = 0; i < 8; i++) {
				if (branch->parent->branch_children[i] == branch) {
					return i;
				}
			}
			assert(false);
		}

		std::uint8_t CalculateNodeIndex(glm::vec3 position, glm::vec3 center, glm::vec3 bounds) {
			// Ensure the passed in position is in bounds.
#if _DEBUG
			glm::vec3 center_to_pos = glm::abs(position - center);
			assert(center_to_pos.x <= bounds.x && center_to_pos.y <= bounds.y && center_to_pos.z <= bounds.z);
#endif

			bool right_of_center = position.x > center.x;
			bool above_center = position.y > center.y;
			bool ahead_of_center = position.z > center.z;

			return ((right_of_center << 2) | (above_center << 1) | ahead_of_center);
		}

		inline glm::vec3 CalculateCenter(glm::vec3 parent_center, glm::vec3 parent_bounds, uint8_t child_index) {
			// Use the index to figure out which direction we're going in.
			float right = (bool)(child_index & (1 << 2));
			float up = (bool)(child_index & (1 << 1));
			float forward = (bool)(child_index & 1);

			return glm::vec3(
				parent_center.x + (right - 0.5f) * parent_bounds.x,
				parent_center.y + (up - 0.5f) * parent_bounds.y,
				parent_center.z + (forward - 0.5f) * parent_bounds.z
			);
		}

		void PrintTree(OctreeNode* node, glm::vec3 center, glm::vec3 bounds, uint8_t depth) {
			const std::string spaces = std::string(depth * 2, ' ');
			if (node == nullptr) {
				printf("%snullptr\n", spaces.c_str());
				return;
			}

			if (node->is_branch) {
				printf("%sBranch center (%f, %f, %f), bounds (%f, %f, %f) %u items:\n",
					spaces.c_str(), center.x, center.y, center.z, bounds.x, bounds.y, bounds.z, node->count);

				for (uint8_t i = 0; i < 8; i++) {
					const auto new_bounds = bounds / 2.0f;
					const auto new_center = CalculateCenter(center, bounds, i);
					PrintTree(node->branch_children[i], new_center, new_bounds, depth + 1);
				}

			} else {
				printf("%sLeaf center (%f, %f, %f), bounds (%f, %f, %f) %u items:\n",
					spaces.c_str(), center.x, center.y, center.z, bounds.x, bounds.y, bounds.z, node->count);
				
				for (uint8_t i = 0; i < node->count; i++) {
					glm::vec3 position = GetPosition(node->items[i]);
					printf("%s (%f, %f, %f)\n", spaces.c_str(), position.x, position.y, position.z);
				}
			}
		}

		void ValidateTree(OctreeNode* node) {
#if _DEBUG
			if (node->is_branch) {
				for (uint8_t i = 0; i < 8; i++) {
					OctreeNode* child = node->branch_children[i];
					if (child != nullptr) {
						assert(child->parent == node);
						ValidateTree(child);
					}
				}
			} 
#endif
		}

		OctreeNode* root;
		glm::vec3 tree_center;
		glm::vec3 tree_bounds;
		PositionGetter GetPosition;
	};
}


#if RUN_DOCTEST
namespace {
	struct Particle {
		Particle() {

		}

		Particle(glm::vec3 pos, glm::vec3 dir = glm::vec3(0.f)) {
			position = pos;
			direction = dir;
		}
			
		glm::vec3 position;
		glm::vec3 direction;
	};

	void TestFindItems(seam::Octree<Particle, 8>& octree, glm::vec3 search_center, float search_radius, Particle* particles, size_t particle_count) {
		std::vector<Particle*> search_result;
		search_result.resize(particle_count / 10);
		
		auto start = std::chrono::high_resolution_clock::now();
		octree.FindItems(search_center, search_radius, search_result);
		auto end = std::chrono::high_resolution_clock::now();
		auto ns = (end - start).count();
		float ms = ns / 100000.f;
		std::cout << "took " << ms << "ms to find " << search_result.size() << " items, search radius: " << search_radius << std::endl;

		for (size_t i = 0; i < search_result.size(); i++) {
			Particle* p = search_result[i];
			// printf("fp %d: (%f, %f, %f)\n", i, p->position.x, p->position.y, p->position.z);
		}

		// Verify that the search result contains all the expected items.
		size_t expected = 0;
		for (size_t i = 0; i < particle_count; i++) {
			expected += (glm::distance(particles[i].position, search_center) <= search_radius);
		}
		assert(expected == search_result.size());
	}
}

TEST_CASE("Testing Octree") {
	std::function<glm::vec3(Particle*)> get = [](Particle* p) -> glm::vec3 { return p->position; };
	const float bounds = 50.f;
	seam::Octree<Particle, 8> octree = seam::Octree<Particle, 8>(glm::vec3(0.f), glm::vec3(bounds), get);
	const int particle_count = 100000;
	Particle* particles = new Particle[particle_count];

	for (int i = 0; i < particle_count; i++) {
		float x = (float)rand() / ((float)RAND_MAX) - 0.5f;
		float y = (float)rand() / ((float)RAND_MAX) - 0.5f;
		float z = (float)rand() / ((float)RAND_MAX) - 0.5f;

		glm::vec3 pos = glm::vec3(x, y, z);
		pos = pos * (bounds - 0.01f) * 2.f;
		particles[i] = Particle(pos);
		octree.Add(&particles[i], pos);
		assert(octree.Count() == i + 1);
	}

	// octree.PrintTree();

	TestFindItems(octree, glm::vec3(0.f), 20.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(0.f), 0.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(-10.f), 5.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(4.f, 49.f, 26.f), 20.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(33.f), 40.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(89.f), 50.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(89.f), 5.f, particles, particle_count);

	for (int i = 0; i < particle_count; i++) {
		octree.Remove(&particles[i], particles[i].position);
		assert(octree.Count() == particle_count - i - 1);
	}
	
	// octree.PrintTree();

	delete[] particles;
}
#endif // RUN_DOCTEST